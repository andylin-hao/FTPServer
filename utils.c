//
// Created by masteryoda on 18-10-26.
//

#include "utils.h"
#include "network.h"

void initClients() {
    for (int i = 0; i < MAX_CON; ++i) {
        clients[i].commandCon = -1;
        clients[i].fileTransferCon = -1;
        clients[i].listenCon = -1;
        memset(clients[i].fileName, 0, sizeof(clients[i].fileName));
        clients[i].loginState = 0;
        clients[i].renameState = 0;
        clients[i].port = 0;
        memset(clients[i].directory, 0, sizeof(clients[i].directory));
        strcpy(clients[i].directory, ROOT);
        memset(clients[i].renamePath, 0, sizeof(clients[i].renamePath));
        clients[i].dirPointer = NULL;
        memset(&clients[i].dirState, 0, sizeof(clients[i].dirState));
        memset(clients[i].dirName, 0, sizeof(clients[i].dirName));
    }
}

char *processMsg(char *msg, int index) {
    char **command = parseCommand(msg);

    if (!command[0])
        return "500 Please input correct command\r\n";

    upper(command[0]);

    chdir(clients[index].directory);

    if (!strcmp(command[0], "USER"))
        return user(command[1], index);

    if (!strcmp(command[0], "PASS"))
        return pass(command[1], index);

    if (!strcmp(command[0], "RETR"))
        return retr(command[1], index);

    if (!strcmp(command[0], "STOR"))
        return stor(command[1], index);

    if (!strcmp(command[0], "QUIT") || !strcmp(command[0], "ABOR"))
        return quit(command[1], index);

    if (!strcmp(command[0], "SYST"))
        return syst(command[1], index);

    if (!strcmp(command[0], "TYPE"))
        return type(command[1], index);

    if (!strcmp(command[0], "PORT"))
        return port(command[1], index);

    if (!strcmp(command[0], "PASV"))
        return pasv(command[1], index);

    if (!strcmp(command[0], "MKD"))
        return mkd(command[1], index);

    if (!strcmp(command[0], "CWD"))
        return cwd(command[1], index);

    if (!strcmp(command[0], "PWD"))
        return pwd(command[1], index);

    if (!strcmp(command[0], "LIST"))
        return list(command[1], index);

    if (!strcmp(command[0], "RMD"))
        return rmd(command[1], index);

    if (!strcmp(command[0], "RNFR"))
        return rnfr(command[1], index);

    if (!strcmp(command[0], "RNTO"))
        return rnto(command[1], index);

    else
        return "500 Please input correct command\r\n";
}

char *generateData(int state, int index) {
    switch (state) {
        case 1:
            return sendFile(index);
        case 2:
            return sendList(index);
        default:
            clients[index].transferState = 0;
            disconnectFileTransfer(index);
            return "426 Data connection accidentally broke";
    }
}

char *user(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    if (!strcmp(content, "anonymous") && clients[index].loginState < 2) {
        clients[index].loginState = 1;
        return "331 Guest login ok, send your complete e-mail address as password.\r\n";
    } else if (strcmp(content, "anonymous") != 0) {
        return "530 User name incorrect\r\n";
    } else {
        return "550 Unknown error\r\n";
    }
}

char *pass(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";

    char *pattern = ".+@.+..+";
    regex_t reg;
    regmatch_t pmatch[1];
    regcomp(&reg, pattern, REG_EXTENDED);
    int result = regexec(&reg, content, 1, pmatch, 0);

    if (!result && clients[index].loginState == 1) {
        clients[index].loginState = 2;
        return "230 Login ok\r\n";
    } else if (!result && clients[index].loginState == 2) {
        return "202 Already logged in\r\n";
    } else if (!result && clients[index].loginState == 0) {
        return "503 Please input USER first\r\n";
    } else if (result) {
        return "530 Password incorrect\r\n";
    } else {
        return "550 Unknown error\r\n";
    }
}

char *retr(char *content, int index) {
    return NULL;
}

char *stor(char *content, int index) {
    return NULL;
}

char *quit(char *content, int index) {
    char *msg = "221 Bye\r\n";
    int num = (int) write(clients[index].commandCon, msg, strlen(msg));
    if (num < 0)
        perror("fail to write\r\n");
    disconnect(index);
    return NULL;
}

