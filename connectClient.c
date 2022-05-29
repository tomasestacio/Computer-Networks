#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "connectClient.h"

//starting with initiating the connection in the client side - create a connection socket in server

int client_init(char *ip, int port, int *socketfd){
    //returns -1 if error, 0 if success;
    struct sockaddr_in server_addr;

    //server addr handling
    bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */

    //open TCP connection
    if((*socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket()");
        return -1;
    }

    //connect to server
    if(connect(*socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect()");
        return -1;
    }

    return 0;
}

int clientCommand(int socketfd, char * command){
    //sends command, returns -1 if error, 0 if closing connection, 1 if command other than closing the connection and well sent
    int size;

    if((size = write(socketfd, command, strlen(command))) <= 0){
        printf("Error: command not sent\n");
        return -1;
    }

    printf("< %s\n", command);
    return 0;
}

int pasvMode(int socketfd, char *ip, int *port){

    char buf[1024];

    if(readResponse(socketfd, buf, sizeof(buf)) == -1){
       printf("Error: cannot enter passive mode\n");
       return -1;
    }

    //< 227 Entering Passive Mode (193,136,28,12,19,91), example

    strtok(buf, "(");
    char* ip1 = strtok(NULL, ",");
    char* ip2 = strtok(NULL, ",");
    char* ip3 = strtok(NULL, ",");
    char* ip4 = strtok(NULL, ",");

    sprintf(ip, "%s.%s.%s.%s", ip1, ip2, ip3, ip4);

    char* aux1 = strtok(NULL, ",");
    char* aux2 = strtok(NULL, ")");

    *port = (256 * atoi(aux1)) + atoi(aux2);

    return 0;
}

int readResponse(int socketfd, char* rd, size_t size){

    //return 0 if success, -1 if some command is wrong (for different reasons)
    /*char *aux;
    read(socketfd, rd, size);
    printf("%s", rd);
    aux = strtok(rd, "-");
    long num = atoi(aux);
    if(num > 420 && num < 554){
        printf("Command error - «400» (command not accepted and requested action did not take place) or «500» (syntax error, command unrecognized and the requested action did not take place)\n");
        return -1;
    }

    return 0;*/
    char aux[3];
    long num;
    FILE* fd;

    if(!(fd = fdopen(socketfd, "r"))){
        printf("Error opening file to read\n");
        return -1;
    }
    
    do{
        memset(rd, 0, size);
        rd = fgets(rd, size, fd);
        printf("%s", rd);
    } while(rd[3] != ' ');

    strncpy(aux, rd, 3);
    num = atoi(aux);
    if(num > 420 && num < 554){
        printf("Command error - «400» (command not accepted and requested action did not take place) or «500» (syntax error, command unrecognized and the requested action did not take place)\n");
        return -1;
    }

    return 0;
}

int writeFile(int socketfd, char* filename){
    //returns -1 if error, 0 if success
    int bytes;
    char buf[1024];

    FILE* fd;

    if(!(fd = fopen(filename, "w"))){
        printf("Error opening file to write\n");
        return -1;
    }

    while((bytes = read(socketfd, buf, sizeof(bytes))) > 0){
        if(bytes < 0){
            printf("Error: Nothing received from data socket\n");
            return -1;
        }
        if((fwrite(buf, bytes, 1, fd)) < 0){
            printf("Error writting file in socketfile\n");
            return -1;
        }
    }

    fclose(fd);

    return 0;
}

