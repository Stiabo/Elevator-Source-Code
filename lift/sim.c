#include "sim.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "com.h"
#include "elevators.h"
#include "elevCtrl.h"
#include "elev.h"
#include "state.h"
#include "queue.h"

//How much things are wigthed in the simulation
#define wTravel 3
#define wStop 5
#define wPass 1
#define wQueue 1
#define wDir 10




//This function is to be ran in an thread.
//Input: the elevators struct.
//Run the simulations when the program flags for the thread to run a simulation.
//Flaging is done by setting and index(floor) in queue Up/down.
//The thread sets a CMD(9) message in the com sendBuffer.
void* sim_findElev(void *arg){
	printf("Start the simulation thread...\n");
	elevators *elev = (elevators*)arg;
	int totalTime[maxNumberOfElevators];//[elev->numberOfElevators];
	int candidateFloor;
	int candidateUp;
	int i;
	int bestElev;
	int bestTime;

	while(elev->run){

		//Checks if there is any simulation to be preformed
		//If so: candidateFloor and candidateUp is set; while end
		i = 0;
		candidateFloor = -1;
		candidateUp = -1;
		while(candidateFloor == -1 && i < numberOfFloors){
			if(elev->simUp[i]){
				candidateFloor = i;
				candidateUp = 1;
			}
			else if(elev->simDown[i]){
				candidateFloor = i;
				candidateUp = 0;
			}
			i++;
		}
		//If start simulation: Checks if elevator j is alive and is not stoped/obs
		// The simulation stores all simulation times in an array
		if(candidateFloor != -1){
			for(int j = 0;j < maxNumberOfElevators;j++){
				if(elev->alive[j]){
					if(elev->stop[j] || elev->obs[j]){totalTime[j] = LONG_TIME;}
					else{
						totalTime[j] = sim_timeToArrival(elev,j,candidateFloor,candidateUp);
					}
				}	
			}
		
			//Find the best elevator to use based on the array
			bestTime = totalTime[elevators_lowestNumber(elev)];
			bestElev = elevators_lowestNumber(elev);
			for(int k = elevators_lowestNumber(elev)+1;k < maxNumberOfElevators;k++){
				if(elev->alive[k]){
					if(totalTime[k]<bestTime){
						bestTime = totalTime[k];
						bestElev = k;
					}
				}
			}

			//Set a CMD msg in buffer to be sent (will be sent to the best elevator)
			if(candidateUp == 1){
				//send a CMD msg
				com_setMsgBC(9,elev->self,bestElev,0,candidateFloor,1,elev);
				elev->simUp[candidateFloor] = 0;
				elev->internalQueueUp[bestElev][candidateFloor] = 1;

			}
			else if(candidateUp == 0){
				//send a CMD msg
				com_setMsgBC(9,elev->self,bestElev,1,candidateFloor,1,elev);
				elev->simDown[candidateFloor] = 0;
				elev->internalQueueDown[bestElev][candidateFloor] = 1;
			}
		}
	}
	return 0;
}

//Simulate how long time it takes before elevatorNumber arrive at candidateFloor
//Return a estimated timeCost value
//The defines is the waigth used
int sim_timeToArrival(elevators *elev, int elevatorNumber, int candidateFloor, int candidateUp){
	int time = 0;
	//Make a copy of the three queues to elevatorNumber
	int cpyInternalQueue[sizeof(elev->internalQueue[elevatorNumber])];
	int cpyInternalQueueUp[sizeof(elev->internalQueueUp[elevatorNumber])];
	int cpyInternalQueueDown[sizeof(elev->internalQueueDown[elevatorNumber])];
	for(int i = 0; i < N_FLOORS; i++){
		cpyInternalQueue[i] = elev->internalQueue[elevatorNumber][i];
		cpyInternalQueueUp[i] = elev->internalQueueUp[elevatorNumber][i];
		cpyInternalQueueDown[i] = elev->internalQueueDown[elevatorNumber][i];
	}
	
	//cpy the floor where the elevator is
	int cpyCurrentFloor = elev->currentFloor[elevatorNumber];
	if (cpyCurrentFloor < 0){
		cpyCurrentFloor = elev->lastFloor[elevatorNumber];
	}
	
	//Enter the candidateFloor in to the cpy queue
	switch (candidateUp){
		case 0:
			cpyInternalQueueDown[candidateFloor] = 1;
			break;
		case 1:
			cpyInternalQueueUp[candidateFloor] = 1;
			break;
	}
	
	int cpyDir = elev->direction[elevatorNumber];
	if(cpyDir != candidateUp && cpyDir != 0 && cpyDir != 2){time = time + wDir;}
	
	int sim_run = true;
	int inQueue;
	int countStops = 0;
	while(sim_run){
		//Find direction
		cpyDir = sim_findDirection(cpyInternalQueueUp, cpyInternalQueueDown, cpyInternalQueue, cpyDir, cpyCurrentFloor);
		if (cpyDir == 1 || cpyDir == -1){time = time + wTravel;}
		else if (cpyDir == 2){return 0;}
		else if (cpyDir == 0){sim_run = false;}
		//set next floor
		cpyCurrentFloor = cpyCurrentFloor + cpyDir;
		inQueue = sim_floorInQueue(cpyDir,cpyCurrentFloor,cpyInternalQueueUp,cpyInternalQueueDown,cpyInternalQueue);
		//if floor in queue, add wStop to time
		if (inQueue && (cpyCurrentFloor != candidateFloor)){
			time = time + wStop;
			countStops++;
			cpyInternalQueue[cpyCurrentFloor] = 0;
			cpyInternalQueueUp[cpyCurrentFloor] = 0;
			cpyInternalQueueDown[cpyCurrentFloor] = 0;
		}
		//stopes the simulation if the sim has rerached the correct floor
		else if(inQueue && (cpyCurrentFloor == candidateFloor)){
			sim_run = false;
		}
		else{time = time + wPass;}
	}
	time = time + wQueue*countStops;
	return time;
}


