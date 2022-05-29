#include <stdio.h>
#include <string.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "args.h"

//FIRST PROGRAM: obtain passing arguments for the TCP/IP connection
// ftp://<user>:<password>@<host>/<url-path>

int parseArgs(char *url, args *args){
    //returns -1 if error, 0 if success
    char ftp[] = "ftp://";
    int length = strlen(url);
    int state = 0;
    int i = 0;
    int j = 0;

    while(i < length){
        switch(state){
            case 0:
            if(url[i] == ftp[i] && i < 5) break;
            else if(i == 5 && url[i] == ftp[i]) state = 1;
            else printf("Error: not passing ftp\n");
            break;

            case 1:
            if(url[i] == ':'){
                state = 2;
                j = 0;
            }
            else{
                args->user[j] = url[i];
                j++;
            }
            break;

            case 2:
            if(url[i] == '@'){
                state = 3;
                j = 0;
            }
            else{
                args->password[j] = url[i];
                j++;
            }
            break;

            case 3:
            if(url[i] == '/'){
                state = 4;
                j = 0;
            }
            else{
                args->host[j] = url[i];
                j++;
            }
            break;

            case 4:
            args->url_path[j] = url[i];
            j++;
            break;
        }
        i++;
    }
    //case of anonymous user - define user and pass with something random
    if(strcmp(args->user, "\0") == 0 || strcmp(args->password, "\0") == 0){
        strcpy(args->user, "anonymous");
        strcpy(args->password, "pass");
    }

    if(getIp(args->host, args) != 0){
        printf("Error getIp()\n");
        return -1;
    }

    if(getFileName(args) != 0){
        printf("Error getFileName()\n");
        return -1;
    }
    
    return 0;
    
}

int getIp(char *host, args *args){
    //returns -1 if error, 0 if success
    struct hostent *h;
    
    if((h = gethostbyname(host)) == NULL){
        herror("Error getting host name\n");
        return -1;
    }
    
    strcpy(args->host_name, h->h_name);
    strcpy(args->ip, inet_ntoa( *( ( struct in_addr *)h->h_addr) )); //32 bits Internet addr with net ordenation for dotted decimal notation 

    return 0;
}

int getFileName(args *args){
    //returns 0 if success
    char fullpath[256];

    strcpy(fullpath, args->url_path);
    
    char* aux = strtok(fullpath, "/");

    while(aux != NULL){
        strcpy(args->file_name, aux);
        aux = strtok(NULL, "/");
    }

    return 0;
}