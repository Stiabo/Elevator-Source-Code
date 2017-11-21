#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>
#include "elevators.h"
#include "elevCtrl.h"
#include "elev.h"
#include "queue.h"
#include "com.h"

#define true 1
#define false 0

#define idle 1
#define set_speed 2
#define travel 3
#define arrival 4
#define disembarking 5
#define wait_door 6
#define break_elevator 7
#define wait_obs_stop 8


//Holds the current state of the elevator, executes the functions in the given states
//Normal loop: idle -> set_speed -> travel -> arrival -> disembarking -> wait_door -> idle
//If stop or obs: travel -> break_elevator -> wait_obs_stop -> set_speed
int state(int currentState, elevators *elev){
    clock_t startTime;
	switch (currentState){
        case idle:
            if (queue_findDirection(elev) == 0){
                if(elev->direction[elev->self] != 0){
                    elev->direction[elev->self] = 0;
                    //Send
                    com_setMsgBC(4,elev->self,0,0,0,0,elev);
                 }
            }
            break;
        case set_speed:
            elevCtrl_run(elev);
            break;
        case travel:
            if(elev->currentFloor[elev->self] != -1 && elev_get_floor_sensor_signal() == -1){
                elev->currentFloor[elev->self] = -1;
                //Send
                com_setMsgBC(4,elev->self,0,0,0,0,elev);
            }
            break;
        case arrival:
            elevCtrl_arrival(elev);
            break;
        case disembarking:
            elevCtrl_disembarking(elev);
            break;
        case wait_door:
			startTime = clock();
            while((!elevators_testTime(5,startTime) || elev->obs[elev->self] || elev->stop[elev->self]) && elev_get_floor_sensor_signal() >= 0){}
            elev_set_door_open_lamp(0);
            elev->door[elev->self] = 0;
            //send
            com_setMsgBC(4,elev->self,0,0,0,0,elev);
            break;
        case break_elevator:
            elevCtrl_break(elev);
            elev->elevTime = clock();
            break;
        case wait_obs_stop:
            if(elev_get_floor_sensor_signal() == -1 && elev->door[elev->self]){
                elev->door[elev->self] = 0;
                elev_set_door_open_lamp(0);
                com_setMsgBC(4,elev->self,0,0,0,0,elev);
            }
            //If the elevator have stopped for a little while, send a new elevator to take its orders
            if(elevators_testTime(5,elev->elevTime)){
            	elevators_transferQueue(elev,elev->self);
            	elev->elevTime = clock();
            }
            break;
        default:
            elev->run = false;
            return 1;
            break;
    }
	return 0;
}

//Takes in the current state and returns the next state
int nextState(int currentState,elevators *elev){
    switch (currentState){
        case idle:
            if(elev->obs[elev->self] || elev->stop[elev->self]){return idle;}
            else if (queue_findDirection(elev) == 2){return disembarking;}
            else if (queue_findDirection(elev) == 0){return idle;}
            else if(queue_findDirection(elev) == 1 || queue_findDirection(elev) == -1){return set_speed;}
            else{return idle;}
            break;
        case set_speed:
            return travel;
            break;
        case travel:
            if(elev->obs[elev->self] || elev->stop[elev->self]){return break_elevator;}
            else if (elev_get_floor_sensor_signal() >= 0 && elev_get_floor_sensor_signal() != elev->currentFloor[elev->self]){return arrival;}
            else{return travel;}
            break;
        case arrival:
            if (queue_checkFloor(elev)){return disembarking;}
            else{return travel;}
            break;
        case disembarking:
            return wait_door;
            break;
        case wait_door:
            return idle;
            break;
        case break_elevator:
            return wait_obs_stop;
            break;
        case wait_obs_stop:
            if(!elev->obs[elev->self] && !elev->stop[elev->self]){return set_speed;}
            else{return wait_obs_stop;}
            break;
        default:
            printf("Unable to set next state because previous state was not a valid state\n");
            break;  
    }
    
    return 0;
}

