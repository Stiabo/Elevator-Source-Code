README.txt

COMPILE AND RUN WITH:
make -f Makefile.mk
./lift n

THREADS:
com_readMsg_thr:
Waits for incomming messages, reads them and insert the data into the struct elevators

com_sendBC_thr:
Listen for new message in msgBC, create the message with com_createMsgBC(), and send it on broadcast 

backup_thr:
Send a signal to the backup terminal, to let the backup know it's still running. Also writes to backupFile.dat

elevCtrl_listen_thr:
Listen for any input on the command buttons, and updates the button lights 

sim_thr:
When a index in queueUp or queueDown is set, find the optimal elevator to send. The elevator is found by running a cost function based on the time to arrival and the length of the queues. When the best elevator is found, msgBC is set to send a CMD.

elevators_isAlive_thr:
Lets other elevators know that it's connected to the network, and detects when an elevator is connected or disconnected


MSGBC:
The msg buffer for sending messages stores information in an two dim int array.
Each index represent something spesific (true=1, false=0) (i = buffMsg number):
[i][0] = is there a message to send at this i
[i][1] = type of message to send
[i][2] = fromElevNumber

The rest of the field depends on message type:
x = dont care
message type->:         1      4       5       6       9          10          11          12
[i][3]                  x      x       x       x      to           x          to           x
[i][4]                  x      x       x       x      CMD          x           x           x
[i][5]                  x      x       x       x     floor         x           x           x
[i][6]                  x      x       x       x      set          x           x           x

Use the function com_setMsgBC() to set a message in the buffer. com_sendBC_thr will then send the message.
example: Send CMD:
com_setMsgBC(9,2,1,1,4,1,elev);

MSG FORMAT DURING TRANSMITION: 	
BCinternalQueue         = 1 ("1 fromElevator data[0] ... data[numberOfFloors]")
BCstatus		        = 4 ("4 fromElevator direction lastFloor obstuction stop door speed")
BCinternalqueueUp 		= 5	("5 fromElevator data[0] ... [numberOfFloors]")
BCinternalqueueDown		= 6	("6 fromElevator data[0] ... [numberOfFloors]")
BCcmd			        = 9 ("9 fromElevator toElevator cmd floor set")
BCrequestJoin		    = 10 ("10 fromElevator")
BCreply			        = 11 ("11 fromElevator toElevator numberOfElevators")
BCalive			        = 12 ("12 formElevator")


BACKUP:
The backup file stores data in backupfile.dat
Each line in the file represent a sesific data in the struct elevators,

The backupfile.dat uses the following format:
self
internalQueue
testButtonUp
testButtonDown
lastFloor
direction
currentFloor  
stop
internalQueueUp
internalQueueDown
