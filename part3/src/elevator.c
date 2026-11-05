#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include "elevator.h"
#define NUM_FLOORS 5
#define MAX_PETS 5
#define MAX_WEIGHT 50

enum pet_type {
    CHIHUAHUA = 0,
    PUG,
    PUGHUAHUA,
    DACHSHUND
};

enum elevator_state {
    ELEVATOR_OFFLINE,
    ELEVATOR_IDLE,
    ELEVATOR_LOADING,
    ELEVATOR_UP,
    ELEVATOR_DOWN
};

enum elevator_direction {
    DIRECTION_UP,
    DIRECTION_DOWN
};

static const int pet_weights[] = {
    3, 14, 10, 16
};

struct pet {
    enum pet_type type;
    int start_floor;
    int destination_floor;
    int weight;

    struct list_head list;
};

//Five waiting lists for each floor
static struct list_head floor_queues[NUM_FLOORS];

//List of pets in elevator currently 
static struct list_head elevator_passengers;

//Elevator load information
static int current_load = 0;

//Current pet count 
static int current_pet_count = 0;

static enum elevator_state current_state = ELEVATOR_OFFLINE;
static int current_floor = 1;
static struct task_struct *elevator_thread;
static bool stop_requested;
static int pets_waiting;
static enum elevator_direction current_direction = DIRECTION_UP;
static struct proc_dir_entry *elevator_proc_entry;

/* These symbols are exported by the custom kernel's syscall implementation. */
extern int (*STUB_start_elevator)(void) __attribute__((weak));
extern int (*STUB_issue_request)(int, int, int) __attribute__((weak));
extern int (*STUB_stop_elevator)(void) __attribute__((weak));

//Protects elevator and floor data
static DEFINE_MUTEX(elevator_mutex);

static int pets_serviced = 0;

/* Check for work above the current floor while elevator_mutex is held. */
static bool requests_above_locked(void)
{
    int floor;
    struct pet *pet;

    for (floor = current_floor; floor < NUM_FLOORS; floor++) {
        if (!list_empty(&floor_queues[floor]))
            return true;
    }

    list_for_each_entry(pet, &elevator_passengers, list) {
        if (pet->destination_floor > current_floor)
            return true;
    }

    return false;
}

/* Check for work below the current floor while elevator_mutex is held. */
static bool requests_below_locked(void)
{
    int floor;
    struct pet *pet;

    for (floor = 0; floor < current_floor - 1; floor++) {
        if (!list_empty(&floor_queues[floor]))
            return true;
    }

    list_for_each_entry(pet, &elevator_passengers, list) {
        if (pet->destination_floor < current_floor)
            return true;
    }

    return false;
}

/* During shutdown, only onboard pets are allowed to determine movement. */
static bool passengers_above_locked(void)
{
    struct pet *pet;

    list_for_each_entry(pet, &elevator_passengers, list) {
        if (pet->destination_floor > current_floor)
            return true;
    }

    return false;
}

static bool passengers_below_locked(void)
{
    struct pet *pet;

    list_for_each_entry(pet, &elevator_passengers, list) {
        if (pet->destination_floor < current_floor)
            return true;
    }

    return false;
}

/* Complete one loading/unloading stop and report whether anything changed. */
static bool service_current_floor(void)
{
    int pets_before;
    int pets_after;
    bool boarded = false;

    mutex_lock(&elevator_mutex);
    current_state = ELEVATOR_LOADING;
    pets_before = current_pet_count;
    mutex_unlock(&elevator_mutex);

    unload_pets(current_floor);

    while (board_pet(current_floor))
        boarded = true;

    mutex_lock(&elevator_mutex);
    pets_after = current_pet_count;
    mutex_unlock(&elevator_mutex);

    return boarded || pets_before != pets_after;
}

