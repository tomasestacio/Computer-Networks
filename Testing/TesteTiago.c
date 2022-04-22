#include "linklayer.h"
#include <stdio.h>
int main(int argc, char *argv[]){
    
    if (argc < 3){
		printf("usage: progname /dev/ttySxx tx|rx \n");
		exit(1);
	}

	printf("%s %s\n", argv[1], argv[2]);
	fflush(stdout);

	

    if(strcmp(argv[2], "tx") == 0){ //tx mode
        printf("tx mode\n");
        struct linkLayer ll;
		sprintf(ll.serialPort, "%s", argv[1]);
		ll.role = TRANSMITTER;
		ll.baudRate = 9600;
		ll.numTries = 3;
		ll.timeOut = 3;

        if(llopen(ll)==-1) {
        		fprintf(stderr, "Could not initialize link layer connection\n");
        		exit(1);
   		}
		
		printf("connection opened\n");
		fflush(stdout);
		fflush(stderr);
		
		const int buf_size = MAX_PAYLOAD_SIZE-1;
		unsigned char buffer[buf_size+1];
		int write_result;
		
		buffer[0] = 1;

		FILE *f;
		f=fopen("ola.txt","r");
		int n;
		for(int i=1;(n=fgetc(f))!=EOF;i++) buffer[i]=n;
		
		/*for (int i = 0; i < buf_size; i++)
		printf("%d ", buffer[i]);*/
		

		write_result = llwrite(buffer, buf_size);
		printf("\n");
		fclose(f);

    }
    else if(strcmp(argv[2], "rx") == 0){ //rx mode
        printf("rx mode\n");
        struct linkLayer ll;
		sprintf(ll.serialPort, "%s", argv[1]);
		ll.role = RECEIVER;
		ll.baudRate = 9600;
		ll.numTries = 3;
		ll.timeOut = 3;

        llopen(ll);

		printf("connection opened\n");
		fflush(stdout);
		fflush(stderr);
		
		const int buf_size = MAX_PAYLOAD_SIZE-1;
		unsigned char buffer[buf_size+1];
		int write_result;
		//printf("ENTROU\n");
		llread(buffer);

    }
    else
        printf("bad parameters\n");

    return 0;
}
