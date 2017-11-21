#ifndef BACKUP_H
#define BACKUP_H

#include "elevators.h"

void* backup_send();

void backup_receive(void);

void SIGINT_handler(int sig);

int backup_writeFile(elevators *elev);

int backup_readFile(elevators *elev);

#endif
