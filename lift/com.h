#ifndef COM_H
#define COM_H

#define true 1
#define false 0

#define BUFLEN 512
#define PORT 12004
#define PORTBC 12005

#include "elevators.h"

void err(char *s);

void SIGPIPE_handler(int sig_int);

void* com_sendBC(void* arg);

void com_setMsgBC(int msgType,int fromElevator,int toElevator,int cmd,int floor,int set,elevators *elev);

int com_createMsgBC(elevators *elev,int j,char *msg); 

void com_sendAll(elevators *elev);

void* com_readMsg(void* arg);

#endif