//simulate the next direction
//Return the next (sim) direction
int sim_findDirection(int *internalQueueUp, int *internalQueueDown, int *internalQueue, int dir, int currentFloor){
	int j;
	switch (dir){
		//If direction is 0 (At floor)
		case 0:
			//If elevator is in a floor, and any internal queue in current floor: return 2 (open door)
			if(internalQueue[currentFloor] || internalQueueUp[currentFloor] || internalQueueDown[currentFloor]){return 2;}

			//Checks all the enteries, in the queues abow current floor, for any orders. Return 1 if so (direction is up).
			for(j = currentFloor+1; j < numberOfFloors; j++){
				if(internalQueueUp[j] || internalQueueDown[j] || internalQueue[j]){return 1;}
			}

            //Checks all the enteries, in the queues below current floor, for any orders. Return -1 if so (direction is down).
			for(j = 0; j < currentFloor; j++){
				if(internalQueueUp[j] || internalQueueDown[j] || internalQueue[j]){return -1;}
			}

            break;
        //If direction is 1(up).
        case 1:
            //If stoped in between floors, set direction -1 if you want to go back to your last floor 
            if (internalQueue[currentFloor]) {return -1;}

		    //Check if there is any internal queue above last floor, if so, return 1 
		    for(j = currentFloor+1; j < numberOfFloors; j++){
			    if(internalQueue[j] || internalQueueUp[j] || internalQueueDown[j]){return 1;}
		    }
            //Check if there is any internal queue below last floor, if so, return -1
		    for(j = 0; j < currentFloor; j++){
			    if(internalQueue[j] || internalQueueUp[j] || internalQueueDown[j]){return -1;}
		    }
            break;

        //If direction is -1 (down)
        case -1:
            //If stoped in between floors, set direction 1 if you want to go back to your last floor
            if (internalQueue[currentFloor]) {return 1;}
            //Check if there is any internal queue below last floor, if so, return -1
		    for(j = 0; j < currentFloor; j++){
			    if(internalQueue[j] || internalQueueUp[j] || internalQueueDown[j]){
				    return -1;
			    }
		    }
            //Check if there is any internal queue above last floor, if so, return 1
		    for(j = currentFloor+1; j < numberOfFloors; j++){
			    if(internalQueue[j] || internalQueueUp[j] || internalQueueDown[j]){
				    return 1;
			    }
		    }   
            break;
        default:
            return 0;
            break;
    }
    return 0;
}



//chacks if the (sim) current floor is in the (sim) queue
//returns stop = 1 or notStop = 0
//Does not stop if dir is down and floor is in queue on dirUP 
int sim_floorInQueue(int dir, int currentFloor, int *internalQueueUp, int *internalQueueDown, int *internalQueue){
    int i;
	int stopp = 0;
	int test = 0;

	//dir up
	switch (dir){
		//dir up, tests if there is any floor in the queue abow current floor
		case 1:
			for (i = currentFloor + 1; i<N_FLOORS; i++){
				if (internalQueueUp[i] || internalQueueDown[i] || internalQueue[i]){
					test = 1;
				}
			}
			if (internalQueueUp[currentFloor]|| internalQueue[currentFloor]){
				stopp = 1;
			}
			else if((internalQueueDown[currentFloor]) && !test){
				stopp = 1;
			}
			break;

		//dir down, same as abow, but in down dir.
		case -1:
			for (i = 0; i<currentFloor; i++){
				if (internalQueueUp[i] || internalQueueDown[i] || internalQueue[i]){
					test = 1;
				}
			}
			if (internalQueueDown[currentFloor] || internalQueue[currentFloor]){
				stopp = 1;
			}
			else if((internalQueueUp[currentFloor]) && !test){
				stopp = 1;
			}
	}
	return stopp;
}

