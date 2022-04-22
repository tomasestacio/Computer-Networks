/*Non-Canonical Input Processing*/

#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#define MAX 255
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define A_SE 0x03
#define A_RC 0x01
#define C 0x03
#define BCC A_SE^C

int state=1;
int tentat=1;

volatile int STOP=FALSE;

//fazer funçao para controlar alarme -> signal, flag = false qnd alarme ativado

void control_alarm(){
  printf("TENTATIVAS: %d\n", tentat);
  tentat++;
  STOP = FALSE;
  return;
}

int main(int argc, char** argv)
{
    int fd, c, res;
    struct termios oldtio,newtio;
    unsigned char buf[MAX];
    int sum = 0, speed = 0;
    
   if ( (argc < 2) || 
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
     
    newtio.c_cc[VTIME]    = 30;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* non-blocking read */



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

    /*printf("Enter a string: ");
    fgets(str, MAX, stdin);
    count = strlen(str)+1;

    printf("String: %s\n", str);*/
    buf[0] = FLAG;
    buf[1] = A_SE;
    buf[2] = C;
    buf[3] = BCC;
    buf[4] = FLAG;

    int nr = 0;
 
    res = write(fd,buf,5);  

    printf("%d bytes written\n", res);
    printf("SET: 0X%X:0X%X:0X%X:0X%X:0X%X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);

    int total=0;
    int i=0;


    (void) signal(SIGALRM, control_alarm);

    if(tentat == 4){
      close(fd);
      return 0;
    }

    while(tentat <= 3 && state != 6){
      if(STOP == FALSE){
        alarm(3);
        printf("3 SECONDS TIMEOUT MAKING...\n");
        STOP = TRUE;
      }
      nr = read(fd, &buf[i], 1);
      total += nr; 
      i++;
      switch(state)
      {
          case 1:
          if(buf[0] == FLAG){
            state = 2;
            printf("BUF: %X\n", buf[0]);
          }
          else i--;
          break;

          case 2:
          if(buf[1] == A_RC){
            state = 3;
            printf("BUF: %X\n", buf[1]);
          }
          else i--;
          break;

          case 3:
          if(buf[2] == C){
            state = 4;
            printf("BUF: %X\n", buf[2]);
          }
          else i--;
          break;

          case 4:
          if(buf[3] == BCC){
            state = 5;
            printf("BUF: %X\n", buf[3]);
          }
          else i--;
          break;

          case 5:
          if(buf[4] == FLAG){
            state = 6;
            printf("BUF: %X\n", buf[4]);
          }
          else i--;
          break;
        }
    }

  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */

  bzero(buf,MAX); //limpar o char buf

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}

