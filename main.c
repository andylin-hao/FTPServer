#include "global.h"
#include "network.h"
#include "utils.h"

void messageLoop() {
    while (1) {
        fd_select = fd_read;

        // select function that monitor all sockets
        int error = select(maxfd + 1, &fd_select, NULL, NULL, &timeout);
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
                char msg[] = "125 Data connection established\r\n";
                write(clients[i].commandCon, msg, (size_t) strlen(msg));
            }
        }

        // check all readable connections of file transfer
        for (int i = 0; i < MAX_CON; ++i) {
            fileTransferResponse(i);
        }

        // check all writable connections of data
        for (int i = 0; i < MAX_CON; ++i) {
            dataResponse(i);
        }
    }
}

int main() {
    if (!initServer())
        return 1;
    messageLoop();
    return 0;
}