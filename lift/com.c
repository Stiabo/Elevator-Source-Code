//#define __USE_XOPEN
#include "com.h"
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
#include <strings.h>
#include "elevators.h"
#include "sim.h"
#include "elevCtrl.h"
#include <signal.h>
#include <unistd.h>

sig_atomic_t signalFlagPIPE;

//This function handles some errors
void err(char *s){
    perror(s);
    //exit(1);
}

//This function handle a SIGPIPE interupt ("Network conection lost")
void SIGPIPE_handler(int sig_int){
	signal(SIGPIPE, SIGPIPE_handler);
	signalFlagPIPE = 1;
}


//Set up a UDP connection to send msg buffer on broadcast
//Run as a thread as long as the main program (elev->run) is running
void* com_sendBC(void *arg){
	printf("com_sendBC: A communication broadcast thread has been started...\n");
    elevators *elev = (elevators*)(arg);

	//Sets up a signal PIPE handler and a flag
	signal(SIGPIPE, SIGPIPE_handler);
	signalFlagPIPE = 0;

	while(elev->run){
		//Create a UDP socket
		printf("com_sendBC: Set up socket...\n");
		struct sockaddr_in serv_addr;
		int sockfd, slen=sizeof(serv_addr), i;
		char buf[BUFLEN];
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){
		    err("socket");
		    continue;
		}
		//Set option for broadcast permission
		int broadcastPermission = 1;
		if(setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,(void *) &broadcastPermission,sizeof(broadcastPermission))<0){
		    err("setsockopt()");
		    continue;
		}
		bzero(&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(PORT);
	
		printf("com_sendBC: Conecting...\n");
		if (inet_aton("129.241.187.255", &serv_addr.sin_addr)==0){
			err("inet_aton() failed");
			continue;
		}
		printf("com_sendBC: Initialiation is done...\n");

		int socketConected = true;
		while(socketConected && elev->run){

			//If a SIGPIPE(socket disconected) has been flaged...
			if (signalFlagPIPE == 1){
				close(sockfd);
				printf("com_sendBC: The network conection has been terminated...\n");
				socketConected = false;
				signalFlagPIPE = 0;		
			}
	
			//Searches msgBC for a new message to be sendt. Send the message found and set buffer to 0
			for(i = 0; i < msgBCL; i++){
		        if (elev->msgBC[i][0] == 1){
			        char msgToSend[100];
					if(!com_createMsgBC(elev, i, msgToSend)){
						//Add length to msgToSend
				        bzero(buf,BUFLEN);
				        char msg_length_char[20];
				        sprintf(msg_length_char, "%d", (int)sizeof(msgToSend));
				        strcat(msg_length_char, " ");
				        strcat(buf,msg_length_char);
				        strcat(buf,msgToSend);
			            //End of length editing
			            
			            //Send the message
						if (sendto(sockfd, buf, BUFLEN, 0, (struct sockaddr*)&serv_addr,  slen)==-1){break;}//err("sendto()");}
						//Reset buffer
						elev->msgBC[i][0] = 0;
					}
		        }
        	}
		}
		close(sockfd);
	}
	
	printf("com_sendBC: The comunication thread has been terminated...\n");
	pthread_exit(arg);
}


//Puts data into a buffer, send form thread com_sendBC when msgBC[i][0] = 1
void com_setMsgBC(int msgType,int fromElevator,int toElevator,int cmd,int floor,int set,elevators *elev){
	for(int i = 0;i<msgBCL;i++){
		if (elev->msgBC[i][0] == 0){
			elev->msgBC[i][1] = msgType;
			elev->msgBC[i][2] = fromElevator;
			elev->msgBC[i][3] = toElevator;
			elev->msgBC[i][4] = cmd;
			elev->msgBC[i][5] = floor;
			elev->msgBC[i][6] = set;
			elev->msgBC[i][0] = 1;
			break;
		}
	}
}


