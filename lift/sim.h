#ifndef SIM_H
#define SIM_H

#include "elevators.h"

int sim_timeToArrival(elevators *elev, int elevatorNumber, int candidateFloor, int candidateCMD);

int sim_findDirection(int *internalQueueUp, int *internalQueueDown, int *internalQueue, int dir, int currentFloor);

int sim_floorInQueue(int dir, int currentFloor, int *internalQueueUp, int *internalQueueDown, int *internalQueue);

void* sim_findElev(void* arg);

#endif
