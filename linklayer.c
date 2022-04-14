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
int tentat=0;
int retrans;
static int fd;
int Ns=1;
int Nr=1;
//statistics counters
int TOTALREAD_TRANS, TOTALWRITE_TRANS;
int TOTALREAD_REC, TOTALWRITE_REC;
int rej_count=0;
int rr_count=0;
int error_count=0;

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
    TIMEOUT = connectionParameters.timeOut;
    if(connectionParameters.role == 0) //transmitter
    {
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

        newtio.c_cc[VTIME]    = connectionParameters.timeOut*30;   /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

        tcflush(fd, TCIOFLUSH);

        if(tcsetattr(fd,TCSANOW,&newtio) == -1){
            perror("tcsetattr");
            exit(-1);
        }

        return 1;
    }
    else if(connectionParameters.role == 1) //receiver
    {
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
        newtio.c_cc[VTIME]    = 30;   /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */

        tcflush(fd, TCIOFLUSH);

        if(tcsetattr(fd,TCSANOW,&newtio) == -1){
            perror("tcsetattr");
            exit(-1);
        } 

        return 1;
    }
    else return -1;
}

int establishment_trans()
{
    int state = 0;
    int total, res; 
    unsigned char inicio[5];
    unsigned char aux;

    inicio[0] = FLAG;
    inicio[1] = A_TRANS;
    inicio[2] = SET;
    inicio[3] = A_TRANS^SET;
    inicio[4] = FLAG;

    if(tentat == TIMEOUT){
      llclose(TRUE);
      return -1;
    }

    (void) signal(SIGALRM, control_alarm);
    while(tentat < TIMEOUT && state != 5)
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

    if(total == 5 && state == 5){
        printf("TRANSMITTER READ CONFIRMED\n");
        (void) signal(SIGALRM, SIG_IGN);
        return 1;
    } 
    else return -1; 
}

unsigned char informationcheck()
{
    if(transmitter_information_read() == 1){
        Ns++;
        if(Ns % 2 == 0) return 0x00;
        else return 0x02;
    }
    else{
        if(Ns % 2 == 0) return 0x00;
        else return 0x02;
    }
}

int transmitter_information_write(char* buf, int bufSize)
{
    if(buf == NULL || bufSize > MAX_PAYLOAD_SIZE) return 0;

    int res;
    int state = 0;
    int total;
    unsigned char trama[2*bufSize + 7];
    unsigned char aux;
    int j = 3;
    //stuffing (FLAG -> ESC e FLAG^0x20 ; ESC -> ESC e ESC^0x20)

    trama[0] = FLAG;
    trama[1] = A_TRANS;
    trama[2] = informationcheck(buf, bufSize);
    for(int i=0; i<bufSize; i++)
    {
        BCC2_inicial ^= buf[i];
        if(buf[i] == FLAG){
            trama[j] = ESC;
            j++;
            trama[j] = FLAG^0x20;
        }
        else if(buf[i] == ESC){
            trama[j] = ESC;
            j++;
            trama[j] = FLAG^0x20;
        }
        else trama[j] = buf[i];
        j++;
    }

    if(BCC2_inicial == FLAG){
        trama[j] = ESC;
        j++;
        trama[j] = FLAG^0x20;
    }
    else if(BCC2_inicial == ESC){
        trama[j] = ESC;
        j++;
        trama[j] = ESC^0x20;
    }
    else trama[j] = BCC2_inicial;
    j++;
    trama[j] = FLAG;

    while(tentat < TIMEOUT)
    {
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
        }
        total = write(fd, trama, j+1);
        if(total != 0) break;
        else return 0;
    }

    return 1;
}

int transmitter_information_read()
{
    unsigned char aux;
    int state=0;
    int res, total;

    while(tentat < TIMEOUT && state != 7)
    {
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
                state = 0;
                total = 0;
            }
            break;

            case 2:
            if(aux == 0x01 || aux == 0x21){
                state = 4;
                retrans = 0;
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

            case 3:
            if(aux == 0x05 || aux == 0x25){
                state = 5;
                retrans = 1;
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
        if(retrans == 1) return 0;
        else{
            if(total != 0) return 1;
            else return 0;
        }
    }
}

int termination_trans()
{
    unsigned char aux;
    unsigned char trama[5];
    int state = 0;
    int res, total;

    if(tentat == TIMEOUT){
        llclose(TRUE);
        return -1;
    }

    trama[0] = FLAG;
    trama[1] = A_TRANS;
    trama[2] = DISC;
    trama[3] = A_TRANS^DISC;
    trama[4] = FLAG;

    (void) signal(SIGALRM, control_alarm);
    while(tentat < TIMEOUT && state != 6)
    {
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
            res = write(fd, trama, 5);
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
            if(res == 5) state = 6;
            break;
        }
    }

    (void) signal(SIGALRM, SIG_IGN);
    return 1;
}

int establishment_rec()
{
    int state = 0;
    int total, res; 
    unsigned char inicio[5];
    unsigned char aux;

    inicio[0] = FLAG;
    inicio[1] = A_REC;
    inicio[2] = UA;
    inicio[3] = A_REC^UA;
    inicio[4] = FLAG;

    if(tentat == TIMEOUT){
      llclose(TRUE);
      return -1;
    }

    while(tentat < TIMEOUT && state != 6)
    {   
        if(STOP == FALSE){
            alarm(3);
            STOP = TRUE;
        }

        res = read(fd, &aux, 1);
        total += res;

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
            if(aux == FLAG) state = 5;
            else{
                state = 0;
                total = 0;
            }
            break;

            case 5:
            printf("RECEIVER READ CONFIRMED\n");
            res = write(fd, inicio, 5);
            printf("UA: 0x%X | 0x%X | 0x%X | 0x%X | 0x%X\n", inicio[0], inicio[1], inicio[2], inicio[3], inicio[4]);
            if(res == 5) state = 6;
            break;
        }
    }

    if(total == 5 && state == 6) return 1;
    else return -1;
}