char *syst(char *content, int index) {
    return "215 UNIX Type: L8\r\n";
}

char *type(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    if (!strcmp(content, "I"))
        return "200 Type set to I\r\n.";
    else
        return "501 Please input correct command argument\r\n";
}

char *port(char *content, int index) {
    return NULL;
}

char *pasv(char *content, int index) {
    if (content)
        return "501 Arguments are prohibited\r\n";
    else {
        memset(message, 0, sizeof(message));
        strcpy(message, "227 ");
        strcat(message, IP);
        srand((unsigned int) time(0));
        int port = (int) (random() % (65535 - 20000 + 1) + 20000);
        int i = 0;
        while (i < MAX_CON) {
            if(port == clients[i].port){
                port = (int) (random() % (65535 - 20000 + 1) + 20000);
                i = 0;
                continue;
            } else
                i++;
        }
        char p1[BUF_LEN] = {0};
        char p2[BUF_LEN] = {0};
        sprintf(p1, "%d", port/256);
        sprintf(p2, "%d", port%256);
        strcat(message, ",");
        strcat(message, p1);
        strcat(message, ",");
        strcat(message, p2);
        strcat(message, "\r\n");

        int result = createListeningConn(port, index);
        if (!result)
            return "421 Unable to listen for data connection";
        else
            return message;
    }
}

char *mkd(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {

        int error = mkdir(content, 0755);
        if (error < 0)
            return "550 Directory creation failed\r\n";
        else {
            char *directory = (char *) malloc(1024 * sizeof(char));
            memset(message, 0, sizeof(message));
            chdir(content);
            getcwd(directory, 1024);
            strcpy(message, "250 Directory");
            strcat(message, directory);
            strcat(message, " created");
            strcat(message, "\r\n");
            free(directory);
            return message;
        }
    }
}

char *cwd(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {

        int error = chdir(content);
        if (error < 0)
            return "550 No such file or directory\r\n";
        else {
            memset(clients[index].directory, 0, sizeof(clients[index].directory));
            getcwd(clients[index].directory, BUF_LEN);
            return "250 Working directory has been changed\r\n";
        }
    }
}

char *pwd(char *content, int index) {
    memset(message, 0, sizeof(message));
    strcpy(message, "257 \"");
    strcat(message, clients[index].directory);
    strcat(message, "\"");
    strcat(message, "\r\n");
    return message;
}

char *list(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        if (access(content, F_OK)) {
            clients[index].transferState = 0;
            disconnectFileTransfer(index);
            return "450 No such file or directory";
        } else {
            clients[index].transferState = 2;
            if (stat(content, &clients[index].dirState) < 0) {
                clients[index].transferState = 0;
                disconnectFileTransfer(index);
                return "450 Directory or file can't be accessed";
            } else {
                if (S_ISREG(clients[index].dirState.st_mode)) {
                    clients[index].dirPointer = NULL;
                    realpath(content, clients[index].dirName);
                    if (clients[index].fileTransferCon == -1)
                        return "126 File okay, waiting for data connection";
                    else
                        return "126 File okay";
                } else if (S_ISDIR(clients[index].dirState.st_mode)) {
                    clients[index].dirPointer = opendir(content);
                    realpath(content, clients[index].dirName);
                    if (clients[index].fileTransferCon == -1)
                        return "126 Directory okay, waiting for data connection";
                    else
                        return "126 Directory okay";
                } else {
                    clients[index].transferState = 0;
                    disconnectFileTransfer(index);
                    return "450 Unknown file type";
                }
            }
        }
    }
}

char *rmd(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        int error = removeDir(content);
        if (error < 0)
            return "550 Fail to remove directory\r\n";
        else
            return "250 Successfully remove directory\r\n";
    }
}

char *rnfr(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        if (access(content, F_OK))
            return "550 File does not exist\r\n";
        else if (access(content, R_OK | W_OK))
            return "550 Permission denied\r\n";
        else {
            memset(clients[index].renamePath, 0, sizeof(clients[index].renamePath));
            realpath(content, clients[index].renamePath);
            clients[index].renameState = 1;
            return "350 File is ready\r\n";
        }
    }
}

