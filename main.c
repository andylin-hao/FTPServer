#include "global.h"
#include "network.h"
#include "utils.h"

int initServer() {
    if(chdir(ROOT) < 0){
        perror("Change directory fail");
        return 0;
    }
    int error;

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

void messageLoop() {
    while (1) {
        fd_select = fd_read;

        // select function that monitor all sockets
        int error = select(maxfd + 1, &fd_select, NULL, NULL, NULL);
        if(error < 0) {
            perror("fail to select");
            return;
        }

        if(FD_ISSET(socket_fd, &fd_select)) {
            acceptConnection(socket_fd, "commandCon", -1);
        }

        // check all readable connections of command
        for (int i = 0; i < MAX_CON; ++i) {
            commandResponse(i);
        }

        for (int i = 0; i < MAX_CON; ++i) {
            if (FD_ISSET(clients[i].listenCon, &fd_select)) {
                acceptConnection(clients[i].listenCon, "fileTransferCon", i);
                disconnectListen(i);
            }
        }

        // check all readable connections of file transfer
        for (int i = 0; i < MAX_CON; ++i) {
            fileTranserResponse(i);
        }
    }
}

int main() {
    if (!initServer())
        return 1;
    messageLoop();
    return 0;
}