/* The worker owns movement decisions; list and counter changes stay mutex-protected. */
static int elevator_thread_fn(void *data)
{
    bool floor_changed;
    bool work_above;
    bool work_below;

    while (!kthread_should_stop()) {
        floor_changed = service_current_floor();

        if (floor_changed)
            msleep(1000);

        mutex_lock(&elevator_mutex);

        if (stop_requested && current_pet_count == 0) {
            current_state = ELEVATOR_OFFLINE;
            elevator_thread = NULL;
            mutex_unlock(&elevator_mutex);
            break;
        }

        if (current_pet_count == 0 && pets_waiting == 0) {
            current_state = ELEVATOR_IDLE;
            mutex_unlock(&elevator_mutex);
            msleep(100);
            continue;
        }

        work_above = stop_requested ? passengers_above_locked() :
                                      requests_above_locked();
        work_below = stop_requested ? passengers_below_locked() :
                                      requests_below_locked();

        if (current_direction == DIRECTION_UP &&
            current_floor < NUM_FLOORS && work_above) {
            current_state = ELEVATOR_UP;
            current_direction = DIRECTION_UP;
            current_floor++;
        } else if (current_direction == DIRECTION_DOWN &&
                   current_floor > 1 && work_below) {
            current_state = ELEVATOR_DOWN;
            current_direction = DIRECTION_DOWN;
            current_floor--;
        } else if (current_floor > 1 && work_below) {
            current_state = ELEVATOR_DOWN;
            current_direction = DIRECTION_DOWN;
            current_floor--;
        } else if (current_floor < NUM_FLOORS && work_above) {
            current_state = ELEVATOR_UP;
            current_direction = DIRECTION_UP;
            current_floor++;
        } else if (stop_requested && current_pet_count == 0) {
            current_state = ELEVATOR_OFFLINE;
            elevator_thread = NULL;
            mutex_unlock(&elevator_mutex);
            break;
        } else {
            current_state = ELEVATOR_IDLE;
            mutex_unlock(&elevator_mutex);
            msleep(100);
            continue;
        }

        mutex_unlock(&elevator_mutex);
        msleep(2000);
    }

    return 0;
}

void initialize_lists(void)
{
    int i;

    for (i = 0; i < NUM_FLOORS; i++) {
        INIT_LIST_HEAD(&floor_queues[i]);
    }

    INIT_LIST_HEAD(&elevator_passengers);

    current_state = ELEVATOR_OFFLINE;
    current_floor = 1;
    elevator_thread = NULL;
    stop_requested = false;
    current_direction = DIRECTION_UP;
    current_load = 0;
    current_pet_count = 0;
    pets_waiting = 0;
    pets_serviced = 0;
}


int add_pet(int start_floor, int destination_floor, int type)
{
    struct pet *new_pet;

    // Validate request
    if (start_floor < 1 || start_floor > NUM_FLOORS ||
        destination_floor < 1 || destination_floor > NUM_FLOORS ||
        start_floor == destination_floor ||
        type < CHIHUAHUA || type > DACHSHUND) {
        return -EINVAL;
    }

    // Allocate memory for new pet
    new_pet = kmalloc(sizeof(*new_pet), GFP_KERNEL);

    if (!new_pet) {
        return -ENOMEM;
    }

    //Fill in pet information 
    new_pet->type = type;
    new_pet->start_floor = start_floor;
    new_pet->destination_floor = destination_floor;
    new_pet->weight = pet_weights[type];

    INIT_LIST_HEAD(&new_pet->list);

    mutex_lock(&elevator_mutex);

    if (current_state == ELEVATOR_OFFLINE || stop_requested) {
        mutex_unlock(&elevator_mutex);
        kfree(new_pet);
        return -EINVAL;
    }

    list_add_tail(&new_pet->list, &floor_queues[start_floor - 1]);
    pets_waiting++;

    mutex_unlock(&elevator_mutex);

    return 0;
}


