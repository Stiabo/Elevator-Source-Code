#include "elev.h" //used for getting the floor sensor
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "com.h"
#include "elevators.h"
#include "elevCtrl.h"

//typedef int bool;
//#define true 1
//#define false 0

//This function finds the next direction for the elevator.
//Input is the elev struct: it uses the current direction and the queues to find the direction.
//Return the next direction: 
//up = 1, 
//down = -1, 
//openDor(you are in the next floor) = 2, 
//stand still (e.a. there is no others left) = 0
int queue_findDirection(elevators *elev){
	int j;
	switch (elev->direction[elev->self]){
		//If direction is 0 (At floor)
		case 0:
			//If elevator is in a floor, and any internal queue in current floor: return 2 (open door)
			if((elev->internalQueue[elev->self][elev->lastFloor[elev->self]]||elev->internalQueueUp[elev->self][elev->lastFloor[elev->self]] || elev->internalQueueDown[elev->self][elev->lastFloor[elev->self]]) && elev_get_floor_sensor_signal() >= 0){return 2;}

			//Checks all the enteries, in the queues abow current floor, for any orders. Return 1 if so (direction is up).
			for(j = elev->lastFloor[elev->self]+1; j < N_FLOORS; j++){
				if(elev->internalQueueUp[elev->self][j] || elev->internalQueueDown[elev->self][j] || elev->internalQueue[elev->self][j]){return 1;}
			}

            //Checks all the enteries, in the queues below current floor, for any orders. Return -1 if so (direction is down).
			for(j = 0; j < elev->lastFloor[elev->self]; j++){
				if(elev->internalQueueUp[elev->self][j] || elev->internalQueueDown[elev->self][j] || elev->internalQueue[elev->self][j]){return -1;}
			}

            break;
        //If direction is 1(up).
        case 1:
            //If stoped in between floors, set direction -1 if you want to go back to your last floor 
            if (elev->internalQueue[elev->self][elev->lastFloor[elev->self]] && elev_get_floor_sensor_signal() == -1 && elev->stopRestart[elev->self]) {return -1;}

		    //Check if there is any internal queue above last floor, if so, return 1 
		    for(j = elev->lastFloor[elev->self]+1; j < N_FLOORS; j++){
			    if(elev->internalQueue[elev->self][j]||elev->internalQueueUp[elev->self][j] ||elev->internalQueueDown[elev->self][j]){return 1;}
		    }
            //Check if there is any internal queue below last floor, if so, return -1
		    for(j = 0; j < elev->lastFloor[elev->self]; j++){
			    if(elev->internalQueue[elev->self][j]||elev->internalQueueUp[elev->self][j] ||elev->internalQueueDown[elev->self][j]){return -1;}
		    }
            break;

        //If direction is -1 (down)
        case -1:
            //If stoped in between floors, set direction 1 if you want to go back to your last floor
            if (elev->internalQueue[elev->self][elev->lastFloor[elev->self]] && elev_get_floor_sensor_signal() == -1 && elev->stopRestart[elev->self]) {return 1;}
            //Check if there is any internal queue below last floor, if so, return -1
		    for(j = 0; j < elev->lastFloor[elev->self]; j++){
			    if(elev->internalQueue[elev->self][j]||elev->internalQueueUp[elev->self][j] ||elev->internalQueueDown[elev->self][j]){return -1;}
		    }
            //Check if there is any internal queue above last floor, if so, return 1
		    for(j = elev->lastFloor[elev->self]+1; j < N_FLOORS; j++){
			    if(elev->internalQueue[elev->self][j]||elev->internalQueueUp[elev->self][j] ||elev->internalQueueDown[elev->self][j]){return 1;}
		    }   
            break;
        default:
            return 0;
            break;
    }
    return 0;
}

//Test if the elevator should stop in current floor. Returns 0 for not stop, 1 for stop.
int queue_checkFloor(elevators *elev){
    int i;
	int stop = 0;
	//Test if there are more queues in the direction
	int test = 0; 
	switch (elev->direction[elev->self]){
		case 1:
		    //Check the floors above the current floor if there is any queue there, set test to 1 if true
			for (i = elev->lastFloor[elev->self] + 1; i<N_FLOORS; i++){
				if (elev->internalQueueUp[elev->self][i] || elev->internalQueueDown[elev->self][i] || elev->internalQueue[elev->self][i]){test = 1;}
			}
			//If there is any queue internalQueue or internalQueueUp, stop the elevator at the current floor.
			if (elev->internalQueueUp[elev->self][elev->lastFloor[elev->self]] || elev->internalQueue[elev->self][elev->lastFloor[elev->self]]){stop = 1;}
			//If no more queue above current floor, and queue in internalQueueDown, stop the elevator at the current floor
			else if((elev->internalQueueDown[elev->self][elev->lastFloor[elev->self]]) && !test){stop = 1;}
			break;

        //Same as in direcion up, but check for direction down
		case -1:
			for (i = 0; i<elev->lastFloor[elev->self]; i++){
				if (elev->internalQueueUp[elev->self][i] || elev->internalQueueDown[elev->self][i] || elev->internalQueue[elev->self][i]){test = 1;}
			}
			
			if (elev->internalQueueDown[elev->self][elev->lastFloor[elev->self]] || elev->internalQueue[elev->self][elev->lastFloor[elev->self]]){stop = 1;}
			
			else if((elev->internalQueueUp[elev->self][elev->lastFloor[elev->self]]) && !test){stop = 1;}
			break;
		default:
			return stop;
			break;
	}
//Hvis heisen skal stoppe i etasjen pga at noen skal nedover eller oppover.
	return stop;
}
