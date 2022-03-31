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
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define FLAG 0x7E
#define A_SE 0x03
#define A_RC 0x01
#define C 0x03
#define BCC A_RC^C

int state=1;
int tentat=1;

volatile int STOP=FALSE;

void control_alarm(){
  STOP = TRUE;
  printf("TENT#1: %d\n", tentat);
  tentat++;
  return;
}


int main(int argc, char** argv)
{
  int fd,c, res;
  struct termios oldtio,newtio;
  char buf[255];

/*    if ( (argc < 2) || 

  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 

  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {

      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");

      exit(1);
      
      */
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

    (void) signal(SIGALRM, control_alarm); //controlar o alarme (inserir interrupçao no sistema)

    //while (STOP==FALSE) {       /* loop for input */

      //res = read(fd,buf,8);   /* returns after 5 chars have been input */

      //buf[res]='\0';               /* so we can printf... */

      //printf(":%s:%d\n", buf, res);

      //if (buf[0]=='z') STOP=TRUE;

    //}

  //CONSIDERANDO QUE O READ LÊ DE 1 EM 1 BYTE
  //nt index = 0;
  int nr = 0;
  int total = 0;
  /*while (STOP == FALSE)
  {
    res = read(fd, &buf[index], 1);
    printf(":%X:%d\n", buf[index], index+1);
    index++;
    if(index == 5) STOP = TRUE;
  }*/
  

  (void) signal(SIGALRM, control_alarm);
    while(tentat <= 3 && state != 6){
      if(STOP == FALSE){
        alarm(3);
        printf("3 SECONDS TIMEOUT MAKING...\n");
        STOP = TRUE;
      }
      nr = read(fd, &buf[i], 1);
      total += nr; 
      i++;
      switch (state)
      {
        case 1:
        if(buf[0] == FLAG) state = 2;
        else i--;
        break;

        case 2:
        if(buf[1] == A_SE) state = 3;
        else i--;
        break;

        case 3:
        if(buf[2] == C) state = 4;
        else i--;
        break;

        case 4:
        if(buf[3] == BCC) state = 5;
        else i--;
        break;

        case 5:
        if(buf[4] == FLAG) state = 6;
        else i--;
        break;
      }
    }

  //O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 

  /*int nr=0;
	unsigned int t = 0;
	unsigned int nr_char = 0;*/
  buf[0] = FLAG;
  buf[1] = A_RC;
  buf[2] = C;
  buf[3] = BCC;
  buf[4] = FLAG;
	res = write(fd,buf,total);
	printf("%d bytes written\n", res);
  printf("UA: 0X%2X:0X%2X:0X%2X:0X%2X:0X%2X\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
	res = 0;
  //i=0;
	bzero(buf,255); //limpar o char buf

  tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
  sleep(1);

  return 0;

}
