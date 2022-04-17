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

struct termios oldtio, newtio;

int tx=0;
int rx=0; 
int tentat=0;
int RETRANS=0;
int status=0;
static int fd;
int Ns=0;
int Nr=1;
//statistics counters
int TOTALREAD_TRANS, TOTALWRITE_TRANS;
int TOTALREAD_REC, TOTALWRITE_REC;
int rej_count_trans=0;
int rej_count_rec=0;
int rr_count_trans=0;
int rr_count_rec=0;
int error_count=0;
int resent_read=1;
int resent_write=1;

int NUMTRIES;
int TIMEOUT;

unsigned char BCC2_inicial = 0x00;
unsigned char BCC2_final = 0x00;

volatile int STOP=FALSE;

void control_alarm(){
  printf("TENTATIVA: %d\n", tentat+1);
  tentat++;
  STOP = FALSE;
  return;
}

int get_baud(int baud)
{
    switch (baud) {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 460800:
        return B460800;
    case 500000:
        return B500000;
    case 576000:
        return B576000;
    case 921600:
        return B921600;
    case 1000000:
        return B1000000;
    case 1152000:
        return B1152000;
    case 1500000:
        return B1500000;
    case 2000000:
        return B2000000;
    case 2500000:
        return B2500000;
    case 3000000:
        return B3000000;
    case 3500000:
        return B3500000;
    case 4000000:
        return B4000000;
    default: 
        return -1;
    }
}

int llopen(linkLayer connectionParameters)
{
    int baudrate = get_baud(connectionParameters.baudRate);
    NUMTRIES = connectionParameters.numTries;
    TIMEOUT = connectionParameters.timeOut;
    if(connectionParameters.role == 0) //transmitter
    {
        tx = 1;
        fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );
    
        if(fd <0){
            perror(connectionParameters.serialPort);
            exit(-1);
        }

        if(tcgetattr(fd,&oldtio) == -1){ /* save current port settings */
            perror("tcgetattr");
            exit(-1);
        }

        bzero(&newtio, sizeof(newtio));

        newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;

        /* set input mode (non-canonical, no echo,...) */
        newtio.c_lflag = 0;

        newtio.c_cc[VTIME]    = connectionParameters.timeOut*10;   /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

        tcflush(fd, TCIOFLUSH);

        if(tcsetattr(fd,TCSANOW,&newtio) == -1){
            perror("tcsetattr");
            exit(-1);
        }

        int check = establishment_trans();
        if(check == 1) return 1;
        else return -1;
    }
    else if(connectionParameters.role == 1) //receiver
    {
        rx = 1;
        fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY );

        if(fd <0){
            perror(connectionParameters.serialPort);
            exit(-1);
        }

        if(tcgetattr(fd,&oldtio) == -1){ /* save current port settings */
            perror("tcgetattr");
            exit(-1);
        }

        bzero(&newtio, sizeof(newtio));

        newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;

        /* set input mode (non-canonical, no echo,...) */

        newtio.c_lflag = 0;
        newtio.c_cc[VTIME]    = connectionParameters.timeOut*10;   /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */

        tcflush(fd, TCIOFLUSH);

        if(tcsetattr(fd,TCSANOW,&newtio) == -1){
            perror("tcsetattr");
            exit(-1);
        } 

        int check = establishment_rec();
        if(check == 1) return 1;
        else return -1;
    }
    else return -1;
}

int establishment_trans()
{
    int state = 0;
    int total = 0;
    int res = 0; 
    unsigned char inicio[5];
    unsigned char aux;

    inicio[0] = FLAG;
    inicio[1] = A_TRANS;
    inicio[2] = SET;
    inicio[3] = A_TRANS^SET;
    inicio[4] = FLAG;

    (void) signal(SIGALRM, control_alarm);
    while(tentat < NUMTRIES && state != 5)
    {   
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
            res = write(fd, inicio, 5);
            printf("SET: 0x%X | 0x%X | 0x%X | 0x%X | 0x%X\n", inicio[0], inicio[1], inicio[2], inicio[3], inicio[4]);
        }
        res = read(fd, &aux, 1);
        total += res;

        switch(state){

            case 0:
            if(aux == FLAG) state = 1;
            else total = 0;
            break;

            case 1:
            if(aux == A_REC) state = 2;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0,
                total = 0;
            }
            break;

            case 2:
            if(aux == UA) state = 3;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;
            
            case 3:
            if(aux == A_REC^UA) state = 4;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 4:
            if(aux == FLAG) state = 5;
            else{
                state = 0;
                total = 0;
            }
            break;
        }
    }

    if(state == 5){
        (void) signal(SIGALRM, SIG_IGN);
        STOP = FALSE;
        tentat = 0;
        return 1;
    } 
    else return -1; 
}