int board_pet(int floor)
{
    struct pet *next_pet;

    if (floor < 1 || floor > NUM_FLOORS)
        return -EINVAL;

    mutex_lock(&elevator_mutex);

    if (stop_requested) {
        mutex_unlock(&elevator_mutex);
        return 0;
    }

    //Check if a pet is waiting
    if (list_empty(&floor_queues[floor - 1])) {
        mutex_unlock(&elevator_mutex);
        return 0;
    }

    //Get the first pet in queue
    next_pet = list_first_entry(
        &floor_queues[floor - 1],
        struct pet,
        list
    );

    //Check capacity of elevator
    if (current_pet_count >= MAX_PETS ||
        current_load + next_pet->weight > MAX_WEIGHT) {
        mutex_unlock(&elevator_mutex);
        return 0;
    }

    //Move pet from floor to elevator
    list_del_init(&next_pet->list);
    list_add_tail(&next_pet->list, &elevator_passengers);

    //Updates elevator information 
    current_pet_count++;
    current_load += next_pet->weight;
    pets_waiting--;

    mutex_unlock(&elevator_mutex);

    return 1;
}


void unload_pets(int floor)
{
    struct pet *pet;
    struct pet *next;

    if (floor < 1 || floor > NUM_FLOORS)
        return;

    mutex_lock(&elevator_mutex);

    //Check all pets in elevator
    list_for_each_entry_safe(pet, next, &elevator_passengers, list) {

        //Check if a pet has reached its floor
        if (pet->destination_floor == floor) {

            //Remove pet from elevator
            list_del(&pet->list);

            //Update elevator information 
            current_load -= pet->weight;
            current_pet_count--;
            pets_serviced++;

            //Free pet's allocated memory
            kfree(pet);
        }
    }

    mutex_unlock(&elevator_mutex);
}

void cleanup_lists(void)
{
    struct pet *pet;
    struct pet *next;
    int i;

    mutex_lock(&elevator_mutex);

    //Free pets waiting
    for (i = 0; i < NUM_FLOORS; i++) {
        list_for_each_entry_safe(pet, next, &floor_queues[i], list) {
            list_del(&pet->list);
            kfree(pet);
        }
    }

    //Free pets inside elevator
    list_for_each_entry_safe(pet, next, &elevator_passengers, list) {
        list_del(&pet->list);
        kfree(pet);
    }

    //Reset all elevator information 
    current_load = 0;
    current_pet_count = 0;
    current_state = ELEVATOR_OFFLINE;
    current_floor = 1;
    elevator_thread = NULL;
    stop_requested = false;
    current_direction = DIRECTION_UP;
    pets_waiting = 0;
    pets_serviced = 0;

    mutex_unlock(&elevator_mutex);
}

/* Start creates the worker while holding the state lock, then wakes it after
 * its task pointer has been published. This prevents a second start from
 * creating a competing worker during initialization. */
static int elevator_start(void)
{
    struct task_struct *thread;

    mutex_lock(&elevator_mutex);

    if (elevator_thread || current_state != ELEVATOR_OFFLINE) {
        mutex_unlock(&elevator_mutex);
        return 1;
    }

    current_floor = 1;
    current_direction = DIRECTION_UP;
    current_state = ELEVATOR_IDLE;
    stop_requested = false;

    thread = kthread_create(elevator_thread_fn, NULL, "elevator");
    if (IS_ERR(thread)) {
        current_state = ELEVATOR_OFFLINE;
        mutex_unlock(&elevator_mutex);
        return PTR_ERR(thread);
    }

    elevator_thread = thread;
    mutex_unlock(&elevator_mutex);
    wake_up_process(thread);

    return 0;
}

/* Requests are rejected once shutdown begins; the worker drains onboard pets
 * and leaves waiting pets in their floor queues for a future start. */
static int elevator_issue_request(int start_floor, int destination_floor,
                                  int type)
{
    int result;

    result = add_pet(start_floor, destination_floor, type);
    return result < 0 ? 1 : 0;
}

/* Stop is asynchronous: the syscall marks shutdown, and the worker changes
 * to OFFLINE only after every onboard pet has reached its destination. */
static int elevator_stop(void)
{
    mutex_lock(&elevator_mutex);

    if (stop_requested) {
        mutex_unlock(&elevator_mutex);
        return 1;
    }

    stop_requested = true;
    if (!elevator_thread && current_pet_count == 0)
        current_state = ELEVATOR_OFFLINE;

    mutex_unlock(&elevator_mutex);
    return 0;
}

