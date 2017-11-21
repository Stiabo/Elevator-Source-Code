#include "elev.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "com.h"
#include "elevators.h"
#include "elevCtrl.h"
#include "queue.h"
#include <pthread.h>

//typedef int bool;
#define true 1
#define false 0


//This function is used to initizlise the elevator
//Input is the elevator struct
//Flag COM to send changes is status
void elevCtrl_init(elevators *elev){
	printf("elevCtrl_init: Initilize elevator...\n");
	//bool startup = true;
	int startup = true;
	if(!elev_init()){
		printf("elevCtrl_init: Error during start up. Contact suport...\n");
		assert(!elev_init());
	}
	elev_set_speed(0);
	
	//Blocks if obstruction or stop is pressed
	while(startup){
		if(!elev_get_obstruction_signal() && !elev->stop[elev->self]){
			startup = false;
		}
	}

	//if elevator not in floor, drive to nerest floor abow current position
	if (elev_get_floor_sensor_signal() == -1){
		printf("Floor: %d \n",elev_get_floor_sensor_signal());
		int speed = MAX_SPEED;
		if(elev->direction[elev->self] != 0){
			int dir = elev->direction[elev->self];
			elev->speed[elev->self] = speed*dir;
			elev_set_speed(speed*dir);
		}else{
			elev->speed[elev->self] = speed;
			elev_set_speed(speed);
		}

		//Flag COM to send status
		com_setMsgBC(4,elev->self,0,0,0,0,elev);

		while (speed > 0) {
			if (elev_get_floor_sensor_signal() >= 0){
				speed = elevCtrl_break(elev);
			}
		}
	}
	elevCtrl_arrival(elev);
	if(queue_checkFloor(elev)){
		elevCtrl_disembarking(elev);
		clock_t wait = clock();
		while(!elevators_testTime(3,wait)){}
	}
	printf("elevCtrl_init: Elevator Initilized...\n");
}


//Breaks the elevator. To make sure the elevator stopes as fast as posible, 
//the speed dir is changes for a short moment. 
//The breaktime most be config for each elevator
int elevCtrl_break(elevators *elev){
	printf("elevCtrl_break: The elevator breaks...\n");
	int speed = elev->speed[elev->self];
	speed = speed/-8;
	elev_set_speed(speed);
	
	clock_t startTime = clock();
	while(!elevators_testTime(0.3, startTime)){}

	speed = 0;
	elev->speed[elev->self] = speed;
	elev_set_speed(speed);
	
	//Flag COM to send status
	com_setMsgBC(4,elev->self,0,0,0,0,elev);
	
	printf("elevCtrl_break: The elevator has stoped...\n");
	return speed;
}

//Lisn if anyone is pressing the stop button, if so, 
//elev->stop is set(only once), lamp is set, internal queue is deleted
//and flag COM to send status
void elevCtrl_listenStop(elevators *elev){
	if(elev_get_stop_signal() && elev->stop[elev->self] == 0){
		printf("elevCtrl_listStop: WORNING: Stop has been presed...\n");
		elev->stop[elev->self] = 1;
		elev_set_stop_lamp(1);
		elev->stopRestart[elev->self] = 1;
		
		//Open door if elevator is in floor
		if((elev_get_floor_sensor_signal() >= 0) && (elev->door[elev->self] == false)){
		    elev_set_door_open_lamp(1);
		    elev->door[elev->self] = true;
		}

		//Flag COM to send status
		com_setMsgBC(4,elev->self,0,0,0,0,elev);

		//Delete internal queue
		for (int i = 0; i < N_FLOORS; i++) {
		    elev_set_button_lamp(2, i, 0);
		    elev->internalQueue[elev->self][i] = 0;	    
		}
		
		//Flag COM to send internal queue
		com_setMsgBC(1,elev->self,0,0,0,0,elev);
	}
}

