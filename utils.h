//
// Created by masteryoda on 18-10-26.
//

#ifndef FTPSERVER_UTILS_H
#define FTPSERVER_UTILS_H

#include "global.h"

char** parseCommand(char* command);

char* processMsg(char* msg, int index);

char* generateData(int state, int index);

void upper(char* string);

int removeDir(const char *dir);

void initClients();

char* user(char* content, int index);

char* pass(char* content, int index);

char* retr(char* content, int index);

char* stor(char* content, int index);

char* quit(char* content, int index);

char* syst(char* content, int index);

char* type(char* content, int index);

char* port(char* content, int index);

char* pasv(char* content, int index);

char* mkd(char* content, int index);

char* cwd(char* content, int index);

char* pwd(char* content,  int index);

char* list(char* content, int index);

char* rmd(char* content, int index);

char* rnfr(char* content, int index);

char* rnto(char* content, int index);

char* sendFile(int index);

char* sendList(int index);

#endif //FTPSERVER_UTILS_H
