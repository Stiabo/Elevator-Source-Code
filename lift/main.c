//make -f Makefile.mk
//./lift n
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
#include "sim.h"
#include "backup.h"
#include <signal.h>

#define BUFLEN 512
#define PORT 12004

#define true 1
#define false 0

//Arguments to take in: mode
//mode: normal(n), backup (b)

int main(int argc, char** argv){
	
	if(argc != 2){
      		printf("Usage : %s Need 1 argument: mode(n/b) IP\n",argv[0]);
      		exit(0);
    }
	printf("Start program...\n");
	//Initializing
	printf("init program...\n");
	elevators elev;
	elev = elevators_init(elev,0);

	pthread_t com_readMsg_thr;
	pthread_t com_sendBC_thr;
	pthread_t backup_thr;

	clock_t waitTime;
	clock_t sendTime;
	char mode = *argv[1];
	while(elev.run){
	    switch (mode) {
		    case 'n':	
			    pthread_create(&com_readMsg_thr, NULL, &com_readMsg, &elev);
			    pthread_create(&com_sendBC_thr, NULL, &com_sendBC, &elev);
			
			    //Read backup file, if new elevator: find a number
                if(backup_readFile(&elev)){
			        com_setMsgBC(10,elev.self,0,0,0,0,&elev);
                    waitTime = clock();
			        while(elev.self == 0 && !elevators_testTime(10,waitTime)){
				        sendTime = clock();
				        com_setMsgBC(10,elev.self,0,0,0,0,&elev);
				        while(!elevators_testTime(2,sendTime)){}
				        printf("try to connect\n");
	
			        }
			        if(elev.self==0){
				        elev.self= 1;
				        elev.numberOfElevators = 1;
			        }
			    }
			    
		        printf("Open backup terminal\n");
			    pthread_create(&backup_thr, NULL, &backup_send, &elev);
			    if(system("mate-terminal -x ./lift b") == -1){printf("Backup Error!\n");};
			
			    pthread_t elevCtrl_listen_thr;
			    pthread_create(&elevCtrl_listen_thr, NULL, &elevCtrl_listen, &elev);
			    
			    elevCtrl_init(&elev);
		
			    pthread_t sim_thr;
			    pthread_create(&sim_thr, NULL, &sim_findElev, &elev);
	
			    pthread_t elevators_isAlive_thr;
			    pthread_create(&elevators_isAlive_thr, NULL, &elevators_isAlive, &elev);
			    
			    elev.alive[elev.self] = 1;
			    
			    int currentState = 1;
			    while(elev.run){
				    if (state(currentState, &elev)){printf("Unvalid state...\n");}
				    currentState = nextState(currentState, &elev);
			    }
			    
			    elev_set_speed(0);
			    pthread_join(com_readMsg_thr,NULL);
			    pthread_join(com_sendBC_thr,NULL);
			    pthread_join(backup_thr,NULL);
			    pthread_join(elevCtrl_listen_thr,NULL);
			    pthread_join(sim_thr,NULL);
			    pthread_join(elevators_isAlive_thr,NULL);
			    break;
		    case 'b':
			    backup_receive();
			    printf("Change to normal mode...\n");
			    mode = 'n';
			    break;
		    default:
			    printf("Wrong input (b/n)...\n");
			    elev.run = false;
			    break;
		}
	}		
	return 0;
}