//Create a char that can be sendt as one transfer over network, with necessary data.
//Different cases creates different data to send (queue, status, ect)
//Return 0 for sucess, 1 for fail
int com_createMsgBC(elevators *elev,int j,char *msg){
	char temp[100] = "";
	int i;
	char a[14];
	int msgType = elev->msgBC[j][1];
	int fromElevator = elev->msgBC[j][2];
	switch (msgType){
		//internalQueue
		case 1:
			strcat(temp,"1");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			for(i = 0;i<numberOfFloors;i++){
				strcat(temp," ");
				sprintf(a, "%d", elev->internalQueue[fromElevator][i]); 
				strcat(temp,a);
			}
			break;
		//status
		case 4:
			strcat(temp,"4");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->direction[fromElevator]); 
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->lastFloor[fromElevator]); 
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->obs[fromElevator]); 
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->stop[fromElevator]); 
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->door[fromElevator]); 
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->speed[fromElevator]); 
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", elev->currentFloor[fromElevator]); 
			strcat(temp,a);
			break;
		//intenalQueueUp
		case 5:
			printf("fromElevatorBC: %d\n",fromElevator);
			strcat(temp,"5");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			for(i = 0;i<numberOfFloors;i++){
				strcat(temp," ");
				sprintf(a, "%d", elev->internalQueueUp[fromElevator][i]); 
				strcat(temp,a);
			}
			break;
		//internalQueueDown
		case 6:
			strcat(temp,"6");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			for(i = 0;i<numberOfFloors;i++){
				strcat(temp," ");
				sprintf(a, "%d", elev->internalQueueDown[fromElevator][i]); 
				strcat(temp,a);
			}
			break;
		//CMD (command)
		case 9:
			printf("create 9\n");
			strcat(temp,"9");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			strcat(temp," ");
			int toElevator = elev->msgBC[j][3];
			int cmd = elev->msgBC[j][4];
			int floor = elev->msgBC[j][5];
			int set = elev->msgBC[j][6];
			sprintf(a, "%d", toElevator);
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", cmd);
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", floor);
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", set);
			strcat(temp,a);
			break;
		//Request join
		case 10:
			strcat(temp,"10");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			break;
		//Reply join
		case 11:
			strcat(temp,"11");
			strcat(temp," ");
			sprintf(a, "%d", fromElevator);
			strcat(temp,a);
			strcat(temp," ");
			int toElev = elev->msgBC[j][3];
			int numElev = elev->msgBC[j][4];
			sprintf(a, "%d", toElev);
			strcat(temp,a);
			strcat(temp," ");
			sprintf(a, "%d", numElev);
			strcat(temp,a);
			break;
		//Alive
		case 12:
			strcat(temp,"12");
			strcat(temp," ");
			sprintf(a,"%d",fromElevator);
			strcat(temp,a);
			break;
		default:
			printf("com_createMsgBC(): Unvalid messageType...\n");
			return 1;
			break;
	}
	strcpy(msg,temp);
	return 0;
}
//Send all cases with data concerning the status of the elevator
void com_sendAll(elevators *elev){
	com_setMsgBC(1,elev->self,0,0,0,0,elev);
	com_setMsgBC(4,elev->self,0,0,0,0,elev);
	com_setMsgBC(5,elev->self,0,0,0,0,elev);
	com_setMsgBC(6,elev->self,0,0,0,0,elev);
}
//Set up socket to receive a message. Waits until a message is received, then read the message and put it into elevators elev
//Run as a thread while main program runs
void* com_readMsg(void *arg){
	elevators *elev = (elevators*)(arg); //TypeCast
	
	//Set up a UDP socket
	struct sockaddr_in my_addr, cli_addr;
	int socketUDP;
	socklen_t slen = sizeof(cli_addr);
	char buf[BUFLEN];
	
	if((socketUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){err("Failed to create UDP socket");}
	else{printf("A UDP socket has been successfully created...\n");}

	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//Bind the UDP socket
	if (bind(socketUDP, (struct sockaddr* ) &my_addr, sizeof(my_addr))==-1){
		printf("Server : bind() failed\n");
		err("bind");
	}else{printf("The UDP socket binded successfully...\n");}
	
	char msg[BUFLEN];
	int fromElevator;
	int toElevator;
	int messageType;
	while(elev->run){
		recvfrom(socketUDP, buf, BUFLEN, 0, (struct sockaddr*)&cli_addr, &slen);
		strcpy(msg, buf);
		char *p;
		p = strtok(msg," ");
		//int messageLength = atoi(p);
		p = strtok(NULL, " ");
		messageType = atoi(p);
		p = strtok(NULL, " ");	
		fromElevator = atoi(p);
		int i = 0;
		if(fromElevator != elev->self || messageType == 9 || messageType == 11){
			switch (messageType){
				//internalQueue
				case 1:
					p = strtok(NULL, " ");
					while (p != NULL){
						elev->internalQueue[fromElevator][i] = atoi(p);
						i++;
						p = strtok(NULL, " ");
					}
					break;
				//Status
				case 4:
					p = strtok(NULL," ");
					elev->direction[fromElevator] = atoi(p);
					p = strtok(NULL," ");
					elev->lastFloor[fromElevator] = atoi(p);
					p = strtok(NULL," ");
					elev->obs[fromElevator] = atoi(p);
					p = strtok(NULL," ");
					elev->stop[fromElevator] = atoi(p);
					p = strtok(NULL," ");
					elev->door[fromElevator] = atoi(p);
					p = strtok(NULL," ");
					elev->speed[fromElevator] = atoi(p);
					p = strtok(NULL," ");
					elev->currentFloor[fromElevator] = atoi(p);
					break;
				case 5:
					p = strtok(NULL, " ");
					while (p != NULL){
						elev->internalQueueUp[fromElevator][i] = atoi(p);
						i++;
						p = strtok(NULL, " ");
					}
					break;
				case 6:
					p = strtok(NULL, " ");
					while (p != NULL){
						elev->internalQueueDown[fromElevator][i] = atoi(p);
						i++;
						p = strtok(NULL, " ");
					}
					break;
				case 9:
					p = strtok(NULL, " ");
					toElevator = atoi(p);
					p = strtok(NULL, " ");
					int CMD = atoi(p);
					p = strtok(NULL, " ");
					int floor = atoi(p);
					p = strtok(NULL, " ");
					int set = atoi(p);
					if (CMD == 0){
						elev->internalQueueUp[toElevator][floor] = set;
					}
					else if(CMD == 1){
						elev->internalQueueDown[toElevator][floor] = set;
					}
					else if(CMD == 2 && elev->self != toElevator){
						elev->internalQueue[toElevator][floor] = set;
					}
					
					break;
				//RequestJoin
				case 10:
					if(elevators_lowestNumber(elev) == elev->self){
						com_setMsgBC(11,elev->self,0,elev->numberOfElevators,0,0,elev);
					}
					break;
				//ReplyJoin
				case 11:
					p = strtok(NULL, " ");
					toElevator = atoi(p);
					p = strtok(NULL, " ");
					int numElev = atoi(p);
					if(toElevator == elev->self){elev->self = numElev + 1;}
					elev->numberOfElevators = numElev + 1;
					break;
				//Alive
				case 12:
				    elev->testAlive[fromElevator] = 1;
				    p = strtok(NULL, " ");	
				    break;
				default:
					printf("Msg Error: Not a valid format...\n");
					break;
			}
		
		}
		elevators_print(*elev);
	}
	close(socketUDP);
	pthread_exit(arg);
}
