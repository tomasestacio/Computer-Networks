#define LINKLAYER

#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

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

//SET & UA
#define FLAG 0x7E
#define A_SE 0x03
#define A_RC 0x01
#define C 0x03
#define BCC1_SE A_SE^C
#define BCC1_RC A_RC^C
#define ESC 0x7D
#define SET 0x03
#define DISC 0x0B
#define UA 0x07

int tentat=1;
int fd;

volatile int STOP=FALSE;

void control_alarm(){
  //printf("TENTATIVA: %d\n", tentat);
  tentat++;
  STOP = FALSE;
  return;
}

int llopen_trans(linkLayer connectionParameters)
{
    unsigned char buf[5];
    int res;
    int state = 1;
    struct termios oldtio, newtio;

    if(connectionParameters.role == 0){
        fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
        if(fd < 0){
            perror(connectionParameters.serialPort);
            exit(-1);
        }
        if(tcgetattr(fd, &oldtio) == -1){
            perror("tcgetattr");
            exit(-1);
        }
        bzero(&newtio, sizeof(newtio));

        newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;

        newtio.c_lflag = 0;
        newtio.c_cc[VTIME] = 30;
        newtio.c_cc[VMIN] = 0;

        tcflush(fd, TCIOFLUSH);

        if(tcsetattr(fd,TCSANOW,&newtio) == -1){
            perror("tcsetattr");
            exit(-1);
        }

        buf[0] = FLAG;
        buf[1] = A_SE;
        buf[2] = C;
        buf[3] = BCC1_SE;
        buf[4] = FLAG;
    
        int total=0;
        int i=0;
        int nr=0;

        if(tentat == TIMEOUT_DEFAULT){
            close(fd);
            sleep(1);
            return -1;
        }

        (void) signal(SIGALRM, control_alarm);
        while(tentat <= MAX_RETRANSMISSIONS_DEFAULT && state != 6){
            if(STOP == FALSE){
                res = write(fd,buf,5);  
                printf("%d bytes written\n", res);
                printf("SET: 0X%X:0X%X:0X%X:0X%X:0X%X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
                alarm(3);
                STOP = TRUE;
            }
            nr = read(fd, &buf[i], 1);
            total += nr; 
            i++;
            switch (state)
            {
                case 1:
                if(buf[0] == FLAG) state = 2;
                else{
                    i=0;
                    total=0;
                }
                break;

                case 2:
                if(buf[1] == A_RC) state = 3;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;

                case 3:
                if(buf[2] == C) state = 4;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;

                case 4:
                if(buf[3] == BCC1_RC) state = 5;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;

                case 5:
                if(buf[4] == FLAG) state = 6;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;
            }
        }
        bzero(buf,5); //limpar o char buf

        if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
            perror("tcsetattr");
            exit(-1);
        }
        if(state == 6) (void) signal(SIGALRM, SIG_IGN); //ignora o signal
        close(fd);
        sleep(1);
        return 0;
    }
    else return -1;
}

int llopen_recev(linkLayer connectionParameters)
{
    unsigned char buf[MAX_PAYLOAD_SIZE];
    int res;
    int state = 1;
    struct termios oldtio, newtio;

    if(connectionParameters.role == 1){
        fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
        if(fd < 0){
            perror(connectionParameters.serialPort);
            exit(-1);
        }
        if(tcgetattr(fd, &oldtio) == -1){
            perror("tcgetattr");
            exit(-1);
        }
        bzero(&newtio, sizeof(newtio));

        newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;

        newtio.c_lflag = 0;
        newtio.c_cc[VTIME] = 30;
        newtio.c_cc[VMIN] = 0;

        tcflush(fd, TCIOFLUSH);

        if(tcsetattr(fd,TCSANOW,&newtio) == -1){
            perror("tcsetattr");
            exit(-1);
        }

        int total=0;
        int i=0;
        int nr=0;

        if(tentat == TIMEOUT_DEFAULT){
            close(fd);
            sleep(1);
            return -1;
        }
        (void) signal(SIGALRM, control_alarm);
        while(state != 6 && tentat <= MAX_RETRANSMISSIONS_DEFAULT){
            if(STOP == FALSE){
                alarm(3);
                STOP = TRUE;
            }
            nr = read(fd, &buf[i], 1);
            total += nr; 
            i++;
            switch (state)
            {
                case 1:
                if(buf[0] == FLAG) state = 2;
                else{
                    i=0;
                    total=0;
                }
                break;

                case 2:
                if(buf[1] == A_SE) state = 3;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;

                case 3:
                if(buf[2] == C) state = 4;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;

                case 4:
                if(buf[3] == BCC1_SE) state = 5;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;

                case 5:
                if(buf[4] == FLAG) state = 6;
                else{
                    i=0;
                    total=0;
                    state=1;
                }
                break;
            }   
        }

        buf[0] = FLAG;
        buf[1] = A_RC;
        buf[2] = C;
        buf[3] = BCC1_RC;
        buf[4] = FLAG;
	    res = write(fd,buf,total);
	    printf("%d bytes written\n", res);
        printf("UA: 0X%X:0X%X:0X%X:0X%X:0X%X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
	    res = 0;
        i = 0;
	    bzero(buf,5); //limpar o char buf
        if(state == 6) (void) signal(SIGALRM, SIG_IGN); //ignora o signal
        tcsetattr(fd,TCSANOW,&oldtio);
        close(fd);
        sleep(1);

        return 0;
    }
    else return -1;
}

int llwrite_trans(char* buf, int bufSize)
{
    unsigned char trama[MAX_PAYLOAD_SIZE];
    unsigned char BCC2_SE = 0x00;
    int Ns = 1;
    int j = 5;
    int total = 0;

    trama[0] = ESC;
    trama[1] = FLAG^0x20;
    trama[2] = A_SE;
    if(Ns % 2 == 0) trama[3] = 0x02;
    else trama[3] = 0x00;
    Ns++;
    trama[4] = BCC1_SE;
    for(int i=0; i<bufSize; i++)
    {
        BCC2_SE ^= buf[i];
        if(buf[i] == FLAG){
            trama[j] = ESC;
            j++;
            trama[j] = FLAG^0x20;
        } 
        else if(buf[i] == ESC){
            trama[j] = ESC;
            j++;
            trama[j] = ESC^0x20;
        }
        else{
            trama[j] = buf[i];
        }
        j++;
    }
    trama[j] = BCC2_SE;
    trama[j+1] = ESC;
    trama[j+2] = FLAG^0x20;

    if(tentat == TIMEOUT_DEFAULT){
        close(fd);
        sleep(1);
        return -1;
    }

    (void) signal(SIGALRM, control_alarm);
    while(tentat <= MAX_RETRANSMISSIONS_DEFAULT){
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
            total = write(fd, trama, j+3);
        }
        break;
    }
    if(total != 0) (void) signal(SIGALRM, SIG_IGN);

    return total;
}

