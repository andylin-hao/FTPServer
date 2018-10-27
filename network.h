//
// Created by masteryoda on 18-10-26.
//

#ifndef FTPSERVER_NETWORK_H
#define FTPSERVER_NETWORK_H

#include "global.h"
#include "utils.h"

void acceptConnection(int socketFd, const char* type, int index);

void disconnectCommand(int index);

void disconnectFileTransfer(int index);

void disconnectListen(int index);

void disconnect(int index);

void commandResponse(int i);

void fileTranserResponse(int i);

#endif //FTPSERVER_NETWORK_H
