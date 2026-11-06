#ifndef ELEVATOR_H
#define ELEVATOR_H

void initialize_lists(void);
int add_pet(int start_floor, int destination_floor, int type);
int board_pet(int floor);
void unload_pets(int floor);
void cleanup_lists(void);

#endif