unsigned char informationcheck()
{
    if(Ns == 0) return 0x00;
    else return 0x02;
}

int transmitter_information_write(char* buf, int bufSize)
{
    int res = 0;
    int state = 0;
    int total = 0;
    unsigned char trama[2*bufSize + 7];
    unsigned char aux;
    unsigned char BCC2 = 0x00;
    int j = 3;
    //stuffing (FLAG -> ESC e FLAG^0x20 ; ESC -> ESC e ESC^0x20)

    trama[0] = FLAG;
    trama[1] = A_TRANS;
    trama[2] = informationcheck(buf, bufSize);
    for(int i=0; i<bufSize; i++)
    {
        BCC2^=buf[i];
    }
    for(int i=0; i<bufSize; i++)
    {
        //BCC2 ^= buf[i];
        if(buf[i] == FLAG){
            trama[j] = ESC;
            j++;
            trama[j] = 0x5E;
        }
        else if(buf[i] == ESC){
            trama[j] = ESC;
            j++;
            trama[j] = 0x5D;
        }
        
        else trama[j] = buf[i];

        j++;
    }
    if(BCC2 == FLAG){
        trama[j] = ESC;
        j++;
        trama[j] = FLAG^0x20;
    }
    else if(BCC2 == ESC){
        trama[j] = ESC;
        j++;
        trama[j] = ESC^0x20;
    }
    else trama[j] = BCC2;
    j++;
    trama[j] = FLAG;

    while(tentat < NUMTRIES)
    {
        total = write(fd, trama, j+1);
        if(total != 0) break;
        else return 0;
    }

    return bufSize;
}

int transmitter_information_read()
{
    unsigned char aux;
    int state = 0;
    int res = 0;
    int total = 0;

    while(tentat < NUMTRIES && state != 7)
    {
        res = read(fd, &aux, 1);
        if(res == 0) return -1;
        total += res;
        switch(state){
            case 0:
            if(aux == FLAG) state = 1;
            else total = 0;
            break;

            case 1:
            if(aux == A_REC) state = 2;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            } 
            else{
                state = 0;
                total = 0;
            }
            break;

            case 2:
            if(aux == 0x01 || aux == 0x21) state = 4;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 3:
            if(aux == 0x05 || aux == 0x25){
                state = 5;
                RETRANS = 1;
            } 
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 4:
            if(aux == A_REC^0x01 || aux == A_REC^0x21) state = 6;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 5:
            if(aux == A_REC^0x05 || aux == A_REC^0x25) state = 6;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 6:
            if(aux == FLAG) state = 7;
            else{
                state = 0;
                total = 0;
            }
            break;
        }
        if(RETRANS == 1 && state == 7){
            RETRANS = 0;
            return 0;
        }
    }
    return 1;
}

int termination_trans()
{
    unsigned char aux;
    unsigned char trama[5];
    int state = 0;
    int res = 0;
    int total = 0;

    trama[0] = FLAG;
    trama[1] = A_TRANS;
    trama[2] = DISC;
    trama[3] = A_TRANS^DISC;
    trama[4] = FLAG;

    (void) signal(SIGALRM, control_alarm);
    while(tentat < NUMTRIES && state != 6)
    {
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
            res = write(fd, trama, 5);
            printf("DISC: 0x%X | 0x%X | 0x%X | 0x%X | 0x%X\n", trama[0], trama[1], trama[2], trama[3], trama[4]);
        }

        if(state != 5){
            res = read(fd, &aux, 1);
            total += res;
        }
        
        switch(state){
            case 0:
            if(aux == FLAG) state = 1;
            else total = 0;
            break;

            case 1:
            if(aux == A_REC) state = 2;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 2:
            if(aux == DISC) state = 3;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 3:
            if(aux == A_REC^DISC) state = 4;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 4:
            if(aux == FLAG) state = 5;
            else{
                state = 0;
                total = 0;
            }
            break;

            case 5:
            trama[2] = UA;
            trama[3] = A_TRANS^UA;
            res = write(fd, trama, 5);
            printf("UA: 0x%X | 0x%X | 0x%X | 0x%X | 0x%X\n", trama[0], trama[1], trama[2], trama[3], trama[4]);
            if(res == 5) state = 6;
            break;
        }
    }
    
    if(tentat == NUMTRIES) return -1;

    (void) signal(SIGALRM, SIG_IGN);
    STOP = FALSE;
    tentat = 0;
    return 1;
}

