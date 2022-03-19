/*Non-Canonical Input Processing*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#define MAX 255
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd, c, res;
    struct termios oldtio,newtio;
    char buf[MAX];
    int i, sum = 0, speed = 0;
    
    /*  if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
     
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    char str[MAX];
    char recebido[MAX];
    char aux[MAX];
    int count = 0, nr = 0;
    printf("Enter a string: ");
    fgets(str, MAX, stdin);
    count = strlen(str)+1;

    printf("String: %s\n", str);

    res = write(fd,str,count);   
    printf("%d bytes written\n", res);

    int j=0;
    double time_spent;
    clock_t begin = clock();
    time_spent = (double)(clock() - begin) / CLOCKS_PER_SEC;

    while(j<4){
      if(time_spent < 3.0){
        while (STOP == FALSE)
        {
          nr += read(fd, aux, 1);
          recebido[nr-1] = aux[0];
          if(aux[0] == '\0') STOP = TRUE;
        }   
        printf("Mensagem recebida: %s\n", recebido);
        printf("%d bytes recebidos\n", nr); 
        if(recebido != NULL) break;
      }
      else{
        alarm(3);
        j++;
      }
    }
  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */



   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}
