//
// Created by masteryoda on 18-10-26.
//

#include "utils.h"
#include "network.h"

int parseArgs(int argc, const char **argv) {
    if(argc == 1) {
        PORT = 21;
        strcpy(ROOT, "/tmp");
        return 0;
    }

    if(argc != 5){
        printf("Arguments number unmatched\n");
        return -1;
    }

    if (strcmp(argv[1], "-port") != 0 && strcmp(argv[3], "-root") != 0){
        printf("Arguments unmatched\n");
        return -1;
    }

    errno = 0;
    char* ptr = NULL;
    PORT = (int) strtol(argv[2], &ptr, 10);
    if ((errno == ERANGE && (PORT == INT_MAX || PORT == INT_MIN)) || (errno != 0 && PORT == 0) || argv[2] == ptr) {
        printf("Argument port invalid format\n");
        return -1;
    }

    if(access(argv[4], F_OK) < 0) {
        printf("Path %s does not exist\n", argv[4]);\
        return -1;
    }

    memset(ROOT, 0, sizeof(ROOT));
    strcpy(ROOT, argv[4]);
    return 0;
}

void initClients() {
    for (int i = 0; i < MAX_CON; ++i) {
        clients[i].commandCon = -1;
        clients[i].fileTransferCon = -1;
        clients[i].listenCon = -1;
        clients[i].loginState = 0;
        clients[i].renameState = 0;
        clients[i].port = 0;
        clients[i].transferProcess = 0;
        clients[i].dirPointer = NULL;
        clients[i].mode = 0;
        clients[i].port_port = -1;

        memset(clients[i].port_ip, 0, sizeof(clients[i].port_ip));
        memset(clients[i].fileName, 0, sizeof(clients[i].fileName));
        memset(clients[i].renamePath, 0, sizeof(clients[i].renamePath));
        memset(&clients[i].dirState, 0, sizeof(clients[i].dirState));
        memset(clients[i].dirName, 0, sizeof(clients[i].dirName));
        memset(clients[i].directory, 0, sizeof(clients[i].directory));
        strcpy(clients[i].directory, ROOT);
    }
}

char *processMsg(char *msg, int index) {
    parseCommand(msg);

    if (!command[0])
        return "500 Please input correct command\r\n";

    upper(command[0]);

    chdir(clients[index].directory);

    if (clients[index].transferState)
        return NULL;

    if (!strcmp(command[0], "USER"))
        return user(command[1], index);

    if (!strcmp(command[0], "PASS"))
        return pass(command[1], index);

    if (clients[index].loginState != 2)
        return "332 Please login first\r\n";

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

int generateData(int state, int index) {
    switch (state) {
        case 1:
            return sendFile(index);
        case 2:
            return sendList(index);
        default:
            disconnectFileTransfer(index);
            return 0;
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

    char *pattern = ".*@.*";
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
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        struct stat fileState;
        if (access(content, F_OK | R_OK) || stat(content, &fileState))
            return "551 File can't be read or does not exist\r\n";
        else if (!S_ISREG(fileState.st_mode))
            return "551 Path must be a readable regular file\r\n";
        else {
            if (strstr(content, "../"))
                return "501 Path should not content \"..\"\r\n";
            if (clients[index].mode == 0)
                return "425 Data connection has not been established\r\n";
            else if (clients[index].mode == 1) {
                int result = connectToClient(index);
                if (!result)
                    return "425 Cannot connect to client\r\n";
                else {
                    clients[index].transferState = 1;
                    realpath(content, clients[index].fileName);
                    return "150 Data connection established, begin transfer\r\n";
                }
            } else {
                if (clients[index].fileTransferCon == -1)
                    return "425 No data connection\r\n";
                else {
                    clients[index].transferState = 1;
                    realpath(content, clients[index].fileName);
                    return "150 Data connection established, begin transfer\r\n";
                }
            }
        }
    }
}

char *stor(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        FILE *fp = fopen(content, "w");
        if (!fp)
            return "551 File can't be created\r\n";
        else {
            fclose(fp);
            if (strstr(content, "../"))
                return "501 Path should not content \"..\"\r\n";
            if (clients[index].mode == 0)
                return "425 Data connection has not been established\r\n";
            if (clients[index].mode == 1) {
                int result = connectToClient(index);
                if (!result)
                    return "425 Cannot connect to client\r\n";
                else {
                    clients[index].transferState = 3;
                    realpath(content, clients[index].fileName);
                    return "150 Data connection established, begin storing\r\n";
                }
            } else {
                if (clients[index].fileTransferCon == -1)
                    return "425 No data connection\r\n";
                else {
                    clients[index].transferState = 3;
                    realpath(content, clients[index].fileName);
                    return "150 Data connection established, begin storing\r\n";
                }
            }
        }
    }
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
        return "200 Type set to I.\r\n";
    else
        return "501 Please input correct command argument\r\n";
}

