#ifndef LINKLAYER
#define LINKLAYER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct linkLayer{
    char serialPort[50];
    int role; //defines the role of the program: 0==Transmitter, 1=Receiver
    int baudRate;
    int numTries;
    int timeOut;
} linkLayer;

//ROLE
#define NOT_DEFINED -1
#define TRANSMITTER 0
#define RECEIVER 1


//SIZE of maximum acceptable payload; maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

//CONNECTION deafault values
#define BAUDRATE_DEFAULT B38400
#define MAX_RETRANSMISSIONS_DEFAULT 3
#define TIMEOUT_DEFAULT 4
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//MISC
#define FALSE 0
#define TRUE 1

// Opens a conection using the "port" parameters defined in struct linkLayer, returns "-1" on error and "1" on sucess
int llopen_trans(linkLayer connectionParameters);
int llopen_recev(linkLayer connectionParameters);
// Sends data in buf with size bufSize
int llwrite_trans(char* buf, int bufSize);
int write_recev_OK(); //write para quando o read no receiver correu bem, keep going
int write_recev_NOTOK(); //write para quando a mensagem o read no receiver não correu bem, request retransmisson 
// Receive data in packet
int llread_trans(char* packet);
int llread_recev(char* packet);
// Closes previously opened connection; if showStatistics==TRUE, link layer should print statistics in the console on close
int llclose(int showStatistics);


#endif



