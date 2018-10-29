//
// Created by masteryoda on 18-10-25.
//

#ifndef FTPSERVER_GLOBAL_H
#define FTPSERVER_GLOBAL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <regex.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define PORT 8080
#define MAX_CON 20                   // max num of server connection
#define BUF_LEN 1024                 // max command length
#define ROOT "/home/masteryoda/tmp"  // root directory
#define IP "127,0,0,1"               // local IP address


struct Client {
    int listenCon;                   // sockets for listening
    int commandCon;                  // connections for command
    int fileTransferCon;             // connections for file transfer
    int loginState;                  // state of login
    int renameState;                 // state of renaming file
    int mode;                        // 0 for no PORT or PASV, 1 for PORT, 2 for PASV
    int port_port;                   // port of PORT command
    int transferState;               // state of transfer, 0 for no transfer, 1 for retrieve, 2 for list, 3 for store, 4 for complete
    int port;                        // port
    int transferProcess;             // the process of file transfer
    DIR* dirPointer;                 // current directory
    struct stat dirState;            // state of directory
    char port_ip[BUF_LEN];           // ip of PORT command
    char dirName[BUF_LEN];           // directory that will be transfer through LIST
    char fileName[BUF_LEN];          // file name
    char directory[BUF_LEN];         // client's working directory
    char renamePath[BUF_LEN];        // directory of renaming file

};

struct Client clients[MAX_CON];      // array of all clients

fd_set fd_read;                      // backup for select
fd_set fd_select;                    // backup for fd_read

struct sockaddr_in serv_addr;        // address of server
struct sockaddr_in cli_addr;         // address of client

int socket_fd;                       // initial socket descriptor

int maxfd;                           // max num of file descriptor

char message[BUF_LEN];               // dynamic message

char data[BUF_LEN];                  // data to transfer

char* command[2];                    // parsed command

struct timeval timeout;              // select timeout

#endif //FTPSERVER_GLOBAL_H