int establishment_rec()
{
    int state = 0;
    int total = 0;
    int res = 0; 
    unsigned char inicio[5];
    unsigned char aux;

    inicio[0] = FLAG;
    inicio[1] = A_REC;
    inicio[2] = UA;
    inicio[3] = A_REC^UA;
    inicio[4] = FLAG;

    (void) signal(SIGALRM, control_alarm);
    while(tentat < NUMTRIES && state != 6)
    {   
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
        }

        if(total != 5){
            res = read(fd, &aux, 1);
            total += res;
        }
        
        switch(state)
        {
            case 0:
            if(aux == FLAG) state = 1;
            else total = 0;
            break;

            case 1:
            if(aux == A_TRANS) state = 2;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 2:
            if(aux == SET) state = 3;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;
            
            case 3:
            if(aux == A_TRANS^SET) state = 4;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 4:
            if(aux == FLAG){
                state = 5;
            } 
            else{
                state = 0;
                total = 0;
            }
            break;

            case 5:
            res = write(fd, inicio, 5);
            printf("UA: 0x%X | 0x%X | 0x%X | 0x%X | 0x%X\n", inicio[0], inicio[1], inicio[2], inicio[3], inicio[4]);
            if(res == 5) state = 6;
            break;
        }
    }

    if(state == 6){
        (void) signal(SIGALRM, SIG_IGN); 
        STOP = FALSE;
        tentat = 0;
        return 1;
    }
    else return -1;
}

unsigned char confirmationcheck()
{
    if(BCC2_inicial == BCC2_final){
        if(Nr == 1) return 0x21;
        else return 0x01;
    } 
    else{
        if(Nr == 1) return 0x25;
        else return 0x05;
    }
}

int receiver_information_read(char* packet)
{
    int state = 0;
    int j = 0;
    int res = 0;
    int total = 0;
    unsigned char aux;

    BCC2_final = 0x00;
    BCC2_inicial = 0x00;

    while(tentat < NUMTRIES && state != 5)
    {
        res = read(fd, &aux, 1);
        if(res == 0) return -1;
        total += res;
        switch(state){
            case 0:
            if(aux == FLAG) state = 1;
            else total = 0;
            break;

            case 1:
            if(aux == A_TRANS) state = 2;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 2:
            if(aux == 0x00 || aux == 0x02) state = 3;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;
            
            case 3:
            if(aux == 0x1){
                state = 4;
                packet[j] = aux;
                j++;
            }
            else if(aux == 0x0){
                state = 5;
                packet[j] = aux;
                j++;
            }
            else if(aux == FLAG){
                state = 1;
                total = 1;
            } 
            else{
                state = 0;
                total = 0;
            }
            break;

            case 4:
            if(aux == ESC) status = 1;
            else if(aux == 0x5D && status == 1){
                status = 0;
                packet[j] = ESC;
                j++;
            }
            else if(aux == 0x5E && status == 1){
                status = 0;
                packet[j] = FLAG;
                j++;
            }
            else if(aux == FLAG) state = 5;
            else{
                packet[j] = aux;
                j++;
            }
            break;
        }
        if(state == 5){
            if(packet[0] == 0x1){
                BCC2_inicial = packet[j-1];
                j--; 
                for(int i = 0; i < j; i++){
                    BCC2_final ^= packet[i];
                }
            }
            else break;
        }
    }

    if(BCC2_inicial == BCC2_final) return j;
    else return 0;

}

