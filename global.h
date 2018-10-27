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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>

#define PORT 8080
#define MAX_CON 20                   // max num of server connection
#define BUF_LEN 1024                 // max command length
#define ROOT "/home/masteryoda/tmp"  //root directory


struct Client {
    int listenCon;                   // sockets for listening
    int commandCon;                  // connections for command
    int fileTransferCon;             // connections for file transfer
    char fileName[BUF_LEN];          // file names
    char directory[BUF_LEN];         // client's working directory
    int loginState;                  // state of login
    int renameState;                 // state of renaming file
    char renamePath[BUF_LEN];        // directory of renaming file
};

struct Client clients[MAX_CON];      // array of all clients

fd_set fd_read;                      // backup for select
fd_set fd_select;                    // backup for fd_read

struct sockaddr_in serv_addr;        // address of server
struct sockaddr_in cli_addr;         // address of client

int socket_fd;                       // initial socket descriptor

int maxfd;                           // max num of file descriptor

char message[BUF_LEN*BUF_LEN];       // dynamic message

#endif //FTPSERVER_GLOBAL_H
