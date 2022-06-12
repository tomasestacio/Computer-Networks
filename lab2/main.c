#include "main.h"

int main(int argc, char **argv){

    if(argc != 2){
        fprintf(stderr, "usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(0);
    }

    args arguments;
    int socketfd_data, socketfd_control;
    char urlcpy[256];
    char command[256];
    char rd[1024];

    strcpy(urlcpy, argv[1]);
    
    memset(arguments.file_name, 0, 128);
    memset(arguments.host, 0, 128);
    memset(arguments.host_name, 0, 128);
    memset(arguments.ip, 0, 128);
    memset(arguments.password, 0, 128);
    memset(arguments.url_path, 0, 128);
    memset(arguments.user, 0, 128);

    if(parseArgs(urlcpy, &arguments) == -1){
        printf("usage: %s ftp://<user>:<password>@<host>/<url-path>\n", argv[0]);
        return -1;
    }
  
    printf("host: %s\nurl path: %s\nuser: %s\npassword: %s\nfile name: %s\nhost name: %s\nip addr: %s\n", arguments.host, arguments.url_path, arguments.user, arguments.password, arguments.file_name, arguments.host_name, arguments.ip);

    //TCP port = 21
    if(client_init(arguments.ip, 21, &socketfd_control) == -1){
        printf("Error: init() control\n");
        return -1;
    }

    if(readResponse(socketfd_control, rd, sizeof(rd)) == -1) return -1;

    sprintf(command, "user %s\r\n", arguments.user);

    if(clientCommand(socketfd_control, command) == -1) return -1;
    if(readResponse(socketfd_control, rd, sizeof(rd)) == -1) return -1;

    sprintf(command, "pass %s\r\n", arguments.password);

    if(clientCommand(socketfd_control, command) == -1) return -1;
    if(readResponse(socketfd_control, rd, sizeof(rd)) == -1) return -1;

    char ip[32];
    int port;

    sprintf(command, "pasv\r\n");
    if(clientCommand(socketfd_control, command) == -1) return -1;
    if(pasvMode(socketfd_control, ip, &port)) return -1;

    printf("IP: %s\nPort Number: %d\n", ip, port);

    if((client_init(ip, port, &socketfd_data)) == -1){
        printf("Error: init() data\n");
        return -1;
    }

    sprintf(command, "retr %s\r\n", arguments.url_path);
    if(clientCommand(socketfd_control, command) == -1) return -1; 
    if(readResponse(socketfd_control, rd, sizeof(rd)) == -1) return -1;

    if(writeFile(socketfd_data, arguments.file_name) == -1) return -1;
    
    if(readResponse(socketfd_control, rd, sizeof(rd)) == -1) return -1;

    sprintf(command, "quit\r\n");
    if(clientCommand(socketfd_control, command) == -1) return -1;
    if(readResponse(socketfd_control, rd, sizeof(rd)) == -1) return -1;
    
    close(socketfd_control);
    close(socketfd_data);

    return 0;
}