int receiver_information_write(char* packet)
{
    unsigned char trama[5];
    int total;

    trama[0] = FLAG;
    trama[1] = A_REC;
    trama[2] = confirmationcheck(packet);
    trama[3] = trama[1]^trama[2];
    trama[4] = FLAG;

    while(tentat < NUMTRIES){
        total = write(fd, trama, 5);
        if(total == 5) break;
        else return 0;
    }

    return 1;
}

int termination_rec()
{
    unsigned char aux;
    unsigned char trama[5];
    int state = 0;
    int res, total;

    trama[0] = FLAG;
    trama[1] = A_REC;
    trama[2] = DISC;
    trama[3] = A_REC^DISC;
    trama[4] = FLAG;

    (void) signal(SIGALRM, control_alarm);
    while(tentat < NUMTRIES && state != 11)
    {
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
        }

        if(state != 5){
            res = read(fd, &aux, 1);
            total += res;
        }

        switch(state){
            case 0:
            if(aux == FLAG) state = 1;
            else total = 0;
            break;

            case 1:
            if(aux == A_TRANS) state = 2;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 2:
            if(aux == DISC) state = 3;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 3:
            if(aux == A_TRANS^DISC) state = 4;
            else if(aux == FLAG){
                state = 1;
                total = 1;
            }
            else{
                state = 0;
                total = 0;
            }
            break;

            case 4:
            if(aux == FLAG) state = 5;
            else{
                state = 0;
                total = 0;
            }
            break;

            case 5:
            total = 0;
            res = write(fd, trama, 5);
            printf("DISC: 0x%X | 0x%X | 0x%X | 0x%X | 0x%X\n", trama[0], trama[1], trama[2], trama[3], trama[4]);
            if(res == 5) state = 6;
            break;

            case 6:
            if(aux == FLAG) state = 7;
            else total = 0;
            break;

            case 7:
            if(aux == A_TRANS) state = 8;
            else if(aux == FLAG){
                state = 7;
                total = 1;
            }
            else{
                state = 6;
                total = 0;
            }
            break;

            case 8:
            if(aux == UA) state = 9;
            else if(aux == FLAG){
                state = 7;
                total = 1;
            }
            else{
                state = 6;
                total = 0;
            }
            break;

            case 9:
            if(aux == A_TRANS^UA) state = 10;
            else if(aux == FLAG){
                state = 7;
                total = 1;
            }
            else{
                state = 6;
                total = 0;
            }
            break;

            case 10:
            if(aux == FLAG) state = 11;
            else{
                state = 6;
                total = 0;
            }
            break;
        }
    }

    if(tentat == NUMTRIES) return -1;

    (void) signal(SIGALRM, SIG_IGN);
    STOP = FALSE;
    tentat = 0;
    return 1;
}

int llwrite(char* buf, int bufSize)
{
    int totalwr = 0;
    int totalread = 0;

    if(buf == NULL || bufSize > MAX_PAYLOAD_SIZE) return -1;

    (void) signal(SIGALRM, control_alarm);

    if(STOP == FALSE){
        alarm(3);
        STOP = TRUE;
    }

    totalwr = transmitter_information_write(buf, bufSize);  
    //printf("Transmitter write ok: (totalwr) %d\n", totalwr);  
    TOTALWRITE_TRANS += totalwr;
    if(STOP == FALSE){
        if(tentat == NUMTRIES){
            llclose(TRUE);
            return -1;
        }
        else{
            //sleep(1);
            return llwrite(buf, bufSize);
        }
    }
    
    totalread = transmitter_information_read();
    //printf("Transmitter read ok: (totalread) %d\n", totalread);  
    TOTALREAD_TRANS += totalread;
    if(STOP == FALSE && totalread == -1){
        if(tentat == NUMTRIES){
            llclose(TRUE);
            return -1;
        }
        else{
            resent_write++;
            //sleep(1);
            return llwrite(buf, bufSize);
        }
    }
    
    //REJ retrans
    if(totalread == 0){
        rej_count_trans++;
        (void) signal(SIGALRM, SIG_IGN);
        //sleep(1);
        return llwrite(buf, bufSize);
    }
    
    (void) signal(SIGALRM, SIG_IGN);
    rr_count_trans++;
    tentat = 0;
    STOP = FALSE;
    if(Ns == 0) Ns = 1;
    else if(Ns == 1) Ns = 0;

    return totalwr;

}