unsigned char confirmationcheck()
{
    if(transmitter_information_read() == 1){
        Nr++;
        if(BCC2_inicial == BCC2_final){
            if(Nr % 2 == 0) return 0x21;
            else return 0x01;
        } 
        else{
            if(Nr % 2 == 0) return 0x25;
            else return 0x05;
        }
    }
    else{
        if(BCC2_inicial == BCC2_final){
            if(Nr % 2 == 0) return 0x21;
            else return 0x01;
        } 
        else{
            if(Nr % 2 == 0) return 0x25;
            else return 0x05;
        }
    }
}

int receiver_information_read(char* packet)
{
    int state = 0;
    int status;
    int j = 3;
    int res, total;
    unsigned char aux;
    int packetSize = sizeof(packet);
    unsigned char trama[packetSize + 7];

    if(packet == NULL) return 0;

    while(tentat < TIMEOUT && state != 6)
    {
        res = read(fd, &aux, 1);
        total += res;
        switch(state){
            case 0:
            if(aux == FLAG){
                state = 1;
                trama[0] = aux;
            }
            else total = 0;
            break;

            case 1:
            if(aux == A_TRANS){
                trama[1] = aux;
                state = 2;
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

            case 2:
            if(aux == 0x00 || aux == 0x02){
                trama[2] = aux;
                state = 3;
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

            case 3:
            for(int i=0; i<packetSize; i++){
                if(aux == ESC) status = 1;
                else if(aux == FLAG^0x20 && status == 1){
                    trama[j] = FLAG;
                    j++;
                    status = 0;
                }
                else if(aux == ESC^0x20 && status == 1){
                    trama[j] = ESC;
                    j++;
                    status = 0;
                }
                else{
                    trama[j] = aux;
                    j++;
                }
                if(i != packetSize-1) res = read(fd, &aux, 1);
                total += res;
            }
            state = 4;
            break;

            case 4:
            for(int i=3; i<j-1; i++){
                BCC2_final ^= trama[i];
            }
            if(aux == BCC2_final){
                trama[j] = BCC2_final;
                j++;
                state = 5;
            }
            else if(aux == ESC){
                res = read(fd, &aux, 1);
                total += res;
                if(aux == FLAG^0x20 && BCC2_final == FLAG){
                    trama[j] = BCC2_final;
                    j++;
                    state = 5;
                }
                else if(aux == ESC^0x20 && BCC2_final == ESC){
                    trama[j] = BCC2_final;
                    j++;
                    state = 5;
                }
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

            case 5:
            if(aux == FLAG){
                trama[j] = aux;
                state = 6;
            }
            else{
                state = 0;
                total = 0;
            }
            break;
        }
    }

    return 1;
}

int receiver_information_write(char* packet)
{
    unsigned char trama[5];
    int total;

    trama[0] = FLAG;
    trama[1] = A_REC;
    trama[2] = confirmationcheck(packet);

    if(trama[2] == 0x25 || trama[2] == 0x05) rej_count++; //REJ sent
    else rr_count++; //RR sent

    trama[3] = trama[1]^trama[2];
    trama[4] = FLAG;

    if(packet == NULL) return 0;

    while(tentat < TIMEOUT){
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

    if(tentat == TIMEOUT){
        llclose(TRUE);
        return -1;
    }

    while(tentat < TIMEOUT && state != 6)
    {
        res = read(fd, &aux, 1);
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
            res = write(fd, trama, 5);
            if(res == 5) state = 6;
            break;
        }
    }
    
    (void) signal(SIGALRM, SIG_IGN);
    return 1;
}

int llwrite(char* buf, int bufSize)
{
    int totalwr, totalread;

    if(buf == NULL || bufSize > MAX_PAYLOAD_SIZE) return -1;

    if(tentat == TIMEOUT){
        llclose(TRUE);
        return -1;
    }

    (void) signal(SIGALRM, control_alarm);
    totalwr = transmitter_information_write(buf, bufSize);
    TOTALWRITE_TRANS += totalwr;

    if(totalwr == 0){
        error_count++;
        sleep(3);
    }

    totalread = transmitter_information_read();
    TOTALREAD_TRANS += totalread;

    if(totalread  == 0){
        error_count++;
        sleep(3);
    }
    else (void) signal(SIGALRM, SIG_IGN);

    return totalwr;
}

int llread(char* packet)
{
    int totalread, totalwr;
    if(packet == NULL) return -1;

    if(tentat == TIMEOUT){
        llclose(TRUE);
        return -1;
    }

    totalread = receiver_information_read(packet);
    TOTALREAD_REC += totalread;

    if(totalread == 0){
        error_count++;
        sleep(3);
    }

    totalwr = receiver_information_write(packet);
    TOTALWRITE_REC += totalwr;

    if(totalwr == 0){
        error_count++;
        sleep(3);
    }

    return totalread;
}

int llclose(int showStatistics)
{
    if(tcsetattr(fd,TCSANOW,&oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    if(showStatistics == TRUE){
        printf("STATISTICS OF TRANSMITTER:\n");
        printf("Number of frames sent: %d\n", TOTALWRITE_TRANS);
        printf("Number of frames received: %d\n", TOTALREAD_REC);
        printf("Number of frames confirmed: %d\n", rr_count);
        printf("Number of frames rejected: %d\n", rej_count);
        printf("Number of frames with errors: %d\n", error_count);
    }

    close(fd);
    return 1;
}