/*Non-Canonical Input Processing*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

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
    } /*


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

    //while (STOP==FALSE) {       /* loop for input */
      //res = read(fd,buf,8);   /* returns after 5 chars have been input */
      //buf[res]='\0';               /* so we can printf... */
      //printf(":%s:%d\n", buf, res);
      //if (buf[0]=='z') STOP=TRUE;
    //}
   char aux[255];
   //int i=0; 
   //CONSIDERANDO QUE O READ LÊ DE 8 EM 8 BYTES
   /*while(STOP == FALSE){
        res = read(fd, aux, 8);
        i += res;
        printf(":%s:%d\n", aux, res);
        strcat(buf, aux);
        if(aux[i-1] == '\0') STOP = TRUE;
    }
    res = i;
    */
   //CONSIDERANDO QUE O READ LÊ DE 1 EM 1 BYTE
   while (STOP == FALSE)
   {
     res += read(fd, aux, 1);
     printf(":%c:%d\n", aux[0], res);
     buf[res-1] = aux[0];
     if(aux[0] == '\0') STOP == TRUE;
   }
 
  //O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  int nr=0;
	unsigned int t = 0;
	unsigned int nr_char = 0;

	res = write(fd, buf, strlen(buf) + 1);
	printf("(%d bytes written)\n", res);
	res = 0;
  //i=0;
	bzero(buf,255); //limpar o char buf

  //CONSIDERANDO QUE O READ LÊ DE 8 EM 8 BYTES
  /*while(STOP == FALSE){
        res = read(fd, aux, 255);
        i += res;
        printf(":%s:%d\n", aux, res);
        strcat(buf, aux);
        if(aux[i-1] == '\0') STOP = TRUE;
    }
  */
 //CONSIDERANDO QUE O READ LÊ DE 1 EM 1 BYTE
  while (STOP == FALSE)
  {
    res += read(fd, aux, 1);
    buf[res-1] = aux[0];
    if(aux[0] == '\0') STOP == TRUE;
  }
  
	nr=write(fd,buf,res);
		
	printf("Mensagem reenviada: %s", buf);
	printf(" (%d bytes enviados)\n",nr);

	

  tcsetattr(fd,TCSANOW,&oldtio);
  close(fd);
  sleep(1);
  return 0;
}
