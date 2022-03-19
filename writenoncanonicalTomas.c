/*Non-Canonical Input Processing*/

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
#define A 0x03
#define C 0x03
#define BCC A^C

int state=1;

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
     
    newtio.c_cc[VTIME]    = 30;   /* inter-character timer unused */
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

    //char str[MAX];
    char recebido[MAX];
    char aux;
    int count = 0, nr = 0;
    /*printf("Enter a string: ");
    fgets(str, MAX, stdin);
    count = strlen(str)+1;

    printf("String: %s\n", str);*/
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC;

    res = write(fd,buf,5);   
    printf("%d bytes written\n", res);

    int total=0;

   /* while(j<3){
      while (STOP == FALSE)
      {
        nr += read(fd, &aux, 1);
        recebido[nr-1] = aux;
        if(aux == '\0') STOP = TRUE;
      }   
      printf("Mensagem recebida: %s\n", recebido);
      printf("%d bytes recebidos\n", nr); 
    }*/

    switch (state)
    {
      case 1:
      nr = read(fd, &aux, 1);
      total += nr;
      if(aux == buf[0]){
        state = 2;
        break;
      }
      else{
        break;
      }

      case 2:
      nr = read(fd, &aux, 1);
      total += nr;
      if(aux == buf[0]){
        state = 2;
        break;
      }
      else if(aux == buf[1]){
        state = 3;
        break;
      }
      else{
        state = 1;
        break;
      }

      case 3:
      nr = read(fd, &aux, 1);
      total += nr;
      if(aux == buf[2]){
        state = 4;
        break;
      }
      else if(aux == buf[0]){
        state = 2;
        break;
      }
      else{
        state = 1;
        break;
      }

      case 4:
      nr = read(fd, &aux, 1);
      total += nr;
      if(aux == buf[3]){
        state = 5;
        break;
      }
      else if(aux == buf[0]){
        state = 2;
        break;
      }
      else{
        state = 1;
        break;
      }

      case 5:
      nr = read(fd, &aux, 1);
      total += nr;
      if(aux == buf[0]){
        state = 6;
        break;
      }
      else{
        state = 1;
        break;
      }

      case 6:
      break;
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