//Lisen if anyone is pressing an internal queue button, if so:
//Set internal queue and lamp (only once)
void elevCtrl_listenInternalQueue(elevators *elev){

	//Chacks all internal queue buttons	
	int floor;
	for(floor = 0; floor < N_FLOORS; floor++){
		if(!(elev->currentFloor[elev->self] == floor && elev->door[elev->self]) || elev->stop[elev->self]){
			if(elev_get_button_signal(2,floor) && elev->internalQueue[elev->self][floor] == 0){
				elev->internalQueue[elev->self][floor] = 1;
				
				//Flag COM to send internal queue
				com_setMsgBC(1,elev->self,0,0,0,0,elev);
				
				//Reset stop button (if pressed)
				if (elev->stop[elev->self] == 1){
					elev->stop[elev->self] = 0;
					elev_set_stop_lamp(0);

					//Flag COM to send status
					com_setMsgBC(4,elev->self,0,0,0,0,elev);
				}
			}
		}
	}
}


//Listen if anyone is pressing an queue up button, if so:
//set the corresponding queueUp (once)
void elevCtrl_listenQueueUp(elevators *elev){
	int floor;
	for(floor = 0; floor < N_FLOORS-1; floor++){
		if(!(elev->currentFloor[elev->self] == floor && elev->door[elev->self] && elev->stop[elev->self] && elev->obs[elev->self])){
			if (elev_get_button_signal(0,floor) && elev->simUp[floor] == 0 && elev->testButtonUp[floor] == 0){
				elev_set_button_lamp(0,floor,1);
				elev->simUp[floor] = 1;
				elev->testButtonUp[floor] = 1;
			}
			else if(elev_get_button_signal(0,floor) == 0 && elev->testButtonUp[floor] == 1){elev->testButtonUp[floor] = 0;}
		}
	}
}

//Listen if anyone is pressing an queue down button, if so:
//set the corresponding queueDown (once)
void elevCtrl_listenQueueDown(elevators *elev){
	int floor;
	for(floor = 1; floor < N_FLOORS; floor++){
		if(!(elev->currentFloor[elev->self] == (floor) && elev->door[elev->self] && elev->stop[elev->self] && elev->obs[elev->self])){
			if (elev_get_button_signal(1,floor) && elev->simDown[floor] == 0 && elev->testButtonDown[floor] == 0){
				elev_set_button_lamp(1,floor,1);
				elev->simDown[floor] = 1;
				elev->testButtonDown[floor] = 1;
			}
			else if(elev_get_button_signal(1,floor) == 0 && elev->testButtonDown[floor] == 1){elev->testButtonDown[floor] = 0;}
		}
	}
}


//This thread listen for buttons beeing pressed and setting ligths
//Therminate thread by setting elev->run = false
void* elevCtrl_listen(void *arg){
	printf("elevCtrl_elevListen: The elevator is now lisning for buttons and seting ligths...\n");
	elevators *elev = (elevators*)(arg);
	while(elev->run){
		elevCtrl_listenInternalQueue(elev);
		elevCtrl_listenQueueUp(elev);
		elevCtrl_listenQueueDown(elev);
		elevCtrl_listenStop(elev);
		elevCtrl_listenOBS(elev);
		elevCtrl_setLigths(elev);
	}
	printf("elevCtrl_elevListen: The elevator has stoped lisning for input and setting ligths...\n");
	pthread_exit(arg);
}


//This function checks that all queue ligths are correct according to the queue arraies
void elevCtrl_setLigths(elevators *elev){

	//If any up/down ligths is not corret, set them correct
	int i;
	int j;
	
	int test = false;
	for(i = 0; i < numberOfFloors -1;i++){
		for(j = 0; j < maxNumberOfElevators; j++){
			if(elev->alive[j]){
				if (elev->internalQueueUp[j][i]){test = true;}
			}
		}
		elev_set_button_lamp(0,i,test);
		//elev->lampUp[i] = elev->internalQueueUp[j][i];
		test = false;
	}
			
	for(i = 1; i < numberOfFloors;i++){
		for(j = 0; j < maxNumberOfElevators; j++){
			if(elev->alive[j]){
				if (elev->internalQueueDown[j][i]){test = true;}
			}
		}
		elev_set_button_lamp(1,i,test);
		//elev->lampDown[i] = elev->internalQueueDown[j][i];
		test = false;
	}
	
	//if any internal queue ligths is not correct, set them correct
	for(j = 0; j < numberOfFloors; j++){
		if(elev->internalQueue[elev->self][j] != elev->lamp[j]){
			elev_set_button_lamp(2,j,elev->internalQueue[elev->self][j]);
			elev->lamp[j] = elev->internalQueue[elev->self][j];
		}
	}
}


