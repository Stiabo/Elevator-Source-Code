#ifndef ELEVATORS_H
#define ELEVATORS_H

#define true 1
#define false 0

#define numberOfFloors 4
#define maxNumberOfElevators 10
#define msgBCL 30
#define msgBCData 7
#define LONG_TIME 999

#include <time.h>


typedef struct elevators elevators;

//Struct that cointains all information about the elevators
struct elevators{
//Sync	
	int internalQueue[maxNumberOfElevators][numberOfFloors];
	int internalQueueUp[maxNumberOfElevators][numberOfFloors];
	int internalQueueDown[maxNumberOfElevators][numberOfFloors];
	int simUp[numberOfFloors];
	int simDown[numberOfFloors];

	int direction[maxNumberOfElevators];
	int lastFloor[maxNumberOfElevators];
	int obs[maxNumberOfElevators];
	int stop[maxNumberOfElevators];
	int door[maxNumberOfElevators];
	int speed[maxNumberOfElevators];
	int currentFloor[maxNumberOfElevators];
//Local
	int self;
	int numberOfElevators;
	int alive[maxNumberOfElevators];
    int stopRestart[maxNumberOfElevators];
    int testAlive[maxNumberOfElevators];
	int msgBC[msgBCL][msgBCData];
	int testButtonUp[numberOfFloors];
	int testButtonDown[numberOfFloors];
	int lamp[numberOfFloors];
	//int lampUp[numberOfFloors];
	//int lampDown[numberOfFloors];
	int run;
	clock_t elevTime;
};

void elevators_print(elevators elev);

elevators elevators_init(elevators elev, int selfNumber);

void* elevators_isAlive(void* arg);

int elevators_lowestNumber(elevators *elev);

void elevators_transferQueue(elevators *elev,int i);

int elevators_countAlive(elevators *elev);

int elevators_testTime(double delay, clock_t startTime);
#endif
