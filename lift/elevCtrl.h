#ifndef ELEVATOR_H
#define ELEVATOR_H

//#include <time.h>
#include "elevators.h"

#define MAX_SPEED 300;

void elevCtrl_init(elevators *elev);

int elevCtrl_break(elevators *elev);

void elevCtrl_listenStop(elevators *elev);

void elevCtrl_listenInternalQueue(elevators *elev);

void elevCtrl_listenQueueDown(elevators *elev);

void elevCtrl_listenQueueUp(elevators *elev);

void* elevCtrl_listen(void *arg);

void elevCtrl_setLigths(elevators *elev);

void elevCtrl_arrival(elevators *elev);

void elevCtrl_disembarking(elevators *elev);

void elevCtrl_listenOBS(elevators *elev);

void elevCtrl_run(elevators *elev);

#endif 
