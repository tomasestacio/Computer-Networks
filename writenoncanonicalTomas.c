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
#define A 0x03
#define C 0x03
#define BCC A^C

int state=1;
int tentat=0;

volatile int STOP=FALSE;

//fazer funçao para controlar alarme -> signal, flag = false qnd alarme ativado

void control_alarm(){
  STOP = TRUE;
  tentat++;
  return;
}

int main(int argc, char** argv)
{
    int fd, c, res;
    struct termios oldtio,newtio;
    unsigned char buf[MAX];
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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



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

    unsigned char aux[MAX];
    /*printf("Enter a string: ");
    fgets(str, MAX, stdin);
    count = strlen(str)+1;

    printf("String: %s\n", str);*/
    buf[0] = FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC;
    buf[4] = FLAG;

    int nr = 0;
 
    res = write(fd,buf,5);  
    /*if(STOP == TRUE){
      alarm(3);
      tcflush(fd, TCIOFLUSH);
      if(tentat == 3){
        close(fd);
        return 0;
      }
    }*/

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

    while(tentat <= 3){
        switch (state)
        {   
          case 1:
          nr = read(fd, &aux[0], 1);
          total += nr;
          if(STOP == TRUE) state = 7;
          if(aux[0] == buf[0]) state = 2;
          else tcflush(fd, TCIOFLUSH);
      
          case 2:
          nr = read(fd, &aux[1], 1);
          total += nr;
          if(STOP == TRUE) state = 7;
          if(aux[1] == buf[1]) state = 3;
          else{
            tcflush(fd, TCIOFLUSH);
            state = 1;
          }
  
          case 3:
          nr = read(fd, &aux[2], 1);
          total += nr;
          if(STOP == TRUE) state = 7;
          if(aux[2] == buf[2]) state = 4;
          else{
            tcflush(fd, TCIOFLUSH);
            state = 1;
          }

          case 4:
          nr = read(fd, &aux[3], 1);
          total += nr;
          if(STOP == TRUE) state = 7;
          if(aux[3] == buf[3]) state = 5;
          else{
            tcflush(fd, TCIOFLUSH);
            state = 1;
          }

          case 5:
          nr = read(fd, &aux[4], 1);
          total += nr;
          if(STOP == TRUE) state = 7;
          if(aux[4] == buf[4]) state = 6;
          else{
            tcflush(fd, TCIOFLUSH);
            state = 1;
          }

          case 6:
          if(total == 5) break;
          else if(STOP == TRUE) state = 7;
          else{
            tcflush(fd, TCIOFLUSH);
            state = 1;
          }
        
          case 7:
          alarm(3);
          tcflush(fd, TCIOFLUSH);
          if(tentat == 3){
            close(fd);
            return 0;
          }
         }
    }
    
    printf("SET: %X:%X:%X:%X:%X\n", aux[0], aux[1], aux[2], aux[3], aux[4]);
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
