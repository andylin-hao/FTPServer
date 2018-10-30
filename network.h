//
// Created by masteryoda on 18-10-26.
//

#ifndef FTPSERVER_NETWORK_H
#define FTPSERVER_NETWORK_H

#include "global.h"
#include "utils.h"

int initServer(int argc, const char *argv[]);

void acceptConnection(int socketFd, const char* type, int index);

void disconnectCommand(int index);

void disconnectFileTransfer(int index);

void disconnectListen(int index);

void disconnect(int index);

void commandResponse(int index);

void fileTransferResponse(int index);

void dataResponse(int index);

int createListeningConn(int port, int index);

int connectToClient(int index);

#endif //FTPSERVER_NETWORK_H
