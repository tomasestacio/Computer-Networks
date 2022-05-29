#ifndef CONNEC_H
#define CONNEC_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <arpa/inet.h>
#include "args.h"

//initiate the connection between client and server with the creation of a connection socket
int client_init(char *ip, int port, int *socketfd);
//client sends commands to socket to be received and read in server
int clientCommand(int socketfd, char * command);
//pasvMode command reception (saves IP and Port number)
int pasvMode(int socketfd, char *ip, int *port);
//reading the different commands responses from server to client
int readResponse(int socketfd, char* rd, size_t size);
//writing on socketfile the communication between client and server
int writeFile(int socketfd, char* filename);

#endif