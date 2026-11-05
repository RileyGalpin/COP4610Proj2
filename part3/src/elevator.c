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

//Protects elevator and floor data
static DEFINE_MUTEX(elevator_mutex);

static int pets_serviced = 0;

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
    pets_waiting = 0;
    pets_serviced = 0;

    mutex_unlock(&elevator_mutex);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pet elevator kernel module");

static int __init elevator_init(void)
{
    initialize_lists();

    printk(KERN_INFO "Elevator module loaded\n");

    return 0;
}

static void __exit elevator_exit(void)
{
    cleanup_lists();

    printk(KERN_INFO "Elevator module unloaded\n");
}

module_init(elevator_init);
module_exit(elevator_exit);