int write_recev_OK()
{
    unsigned char trama[9];
    unsigned char BCC2_RC = 0x00;
    int Nr = 1;
    int j = 9;
    int total = 0;

    trama[0] = FLAG;
    trama[1] = A_RC;
    trama[2] = SET;
    trama[3] = DISC;
    trama[4] = UA;
    if(Nr % 2 == 0){
        trama[5] = 0x01;
        trama[6] = 0x05;
    }
    else{
        trama[5] = 0x21;
        trama[6] = 0x25;
    }
    Nr++;
    trama[7] = BCC1_RC;
    trama[8] = FLAG;

    if(tentat == TIMEOUT_DEFAULT){
        close(fd);
        sleep(1);
        return -1;
    }

    (void) signal(SIGALRM, control_alarm);
    while(tentat <= MAX_RETRANSMISSIONS_DEFAULT){
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
            total = write(fd, trama, 9);
        }
        break;
    }
    if(total != 0) (void) signal(SIGALRM, SIG_IGN);

    return total;
}

int write_recev_NOTOK()
{
    unsigned char trama[9];
    unsigned char BCC2_RC = 0x00;
    int Nr = 1;
    int j = 9;
    int total = 0;

    trama[0] = FLAG;
    trama[1] = A_RC;
    trama[2] = SET;
    trama[3] = DISC;
    trama[4] = UA;
    if(Nr % 2 == 0){
        trama[5] = 0x21;
        trama[6] = 0x25;
    }
    else{
        trama[5] = 0x01;
        trama[6] = 0x05;
    }
    Nr++;
    trama[7] = BCC1_RC;
    trama[8] = FLAG;

    if(tentat == TIMEOUT_DEFAULT){
        close(fd);
        sleep(1);
        return -1;
    }

    (void) signal(SIGALRM, control_alarm);
    while(tentat <= MAX_RETRANSMISSIONS_DEFAULT){
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
            total = write(fd, trama, 9);
        }
        break;
    }
    if(total != 0) (void) signal(SIGALRM, SIG_IGN);

    return total;
}

int llread_trans(char* packet)
{
    //if FLAG occurs inside a frame, substitui-se FLAG por 0x7d 0x5e (ESC e exclusive OR entre FLAG e 0x20)
    //if ESC occurs inside a frame, substitui-se ESC por 0x7d 0x5d (ESC e exclusive OR entre ESC e 0x20)
    //BCC só considera os octets originais sem stuffings (ou seja, estas substituições) ->
    //-> temos que realizar o cálculo do BCC antes do stuffing
    //BCC verification is performed on the original octets -> after destuffing
    unsigned char leitura[MAX_PAYLOAD_SIZE];
    int fd;
    unsigned char BCC2_SE = 0x00;
    int par = 1;
    int i = 0;
    int Nr = 1;
    int total = 0;
    int state = 1;

    if(tentat == TIMEOUT_DEFAULT){
        close(fd);
        sleep(1);
        return -1;
    }

    (void) signal(SIGALRM, control_alarm);

    while(packet[i] != '\0')
    {
        BCC2_SE ^= packet[i];
        i++;
    }
    i=0;

    while(tentat <= MAX_RETRANSMISSIONS_DEFAULT){
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
        }
        nr = read(fd, &leitura[i], 1);
        total += nr;
        i++;
        switch(state){
            case 1:
            if(leitura[0] == FLAG){

            }
        }
    }
}