//This function is to bee run when the elevator arrive in a new floor:
//Set the ligth indicator to the rigth floor, set last and current floor
//and flag COM to send status
void elevCtrl_arrival(elevators *elev){
	int floor = elev_get_floor_sensor_signal();
	if (floor >= 0){
	    elev->lastFloor[elev->self] = floor;
        elev->currentFloor[elev->self] = floor;
        elev_set_floor_indicator(floor);
        printf("Arrival at floor: %d \n", floor);
	    com_setMsgBC(4,elev->self,0,0,0,0,elev);
	}
}


//stopes the elevator, open the door,turn of internal queue ligths
//delete queue, Flag Com
void elevCtrl_disembarking(elevators* elev){
	if(elev_get_floor_sensor_signal()>= 0) {
		if(elev->speed[elev->self] != 0){elevCtrl_break(elev);}
		elev_set_door_open_lamp(1);
		elev->door[elev->self] = 1;
		elev_set_button_lamp(2,elev->currentFloor[elev->self],0);
		//Flag COM to send status
		com_setMsgBC(4,elev->self,0,0,0,0,elev);
		
		elev->internalQueue[elev->self][elev->currentFloor[elev->self]] = 0;
		elev->internalQueueUp[elev->self][elev->currentFloor[elev->self]] = 0;
		elev->internalQueueDown[elev->self][elev->currentFloor[elev->self]] = 0;
		
		//Flag COM to send internal queue, 
		com_setMsgBC(1,elev->self,0,0,0,0,elev);
		com_setMsgBC(5,elev->self,0,0,0,0,elev);
		com_setMsgBC(6,elev->self,0,0,0,0,elev);
	}
	else{printf("elevCtrl_disembarking: Something went wrong...\n");}
}


//listen for obs, if obs (only once until not obs) set obs and flag COM
void elevCtrl_listenOBS(elevators *elev){
    if(elev_get_obstruction_signal() && elev->obs[elev->self] == 0){
		elev->obs[elev->self] = 1;
		//Flag COM to send status
		com_setMsgBC(4,elev->self,0,0,0,0,elev);
	}
    else if (!elev_get_obstruction_signal() && elev->obs[elev->self] == 1){
        elev->obs[elev->self] = 0;
        //Flag COM to send status
        com_setMsgBC(4,elev->self,0,0,0,0,elev);
    }
}


//This function checks wich dir it should drive in (according to the queue)
//and (if nessecery) turn on the motor in the rigth direction. Flag Com to send
void elevCtrl_run(elevators *elev){

	//Find the next direction
	elev->direction[elev->self] = queue_findDirection(elev);
	elev->stopRestart[elev->self] = 0;
	switch (elev->direction[elev->self]){
		//Dir up
		case 1:
			elev_set_door_open_lamp(0);
			elev->door[elev->self] = false;
			elev->speed[elev->self] = MAX_SPEED;
			elev_set_speed(elev->speed[elev->self]);
			break;
		//dir down
		case -1:
			elev_set_door_open_lamp(0);
			elev->door[elev->self] = false;
			elev->speed[elev->self] = -MAX_SPEED;
			elev_set_speed(elev->speed[elev->self]);
			break;
		//Elevator allrady in the next floor
		case 2:
			elev->direction[elev->self] = 0;
			break;
		default:
			break;
    } 
    //flag COM to send status
    com_setMsgBC(4,elev->self,0,0,0,0,elev);
}




