char *rnto(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        if (!clients[index].renameState)
            return "503 Please send correct RNFR command first\r\n";
        else {
            char command[1024] = {0};
            strcpy(command, "mv ");
            strcat(command, clients[index].renamePath);
            strcat(command, " ");
            strcat(command, content);
            int error = system(command);
            if (error)
                return "553 Renaming file failed\r\n";
            else
                return "250 Renaming file succeed\r\n";
        }
    }
}

char *sendFile(int index) {
    return NULL;
}

char *sendList(int index) {
    char curDir[] = ".";
    char upDir[] = "..";
    char size[BUF_LEN] = {0};
    char fileName[BUF_LEN] = {0};
    struct dirent *dp;
    struct stat dirState;

    if (S_ISREG(clients[index].dirState.st_mode)) {
        stat(clients[index].dirName, &dirState);
        char *file = strrchr(clients[index].dirName, '/');
        if (file)
            ++file;
        else
            file = clients[index].dirName;
        strcpy(data, "-rw-r--r-- 1 owner group");
        sprintf(size, "%13d", (int) dirState.st_size);
        strcat(data, size);
        strcat(data, " ");
        strcat(data, ctime((const time_t *) &dirState.st_ctim));
        strcat(data, " ");
        strcat(data, file);
        strcat(data, "\r\n");
        clients[index].transferState = 4;
        return data;
    } else if (S_ISDIR(clients[index].dirState.st_mode)) {
        if (!clients[index].dirPointer) {
            clients[index].transferState = 4;
            return NULL;
        }
        dp = readdir(clients[index].dirPointer);
        if (!dp) {
            printf("List transfer complete for client %d", index);
            clients[index].transferState = 4;
            return NULL;
        } else {
            if ((0 == strcmp(curDir, dp->d_name)) || (0 == strcmp(upDir, dp->d_name)))
                return NULL;
            strcpy(fileName, clients[index].dirName);
            strcat(fileName, "/");
            strcat(fileName, dp->d_name);
            stat(fileName, &dirState);

            if (dp->d_type == DT_DIR)
                strcpy(data, "drwxr-xr-x 1 owner group");
            else if (dp->d_type == DT_REG)
                strcpy(data, "-rw-r--r-- 1 owner group");
            sprintf(size, "%13d", (int) dirState.st_size);
            strcat(data, size);
            strcat(data, " ");
            strcat(data, ctime((const time_t *) &dirState.st_ctim));
            strcat(data, " ");
            strcat(data, dp->d_name);
            strcat(data, "\r\n");
            return data;
        }
    }

    return NULL;
}

char **parseCommand(char *command) {
    char **result = (char **) malloc(2 * sizeof(char *));
    result[0] = strtok(command, " ");
    result[1] = strtok(NULL, " ");
    return result;
}


void upper(char *string) {
    if (!string)
        return;
    for (int i = 0; i < strlen(string); ++i) {
        if (string[i] >= 'a' && string[i] <= 'z')
            string[i] += ('A' - 'a');
    }
}

int removeDir(const char *dir) {
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[BUF_LEN];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;

    if (0 != access(dir, F_OK)) {
        return -1;
    }

    if (0 > stat(dir, &dir_stat)) {
        perror("get directory stat error");
        return -1;
    }

    if (S_ISREG(dir_stat.st_mode)) {
        remove(dir);
    } else if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(dir);
        while ((dp = readdir(dirp)) != NULL) {
            if ((0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name))) {
                continue;
            }

            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            removeDir(dir_name);
        }
        closedir(dirp);

        rmdir(dir);
    } else {
        perror("Unknown file type");
    }

    return 0;
}

int getLocalIP(const char *eth_inf, char *ip) {
    int sd;
    struct sockaddr_in sin;
    struct ifreq ifr;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == sd) {
        printf("socket error: %s\n", strerror(errno));
        return -1;
    }

    strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;

    // if error: No such device
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
        printf("ioctl error: %s\n", strerror(errno));
        close(sd);
        return -1;
    }

    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    snprintf(ip, 16, "%s", inet_ntoa(sin.sin_addr));

    close(sd);
    return 0;
}


