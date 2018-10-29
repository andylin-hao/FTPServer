//
// Created by masteryoda on 18-10-26.
//

#include "network.h"

int initServer() {
    if(chdir(ROOT) < 0){
        perror("Change directory fail");
        return 0;
    }
    int error;

    timeout.tv_usec = 0;
    timeout.tv_sec = 0;

    socklen_t serv_len;

    // create TCP socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("fail to create socket");
        return 0;
    }

    // configure local address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    serv_len = sizeof(serv_addr);

    // bind address and port with socket descriptor
    error = bind(socket_fd, (struct sockaddr *) &serv_addr, serv_len);
    if (error < 0) {
        perror("fail to bind");
        return 0;
    }

    // listen for connection
    error = listen(socket_fd, MAX_CON);
    if (error < 0) {
        perror("fail to listen");
        return 0;
    }

    // initialize connections array
    initClients();

    FD_ZERO(&fd_read);
    FD_SET(socket_fd, &fd_read);

    maxfd = socket_fd;

    return 1;
}

void acceptConnection(int socketFd, const char *type, int index) {
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
                char *msg = "220 Anonymous FTP server ready.\r\n";
                int num = (int) write(conn_fd, msg, (size_t) strlen(msg));
                if (num < strlen(msg)) {
                    perror("connect fail");
                }
                break;
            }
        }
    } else if (!strcmp(type, "fileTransferCon")) {
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
        clients[index].mode = 0;
        clients[index].transferProcess = 0;
        clients[index].transferState = 0;
        clients[index].dirPointer = NULL;
        clients[index].port_port = -1;

        memset(clients[index].port_ip, 0, sizeof(clients[index].port_ip));
        memset(clients[index].fileName, 0, sizeof(clients[index].fileName));
        memset(&clients[index].dirState, 0, sizeof(clients[index].dirState));
        memset(clients[index].dirName, 0, sizeof(clients[index].dirName));

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

    clients[index].loginState = 0;
    clients[index].renameState = 0;
    clients[index].port = 0;
    clients[index].transferProcess = 0;
    clients[index].dirPointer = NULL;
    clients[index].mode = 0;
    clients[index].port_port = -1;

    memset(clients[index].port_ip, 0, sizeof(clients[index].port_ip));
    memset(clients[index].fileName, 0, sizeof(clients[index].fileName));
    memset(clients[index].renamePath, 0, sizeof(clients[index].renamePath));
    memset(&clients[index].dirState, 0, sizeof(clients[index].dirState));
    memset(clients[index].dirName, 0, sizeof(clients[index].dirName));
    memset(clients[index].directory, 0, sizeof(clients[index].directory));
    strcpy(clients[index].directory, ROOT);
}

void commandResponse(int index) {
    if (FD_ISSET(clients[index].commandCon, &fd_select)) {
        char buf[BUF_LEN] = {0};
        int num = (int) read(clients[index].commandCon, buf, BUF_LEN);

        if (num > 0) {
            printf("receive message from client %d: %s\n", index, buf);
            char *msg = processMsg(buf, index);
            if (msg) {
                num = (int) write(clients[index].commandCon, msg, (size_t) strlen(msg));
                if (num < 0) {
                    perror("fail to write");
                }
            }
        } else if (num == 0) {
            // client disconnect
            disconnect(index);
        }
    }
}

void fileTransferResponse(int index) {
    if (FD_ISSET(clients[index].fileTransferCon, &fd_select) && clients[index].transferState == 3) {
        char buf[BUF_LEN] = {0};
        if (strcmp(clients[index].fileName, "") != 0) {
            int num = (int) read(clients[index].fileTransferCon, buf, BUF_LEN);
            if (num) {
                FILE *fp = fopen(clients[index].fileName, "a");
                fwrite(buf, 1, (size_t) num, fp);
                fclose(fp);
            } else {
                disconnectFileTransfer(index);
                return;
            }
        }
    }
}

void dataResponse(int index) {
    if (clients[index].transferState == 4) {
        char msg[] = "226 Data transfer complete\r\n";
        write(clients[index].commandCon, msg, strlen(msg));
        disconnectFileTransfer(index);
    } else if ((clients[index].transferState == 1 || clients[index].transferState == 2) && clients[index].fileTransferCon != -1) {
        char *data = generateData(1, index);
        if (data) {
            int num = (int) write(clients[index].fileTransferCon, data, (size_t) strlen(data));
            clients[index].transferProcess += num;
            if (num < strlen(data))
                perror("Data lost");
        }
    }
}

int createListeningConn(int port, int index) {
    disconnectListen(index);
    disconnectFileTransfer(index);

    int error;

    socklen_t serv_len;

    // create TCP socket
    int socket_ls = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ls < 0) {
        perror("fail to create socket");
        return 0;
    }

    // configure local address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t )port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    serv_len = sizeof(serv_addr);

    // bind address and port with socket descriptor
    error = bind(socket_ls, (struct sockaddr *) &serv_addr, serv_len);
    if (error < 0) {
        perror("fail to bind");
        return 0;
    }

    // listen for connection
    error = listen(socket_ls, 1);
    if (error < 0) {
        perror("fail to listen");
        return 0;
    }

    FD_SET(socket_ls, &fd_read);
    clients[index].listenCon = socket_ls;

    if (maxfd < socket_ls) {
        maxfd = socket_ls;
    }

    return 1;
}

int connectToClient(int index) {
    disconnectListen(index);
    disconnectFileTransfer(index);

    int error;

    socklen_t conn_len;

    // create TCP socket
    int socket_cl = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_cl < 0) {
        perror("fail to create socket");
        return 0;
    }

    // configure client address
    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons((uint16_t )clients[index].port_port);
    if(inet_pton(AF_INET, clients[index].port_ip, &cli_addr.sin_addr) <= 0) {
        printf("IP format error");
        return 0;
    }

    // connect to client
    if(connect(socket_cl, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
        printf("Fail connecting to client");
        return 0;
    }

    FD_SET(socket_cl, &fd_read);
    clients[index].fileTransferCon = socket_cl;

    if(maxfd < socket_cl)
        maxfd = socket_cl;

    return 1;
}

