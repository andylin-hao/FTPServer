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
        memset(clients[i].directory, 0, sizeof(clients[i].directory));
        strcpy(clients[i].directory, ROOT);
        memset(clients[i].renamePath, 0, sizeof(clients[i].renamePath));
    }
}

char *processMsg(char *msg, int index) {
    char **command = parseCommand(msg);

    if (!command[0])
        return "500 Please input correct command";

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
        return "500 Please input correct command";
}

char **parseCommand(char *command) {
    char **result = (char **) malloc(2 * sizeof(char *));
    result[0] = strtok(command, " ");
    result[1] = strtok(NULL, " ");
    return result;
}

char *user(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    if (!strcmp(content, "anonymous") && clients[index].loginState < 2) {
        clients[index].loginState = 1;
        return "331 Guest login ok, send your complete e-mail address as password.";
    } else if (strcmp(content, "anonymous") != 0) {
        return "530 User name incorrect";
    } else {
        return "550 Unknown error";
    }
}

char *pass(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";

    char *pattern = ".+@.+..+";
    regex_t reg;
    regmatch_t pmatch[1];
    regcomp(&reg, pattern, REG_EXTENDED);
    int result = regexec(&reg, content, 1, pmatch, 0);

    if (!result && clients[index].loginState == 1) {
        clients[index].loginState = 2;
        return "230 Login ok";
    } else if (!result && clients[index].loginState == 2) {
        return "202 Already logged in";
    } else if (!result && clients[index].loginState == 0) {
        return "503 Please input USER first";
    } else if (result) {
        return "530 Password incorrect";
    } else {
        return "550 Unknown error";
    }
}

char *retr(char *content, int index) {
    return NULL;
}

char *stor(char *content, int index) {
    return NULL;
}

char *quit(char *content, int index) {
    char *msg = "221 Bye";
    int num = (int) write(clients[index].commandCon, msg, strlen(msg));
    if (num < 0)
        perror("fail to write");
    disconnect(index);
    return NULL;
}

char *syst(char *content, int index) {
    return "215 UNIX Type: L8";
}

char *type(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    if (!strcmp(content, "I"))
        return "200 Type set to I.";
    else
        return "501 Please input correct command argument";
}

char *port(char *content, int index) {
    return NULL;
}

char *pasv(char *content, int index) {
    return NULL;
}

char *mkd(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    else {

        int error = mkdir(content, 0755);
        if (error < 0)
            return "550 Directory creation failed";
        else {
            char *directory = (char *) malloc(1024 * sizeof(char));
            memset(message, 0, sizeof(message));
            chdir(content);
            getcwd(directory, 1024);
            strcpy(message, "250 Directory");
            strcat(message, directory);
            strcat(message, " created");
            free(directory);
            return message;
        }
    }
}

char *cwd(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    else {

        int error = chdir(content);
        if (error < 0)
            return "550 No such file or directory";
        else {
            memset(clients[index].directory, 0, sizeof(clients[index].directory));
            getcwd(clients[index].directory, BUF_LEN);
            return "250 Working directory has been changed";
        }
    }
}

char *pwd(char *content, int index) {
    memset(message, 0, sizeof(message));
    strcpy(message, "257 \"");
    strcat(message, clients[index].directory);
    strcat(message, "\"");
    return message;
}

char *list(char *content, int index) {
    return NULL;
}

char *rmd(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    else {

        int error = removeDir(content);
        if (error < 0)
            return "550 Fail to remove directory";
        else
            return "250 Successfully remove directory";
    }
}

char *rnfr(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    else {
        if (access(content, F_OK))
            return "550 File does not exist";
        else if (access(content, R_OK | W_OK))
            return "550 Permission denied";
        else {
            memset(clients[index].renamePath, 0, sizeof(clients[index].renamePath));
            realpath(content, clients[index].renamePath);
            clients[index].renameState = 1;
            return "350 File is ready";
        }
    }
}

char *rnto(char *content, int index) {
    if (!content)
        return "501 Please input correct command argument";
    else {
        if (!clients[index].renameState)
            return "503 Please send correct RNFR command first";
        else {
            char command[1024]={0};
            strcpy(command, "mv ");
            strcat(command, clients[index].renamePath);
            strcat(command, " ");
            strcat(command, content);
            int error = system(command);
            if(error)
                return "553 Renaming file failed";
            else
                return "250 Renaming file succeed";
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
        perror("Unknown file type");
    }

    return 0;
}

int getFileInfo(char *dir) {
    char cur_dir[] = ".";
    char up_dir[] = "..";
    char dir_name[BUF_LEN];
    char size[BUF_LEN] = {0};
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    char *fileName;

    if (0 != access(dir, F_OK)) {
        return -1;
    }

    if (0 > stat(dir, &dir_stat)) {
        perror("get directory stat error");
        return -1;
    }

    memset(message, 0, sizeof(message));

    if (S_ISREG(dir_stat.st_mode)) {
        sprintf(size, "%d", (int) dir_stat.st_size);

        strcpy(message, "226-file name:");
        strcat(message, dir);
        strcat(message, "\r\n");

        strcat(message, "226-file size:");
        strcat(message, size);
        strcat(message, "\r\n");

        strcat(message, "226-create time:");
        strcat(message, ctime((const time_t *) &dir_stat.st_ctim));
        strcat(message, "\r\n");

        strcat(message, "226-last access time:");
        strcat(message, ctime((const time_t *) &dir_stat.st_atim));
        strcat(message, "\r\n");

        strcat(message, "226-last modified time:");
        strcat(message, ctime((const time_t *) &dir_stat.st_mtim));
        strcat(message, "\r\n");

    } else if (S_ISDIR(dir_stat.st_mode)) {
        strcpy(message, "226 ");
        dirp = opendir(dir);
        while ((dp = readdir(dirp)) != NULL) {
            if ((0 == strcmp(cur_dir, dp->d_name)) || (0 == strcmp(up_dir, dp->d_name))) {
                continue;
            }
            memset(dir_name, 0, sizeof(dir_name));
            sprintf(dir_name, "%s ", dp->d_name);
            strcat(message, dir_name);
        }
        closedir(dirp);

    } else {
        perror("Unknown file type");
    }

    return 0;
}


