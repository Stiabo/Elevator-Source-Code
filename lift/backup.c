#define _POSIX_SOURCE
#include <sys/types.h>
//#include <signal.h>
//#include <unistd.h>
#include <sys/wait.h>

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
#include "backup.h"
#include <stdlib.h>
#include <sys/time.h>
#include "elevators.h"
#include "elevCtrl.h"
//pid
#include <unistd.h>
//signals
#include <signal.h>
//shm
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 


#define BUFLEN 512
#define PORT_BACKUP 12001
#define true 1
#define false 0

//Global?
int shmflg; /* shmflg to be passed to shmget() */ 
int shmID; /* return value from shmget() */ 
pid_t *shmPTR;
sig_atomic_t signalFlag;

//Uses signal handeling to send a message every 0.5 sec to a backup terminal. 
//Also writes to backupFile.dat
//Run as a thread while main program runs
void* backup_send(void *arg){
    elevators* elev = (void*)arg;
    printf("Main running...\n");
	pid_t pid = getpid();
	key_t myKey = ftok(".", 's');
	shmID = shmget(myKey,sizeof(pid_t), IPC_CREAT | 0666); // (key, size, shmflg);
	shmPTR = (pid_t *) shmat(shmID, NULL, 0);
	
	while(elev->run){
		clock_t wait = clock();
		while(!elevators_testTime(0.5,wait)){}
		pid = *shmPTR; //Read pid
		kill(pid,SIGINT);
		//Write to backupFile
		backup_writeFile(elev);
	}
	printf("Main terminated...\n");
	pthread_exit(arg);
}

//Receives a message from backup_sendSignal.
//If there is no message received the program will finish, and start as normal mode
//Program is runned in backup mode (mode = 'b')
void backup_receive(void){

	printf("Start backup receive...\n");
	signal(SIGINT, SIGINT_handler); //Activate the signal handler
	pid_t pid = getpid(); //get process ID
	key_t myKey = ftok(".", 's'); //Create a key for the shared memory
	shmID = shmget(myKey,sizeof(pid_t), IPC_CREAT | 0666); //Create the sheared memory w/(key, size, shmflg)
	shmPTR = (pid_t *) shmat(shmID, NULL, 0); //Conect to the shared memory
	*shmPTR = pid; //Store the pid in the shared memory
	
	//If no signal from program in normal mode is received within 6 secounds, while loop ends
	clock_t testTimer = clock();
	while(!elevators_testTime(1,testTimer)){
		if (signalFlag == 1){
			testTimer = clock();
			signalFlag = 0;
		}
		clock_t wait = clock();
		while(!elevators_testTime(0.2,wait)){}
		printf("Signal flag: %d\n",signalFlag);
	}
	printf("Backup mode terminate...\n");
}

void SIGINT_handler(int sig){
	signal(SIGINT, SIGINT_handler);
	signalFlag = 1;
	printf("singalFlag: %d\n",signalFlag);
}


//Write to file to store data, for backup reasons.

int backup_writeFile(elevators *elev){
    FILE *fp;
    
    fp = fopen("backupFile.dat", "w");
    if (fp == NULL) {
        printf("I couldn't open backupFile.txt for writing.\n");
        return 1;
    }
    int j;
    //Write to file: self \n internalQueue \n testButtonUp \n testButtonDown \n
    //lastFloor \n direction \n currentFloor \n stop \n internalQueueUp \n internalQueueDown
      
    //self
    fprintf(fp,"%d\n",elev->self);
    //InternalQueue
    for(j = 0;j<numberOfFloors;j++){fprintf(fp,"%d ",elev->internalQueue[elev->self][j]); }
    fprintf(fp,"\n");
    //testButtonUp
    for(j = 0;j<numberOfFloors;j++){fprintf(fp,"%d ",elev->testButtonUp[j]);}
	fprintf(fp,"\n");
	//testButtonDown
	for(j = 0;j<numberOfFloors;j++){fprintf(fp,"%d ",elev->testButtonDown[j]);}
	fprintf(fp,"\n");
	//lastFloor
    fprintf(fp,"\n%d\n",elev->lastFloor[elev->self]); 
    //direction
    fprintf(fp,"%d\n",elev->direction[elev->self]); 
    //currentFloor
	fprintf(fp,"%d\n",elev->currentFloor[elev->self]); 
	//stop
    fprintf(fp,"%d\n",elev->stop[elev->self]);
    
	if(elevators_countAlive(elev) == 1){
	    //InternalQueueUp
        for(j = 0;j<numberOfFloors;j++){fprintf(fp,"%d ",elev->internalQueueUp[elev->self][j]);}
        fprintf(fp,"\n");
        //InternalQueueDown
        for(j = 0;j<numberOfFloors;j++){fprintf(fp,"%d ",elev->internalQueueDown[elev->self][j]);}
        fprintf(fp,"\n");
    }
    // close the file 
    fclose(fp);
    return 0;
    
}



//Read from file, if found: insert values from file to elevator

int backup_readFile(elevators *elev){
    FILE *fp;
    int i;
    int j;
    /* open the file */
    fp = fopen("backupFile.dat", "r");
    if (fp == NULL) {
        printf("Couldn't open test.dat for reading.\n");
        return 1;
    }
    printf("Found old elevator\n");
    //self
    if(fscanf(fp, "%d", &i) == 1){elev->self = i;}
    //internalQueue
    for(j = 0;j<numberOfFloors;j++){
        if(fscanf(fp,"%d ",&i)==1){elev->internalQueue[elev->self][j] = i;} 
    }
    //testButtonUp
    for(j = 0;j<numberOfFloors;j++){
        if(fscanf(fp,"%d ",&i) == 1){elev->testButtonUp[j] = i;}
    }
    //testButtonDown
    for(j = 0;j<numberOfFloors;j++){
        if(fscanf(fp,"%d ",&i) == 1){elev->testButtonUp[j] = i;}
    }
    //lastFloor
    if(fscanf(fp,"%d",&i) == 1){elev->lastFloor[elev->self] = i;} 
    //direction
    if(fscanf(fp,"%d",&i) == 1){elev->direction[elev->self] = i;} 
    //currentFloor
    if(fscanf(fp,"%d",&i) == 1){elev->currentFloor[elev->self] = i;} 
    //stop
    if(fscanf(fp, "%d", &i) == 1){elev->stop[elev->self] = i;}
    
    //internalQueueUp
    for(j = 0;j<numberOfFloors;j++){
        if(fscanf(fp,"%d ",&i)==1){elev->internalQueueUp[elev->self][j] = i;} 
    }
    //internalQueueDown
    for(j = 0;j<numberOfFloors;j++){
        if(fscanf(fp,"%d ",&i)==1){elev->internalQueueDown[elev->self][j] = i;} 
    }
    /* close the file */
    fclose(fp);
    return 0;
}