char *port(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument\r\n";
    else {
        memset(clients[index].port_ip, 0, sizeof(clients[index].port_ip));
        int error = parseIP_PORT(content, clients[index].port_ip, &clients[index].port_port);
        if (error < 0)
            return "501 Please input correct IP-PORT format\r\n";
        else {
            clients[index].mode = 1;
            disconnectListen(index);
            disconnectFileTransfer(index);
            return "200 Prepare to connect to client\r\n";
        }
    }
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
            if (port == clients[i].port) {
                port = (int) (random() % (65535 - 20000 + 1) + 20000);
                i = 0;
                continue;
            } else
                i++;
        }
        char p1[BUF_LEN] = {0};
        char p2[BUF_LEN] = {0};
        sprintf(p1, "%d", port / 256);
        sprintf(p2, "%d", port % 256);
        strcat(message, ",");
        strcat(message, p1);
        strcat(message, ",");
        strcat(message, p2);
        strcat(message, "\r\n");

        int result = createListeningConn(port, index);
        if (!result)
            return "421 Unable tclients[index].transferState = 0;o listen for data connection\r\n";
        else {
            clients[index].mode = 2;
            return message;
        }
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
            disconnectFileTransfer(index);
            return "450 No such file or directory\r\n";
        } else {
            clients[index].transferState = 2;
            if (stat(content, &clients[index].dirState) < 0) {
                disconnectFileTransfer(index);
                return "450 Directory or file can't be accessed\r\n";
            } else {
                if (S_ISREG(clients[index].dirState.st_mode)) {
                    clients[index].dirPointer = NULL;
                    realpath(content, clients[index].dirName);
                    if (clients[index].fileTransferCon == -1)
                        return "126 File okay, waiting for data connection\r\n";
                    else
                        return "150 File okay";
                } else if (S_ISDIR(clients[index].dirState.st_mode)) {
                    clients[index].dirPointer = opendir(content);
                    realpath(content, clients[index].dirName);
                    if (clients[index].fileTransferCon == -1)
                        return "126 Directory okay, waiting for data connection\r\n";
                    else
                        return "150 Directory okay\r\n";
                } else {
                    disconnectFileTransfer(index);
                    return "450 Unknown file type\r\n";
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

int sendFile(int index) {
    struct stat fileState;
    stat(clients[index].fileName, &fileState);
    if (clients[index].transferProcess >= fileState.st_size) {
        clients[index].transferState = 4;
        return 0;
    } else {
        memset(data, 0, sizeof(data));
        FILE *file = fopen(clients[index].fileName, "r");
        fseek(file, clients[index].transferProcess, SEEK_SET);
        int size = (int) fread(data, 1, BUF_LEN, file);
        fclose(file);
        return size;
    }
}

int sendList(int index) {
    char curDir[] = ".";
    char upDir[] = "..";
    char size[BUF_LEN] = {0};
    char fileName[BUF_LEN] = {0};
    struct dirent *dp;
    struct stat dirState;

    memset(data, 0, sizeof(data));
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
        return (int) strlen(data);
    } else if (S_ISDIR(clients[index].dirState.st_mode)) {
        if (!clients[index].dirPointer) {
            clients[index].transferState = 4;
            return 0;
        }
        dp = readdir(clients[index].dirPointer);
        if (!dp) {
            printf("List transfer complete for client %d", index);
            clients[index].transferState = 4;
            return 0;
        } else {
            if ((0 == strcmp(curDir, dp->d_name)) || (0 == strcmp(upDir, dp->d_name)))
                return 0;
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
            return (int) strlen(data);
        }
    }

    return 0;
}

void parseCommand(char *arg) {
    memset(command, 0, sizeof(command));
    command[0] = strtok(arg, " ");
    command[1] = strtok(NULL, " ");
    if (command[0]) {
        int len = (int) strlen(command[0]);
        if (len > 2) {
            if (command[0][len - 1] == '\n')
                command[0][len - 1] = 0;
            if (command[0][len - 2] == '\r')
                command[0][len - 2] = 0;
        }
    }
    if (command[1]) {
        int len = (int) strlen(command[1]);
        if (len > 2) {
            if (command[1][len - 1] == '\n')
                command[1][len - 1] = 0;
            if (command[1][len - 2] == '\r')
                command[1][len - 2] = 0;
        }
    }
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
        perror("Unknown file type\n");
    }

    return 0;
}

int parseIP_PORT(char *ip_port, char *ip, int *port) {
    char **ip_port_parse = (char **) malloc(6 * sizeof(char *));
    memset(ip_port_parse, 0, 6 * sizeof(char *));
    ip_port_parse[0] = strtok(ip_port, ",");
    for (int i = 1; i < 6; ++i) {
        ip_port_parse[i] = strtok(NULL, ",");
    }
    for (int i = 0; i < 6; ++i) {
        if (ip_port_parse[i] == NULL)
            return -1;
    }
    for (int i = 0; i < 4; ++i) {
        strcat(ip, ip_port_parse[i]);
        if (i != 3)
            strcat(ip, ".");
    }
    char *ptr1 = NULL;
    char *ptr2 = NULL;
    errno = 0;
    int p1 = (int) strtol(ip_port_parse[4], &ptr1, 10);
    int p2 = (int) strtol(ip_port_parse[5], &ptr2, 10);
    if ((errno == ERANGE && (p1 == INT_MAX || p1 == INT_MIN)) || (errno != 0 && p1 == 0) || ip_port_parse[4] == ptr1 ||
        (errno == ERANGE && (p2 == INT_MAX || p2 == INT_MIN)) || (errno != 0 && p2 == 0) || ip_port_parse[5] == ptr2) {
        perror("parse ip error\n");
        return -1;
    }
    *port = p1 * 256 + p2;
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


