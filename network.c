//
// Created by masteryoda on 18-10-26.
//

#include "network.h"

void acceptConnection(int socketFd, const char* type, int index) {
    socklen_t clientLen = sizeof(cli_addr);

    // extract connected socket
    int conn_fd = accept(socketFd, (struct sockaddr *) &cli_addr, &clientLen);
    if (conn_fd < 0) {
        perror("fail to accept");
        exit(1);
    }

    char cli_ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &cli_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);
    printf("----------------------------------------------\n");
    printf("client ip=%s,port=%d\n", cli_ip, ntohs(cli_addr.sin_port));

    if (!strcmp(type, "commandCon")) {
        for (int i = 0; i < MAX_CON; i++) {
            if (clients[i].commandCon != -1) {
                continue;
            } else {
                clients[i].commandCon = conn_fd;
                printf("client %d join\n", i);
                char* msg = "220 Anonymous FTP server ready.\r\n";
                int num = (int) write(conn_fd, msg, (size_t) strlen(msg));
                if(num < strlen(msg)) {
                    perror("connect fail");
                }
                break;
            }
        }
    } else if(!strcmp(type, "fileTransferCon")){
        clients[index].fileTransferCon = conn_fd;
        printf("client %d establish data connection\n", index);
    }

    FD_SET(conn_fd, &fd_read);

    if (maxfd < conn_fd) {
        maxfd = conn_fd;
    }
}

void disconnectCommand(int index) {
    if (clients[index].commandCon != -1) {
        FD_CLR(clients[index].commandCon, &fd_read);
        close(clients[index].commandCon);
        clients[index].commandCon = -1;
    }
}

void disconnectFileTransfer(int index) {
    if (clients[index].fileTransferCon != -1) {
        FD_CLR(clients[index].fileTransferCon, &fd_read);
        close(clients[index].fileTransferCon);
        clients[index].fileTransferCon = -1;
    }
}

void disconnectListen(int index) {
    if (clients[index].listenCon != -1) {
        FD_CLR(clients[index].listenCon, &fd_read);
        close(clients[index].listenCon);
        clients[index].listenCon = -1;
    }
}

void disconnect(int index) {
    printf("client %d exit\n", index);

    disconnectCommand(index);
    disconnectFileTransfer(index);
    disconnectListen(index);
}

void commandResponse(int i) {
    if (FD_ISSET(clients[i].commandCon, &fd_select)) {
        printf("client %d is ready to read", i);

        char buf[BUF_LEN] = {0};
        int num = (int) read(clients[i].commandCon, buf, BUF_LEN);

        if (num > 0) {
            printf("receive message from client %d: %s\n", i, buf);
            char* msg = processMsg(buf, i);
            if(msg) {
                num = (int) write(clients[i].commandCon, msg, (size_t) strlen(msg));
                if (num < 0) {
                    perror("fail to write");
                }
            }
        } else if(num == 0) {
            // client disconnect
            disconnect(i);
        }
    }
}

void fileTranserResponse(int i) {
    if(FD_ISSET(clients[i].fileTransferCon, &fd_select)) {
        printf("client %d is ready to transfer file", i);

        char buf[BUF_LEN] = {0};
        if(strcmp(clients[i].fileName, "") != 0) {
            int num = (int) read(clients[i].fileTransferCon, buf, BUF_LEN);
            if (num) {
                FILE* fp = fopen(clients[i].fileName, "a");
                fwrite(buf, 1, (size_t) num, fp);
                fclose(fp);
            } else {
                disconnectFileTransfer(i);
                return;
            }
        }
    }
}