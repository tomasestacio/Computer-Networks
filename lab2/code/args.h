#ifndef ARGS_H
#define ARGS_H

#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct args{
    char user[128];
    char password[128];
    char host[128];
    char url_path[128];
    char file_name[128];
    char host_name[128];
    char ip[128];
} args;

//gets url-path, username, password, Host_name, Ip from Host_name and File_name
int parseArgs(char *url, args *args);
//gets Ip address from hostname and gets hostname from function gethostname()
int getIp(char *host, args *args);
//gets file_name from url-path
int getFileName(args *args);

#endif