int llread(char* packet)
{
    int totalread = 0;
    int totalwr = 0;

    if(packet == NULL) return -1;

    (void) signal(SIGALRM, control_alarm);

    if(STOP == FALSE){
        alarm(3);
        STOP = TRUE;
    }
    totalread = receiver_information_read(packet);
    //printf("Receiver read ok: (totalread) %d\n", totalread);  
    TOTALREAD_REC += totalread;
    if(STOP == FALSE && totalread == -1){
        if(tentat == NUMTRIES){
            llclose(TRUE);
            return -1;
        }
        else{
            resent_read++;
            return llread(packet);
        }        
    }
    //REJ
    else if(totalread == 0){
        rej_count_rec++;
        (void) signal(SIGALRM, SIG_IGN);
        return llread(packet);
    }
    
    totalwr = receiver_information_write(packet);
    //printf("Receiver write ok: (totalwr) %d\n", totalwr);  
    TOTALWRITE_REC += totalwr;
    if(STOP == FALSE){
        if(tentat == NUMTRIES){
            llclose(TRUE);
            return -1;
        }
        else{
            return llread(packet);
        }        
    }

    (void) signal(SIGALRM, SIG_IGN);
    rr_count_rec++;
    tentat = 0;
    STOP = FALSE;
    if(Nr == 0) Nr = 1;
    else if(Nr == 1) Nr = 0;

    return totalread;

}

int llclose(int showStatistics)
{
    int check_tx, check_rx;

    if(tcsetattr(fd,TCSANOW,&oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    if(tx == 1) check_tx = termination_trans();
    else if(rx == 1) check_rx = termination_rec();

    if(check_tx == -1 || check_rx == -1){
        if(showStatistics == TRUE && tx == 1){
            printf("STATISTICS OF TRANSMITTER:\n");
            printf("Timeout defined: %d seconds\n", TIMEOUT);
            printf("Number of tries defined before closing: %d tries\n", NUMTRIES);
            printf("Number of bytes sent: %d bytes\n", TOTALWRITE_TRANS);
            printf("Number of frames confirmed: %d frames\n", rr_count_trans);
            printf("Number of frames rejected: %d frames\n", rej_count_trans);
            printf("Number of frames timed out: %d frames\n", resent_write);
        }
        else if(showStatistics == TRUE && rx == 1){
            printf("STATISTICS OF RECEIVER:\n");
            printf("Timeout: %d seconds\n", TIMEOUT);
            printf("Number of tries defined before closing: %d tries\n", NUMTRIES);
            printf("Number of bytes received (just the size of buffer): %d bytes\n", TOTALREAD_REC);
            printf("Number of frames confirmed: %d frames\n", rr_count_rec);
            printf("Number of frames rejected: %d frames\n", rej_count_rec);
            printf("Number of frames timed out: %d frames\n", resent_read);
        }

        close(fd);
        return -1;
    } 

    if(showStatistics == TRUE && tx == 1){
        printf("STATISTICS OF TRANSMITTER:\n");
        printf("Timeout defined: %d seconds\n", TIMEOUT);
        printf("Number of tries defined before closing: %d tries\n", NUMTRIES);
        printf("Number of bytes sent : %d bytes\n", TOTALWRITE_TRANS);
        printf("Number of frames confirmed: %d frames\n", rr_count_trans);
        printf("Number of frames rejected: %d frames\n", rej_count_trans);
        printf("Number of frames timed out: %d frames\n", resent_write);
    }
    else if(showStatistics == TRUE && rx == 1){
        printf("STATISTICS OF RECEIVER:\n");
        printf("Timeout defined: %d seconds\n", TIMEOUT);
        printf("Number of tries defined before closing: %d tries\n", NUMTRIES);
        printf("Number of bytes received (just the size of buffer): %d bytes\n", TOTALREAD_REC);
        printf("Number of frames confirmed: %d frames\n", rr_count_rec);
        printf("Number of frames rejected: %d frames\n", rej_count_rec);
        printf("Number of frames timed out: %d frames\n", resent_read);
    }
    //REMINDER: final nr of bytes is different from the size of the picture because the protocok is to add one byte to each frame transmitted7
    close(fd);
    return 1;
}
