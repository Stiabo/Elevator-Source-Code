#include "elevators.h"
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
#include "elevCtrl.h"


//Takes in an elevators struct and display the information stored in the variable elev
void elevators_print(elevators elev){
	int i;
	int j;
	printf("\n\n\n\n==================THE ELEVATORS======================\n\n");
	//system("clear");
	printf("This elevator number is: %d\n",elev.self);
	printf("NumberOfElevators reserved: %d\n",elev.numberOfElevators);
	for(i = 0;i<maxNumberOfElevators;i++){
		if(elev.alive[i]){
			printf("Elevator number %d\n", i);
			printf("Alive: %d\n",elev.alive[i]);
			printf("Dir: %d ",elev.direction[i]);
			printf("LastFloor: %d ",elev.lastFloor[i]);
			printf("Obs: %d ",elev.obs[i]);
			printf("Stop: %d ",elev.stop[i]);
			printf("Door: %d \n",elev.door[i]);
			printf("Speed: %d \n",elev.speed[i]);
			printf("CurrentFloor: %d \n",elev.currentFloor[i]);
			printf("internalQueue: ");	
			for(j = 0;j<numberOfFloors;j++){
				printf("%d ",elev.internalQueue[i][j]);			
			}
			printf("\n");
			printf("internalQueueUp: ");	
			for(j = 0;j<numberOfFloors;j++){
				printf("%d ",elev.internalQueueUp[i][j]);			
			}
			printf("\n");
			printf("internalQueueDown: ");	
			for(j = 0;j<numberOfFloors;j++){
				printf("%d ",elev.internalQueueDown[i][j]);			
			}
			printf("\n\n");
		}
	}
	printf("simUp: \n");
	for(i = 0; i<numberOfFloors;i++){
		printf("%d ",elev.simUp[i]);
	}
	printf("\n\nsimDown: \n");
	for(i = 0; i<numberOfFloors;i++){
		printf("%d ",elev.simDown[i]);
	}
	printf("\n====================================================\n\n");
}


//Initialize the input argument elevators, by setting all values to 0
//Returns the initialized elevators
elevators elevators_init(elevators elev, int selfNumber){
	int i;
	int j;
	elev.self = selfNumber;
	elev.run = true;
	elev.numberOfElevators = 1;
	//elev.IP_broadcast = "129.241.187.255";
	for(i = 0;i<maxNumberOfElevators;i++){
		elev.obs[i] = 0;
		elev.direction[i] = 0;
		elev.stop[i] = 0;
		elev.lastFloor[i] = 0;
		elev.door[i] = 0;
		elev.speed[i] = 0;
		elev.currentFloor[i] = -1;
		elev.alive[i] = 0;
		elev.testAlive[i] = 0;
		for(j = 0;j<numberOfFloors;j++){
			elev.internalQueue[i][j] = 0;
			elev.internalQueueUp[i][j] = 0;
			elev.internalQueueDown[i][j] = 0;		
		}
	}
	for(i = 0; i<numberOfFloors;i++){
		elev.simUp[i] = 0;
		elev.simDown[i] = 0;
		elev.testButtonUp[i] = 0;
		elev.testButtonDown[i] = 0;
		elev.lamp[i] = 0;
	}
	for(i = 0; i<msgBCL;i++){
		for(j = 0; j<msgBCData;j++){
			elev.msgBC[i][j] = 0;
		}
	}
	return elev;
}

//Sends signals on broadcast, to tell other elevators connected that it is alive.
//Sets all testAlive values to 0, to test if other elevators set their testAlive back to 1.
//If testAlive is still 0, or an unused testAlive is 1, an elevator have disconnected/connected.
//Run as a thread while program (elev->run) is active
void* elevators_isAlive(void* arg){
	printf("start isAlive_thr\n");
	elevators *elev = (elevators*)(arg);
	clock_t startTime;
	int i;
	while(elev->run){
	    //
		for(i = 0;i<maxNumberOfElevators;i++){
			if(elev->alive[i]){
				elev->testAlive[i] = 0;      
			}
        }
        //Send signal to other elevators
        for(i = 0;i<3;i++){
			com_setMsgBC(12,elev->self,0,0,0,0,elev);
			startTime = clock();   
		    while(!elevators_testTime(3,startTime)){}
		}
		//Check if there are elevators connected/disconnected
        for(i = 0;i<maxNumberOfElevators;i++){
        	if(i != elev->self){
				if(elev->alive[i]){
					if(elev->testAlive[i] != elev->alive[i]){
						elev->alive[i] = 0;
						printf("Elevator %d disconnected\n",i);
						//Transfer the queue of the disconnected elevator
						if(elev->self == elevators_lowestNumber(elev)){
							elevators_transferQueue(elev,i);
						}
					}	
							
				}else if(!elev->alive[i]){
					if(elev->testAlive[i] != elev->alive[i]){
						//if(elev->numberOfElevators == i){elev->numberOfElevators = elev->numberOfElevators + 1;}
						elev->alive[i] = 1;
						com_sendAll(elev);
						printf("Elevator %d connected\n",i);
					}
				}    
			}
        }
	}
	pthread_exit(arg);
}
//Return the lowest elevator number alive
int elevators_lowestNumber(elevators *elev){
	int i = 0;
	for(i = 0;i<maxNumberOfElevators;i++){
		if(elev->alive[i]){return i;}
	}
	return -1;
}
//Transfers the internalQueueUp/Down from elevator i, to simUp/Down (Picked up by sim and finds optimal elevator)
void elevators_transferQueue(elevators *elev,int i){
	for(int j = 0;j<numberOfFloors;j++){
		if(elev->internalQueueUp[i][j]){
			elev->simUp[j] = 1;
			elev->internalQueueUp[i][j] = 0;
		}
		if(elev->internalQueueDown[i][j]){
			elev->simDown[j] = 1;
			elev->internalQueueDown[i][j] = 0;
		}
	}
}
//Returns the number of elevators alive (with network connection)
int elevators_countAlive(elevators *elev){
    int counter = 0;
    for(int i = 0;i<maxNumberOfElevators;i++){
        if(elev->alive[i]){counter++;}
    }
    return counter;
}
//Returns 1 when it have gone "delay" sec from when startTime was set
int elevators_testTime(double delay, clock_t startTime){
	clock_t clockTicksTaken = clock() - startTime;
	double timeInSeconds = clockTicksTaken/(double) CLOCKS_PER_SEC;
	if(timeInSeconds>delay){
		return 1;
	}
	else{
		return 0;
	}
}