static const char *elevator_state_name(enum elevator_state state)
{
    switch (state) {
    case ELEVATOR_IDLE:
        return "IDLE";
    case ELEVATOR_LOADING:
        return "LOADING";
    case ELEVATOR_UP:
        return "UP";
    case ELEVATOR_DOWN:
        return "DOWN";
    case ELEVATOR_OFFLINE:
    default:
        return "OFFLINE";
    }
}

static char pet_type_name(enum pet_type type)
{
    switch (type) {
    case CHIHUAHUA:
        return 'C';
    case PUG:
        return 'P';
    case PUGHUAHUA:
        return 'H';
    case DACHSHUND:
        return 'D';
    default:
        return '?';
    }
}

/* Produce the assignment's labeled elevator snapshot while holding the same
 * mutex used by producers and the worker, so no list is read mid-update. */
static int elevator_proc_show(struct seq_file *file, void *unused)
{
    struct pet *pet;
    int floor;
    int waiting_count;

    mutex_lock(&elevator_mutex);

    seq_printf(file, "Elevator state: %s\n",
               elevator_state_name(current_state));
    seq_printf(file, "Current floor: %d\n", current_floor);
    seq_printf(file, "Current load: %d lbs\n", current_load);
    seq_puts(file, "Elevator status:");
    list_for_each_entry(pet, &elevator_passengers, list)
        seq_printf(file, " %c%d", pet_type_name(pet->type),
                   pet->destination_floor);
    seq_putc(file, '\n');

    for (floor = NUM_FLOORS - 1; floor >= 0; floor--) {
        waiting_count = 0;
        seq_printf(file, "Floor %d: %s ", floor + 1,
                   current_floor == floor + 1 ? "[*]" : "[ ]");

        list_for_each_entry(pet, &floor_queues[floor], list) {
            waiting_count++;
            seq_printf(file, "%c%d ", pet_type_name(pet->type),
                       pet->destination_floor);
        }

        seq_printf(file, "(%d waiting)\n", waiting_count);
    }

    seq_printf(file, "Number of pets: %d\n", current_pet_count);
    seq_printf(file, "Number of pets waiting: %d\n", pets_waiting);
    seq_printf(file, "Number of pets serviced: %d\n", pets_serviced);

    mutex_unlock(&elevator_mutex);
    return 0;
}

static int elevator_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, elevator_proc_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
    .proc_open = elevator_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* The custom kernel exports these callback slots from syscalls.c. Weak
 * declarations keep the module buildable on a non-custom development kernel. */
static void register_syscall_callbacks(void)
{
    if (&STUB_start_elevator)
        STUB_start_elevator = elevator_start;
    if (&STUB_issue_request)
        STUB_issue_request = elevator_issue_request;
    if (&STUB_stop_elevator)
        STUB_stop_elevator = elevator_stop;
}

static void unregister_syscall_callbacks(void)
{
    if (&STUB_start_elevator)
        STUB_start_elevator = NULL;
    if (&STUB_issue_request)
        STUB_issue_request = NULL;
    if (&STUB_stop_elevator)
        STUB_stop_elevator = NULL;
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pet elevator kernel module");

static int __init elevator_init(void)
{
    initialize_lists();

    elevator_proc_entry = proc_create("elevator", 0444, NULL,
                                      &elevator_proc_ops);
    if (!elevator_proc_entry)
        return -ENOMEM;

    register_syscall_callbacks();

    printk(KERN_INFO "Elevator module loaded\n");

    return 0;
}

static void __exit elevator_exit(void)
{
    struct task_struct *thread;

    unregister_syscall_callbacks();

    if (elevator_proc_entry) {
        proc_remove(elevator_proc_entry);
        elevator_proc_entry = NULL;
    }

    mutex_lock(&elevator_mutex);
    stop_requested = true;
    thread = elevator_thread;
    mutex_unlock(&elevator_mutex);

    if (thread)
        kthread_stop(thread);

    cleanup_lists();

    printk(KERN_INFO "Elevator module unloaded\n");
}

module_init(elevator_init);
module_exit(elevator_exit);