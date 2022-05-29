#include "constants.h"
#include "linklayer.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    //unsigned char buf[MAX_PAYLOAD_SIZE];
    //int bufSize = MAX_PAYLOAD_SIZE;
    
    if(strcmp(argv[2], "tx") == 0){
		printf("tx mode\n");
        linkLayer connectionParameters;
        connectionParameters.baudRate = 9600;
        connectionParameters.role = 0;
        connectionParameters.numTries = 3;
        connectionParameters.timeOut = 1;
        sprintf(connectionParameters.serialPort, "%s", argv[1]);
        if(llopen(connectionParameters)==-1) {
        	fprintf(stderr, "Could not initialize link layer connection\n");
        	exit(1);
   		}
        printf("connection opened\n");
		fflush(stdout);
		fflush(stderr);
        if(establishment_trans() == 1) termination_trans();
    } 
    else if(strcmp(argv[2], "rx") == 0){
        printf("rx mode\n");
        linkLayer connectionParameters;
        connectionParameters.baudRate = 9600;
        connectionParameters.role = 1;
        connectionParameters.numTries = 3;
        connectionParameters.timeOut = 1;
        sprintf(connectionParameters.serialPort, "%s", argv[1]);
        if(llopen(connectionParameters)==-1) {
            fprintf(stderr, "Could not initialize link layer connection\n");
            exit(1);
        }
        printf("connection opened\n");
		fflush(stdout);
		fflush(stderr);
        if(establishment_rec() == 1) termination_rec();
    }
}
