#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include "fileIO.h"
#include "requestUtils.h"
#include "exitLeaks.h"
#include "manifestControl.h"
#include "md5.h"

void createManifest(char* projectName){
    char* filePath = malloc(sizeof(char) * (strlen(projectName) + 12));
    strcat(filePath, projectName);
    strcat(filePath, "/.Manifest");
    writeFile(filePath, "0\n"); //create .Manifest in project folder

    fillManifest(projectName, filePath);
}

void fillManifest(char* ogPath, char* writeTo){
    char path[256] = {0};
    struct dirent* dirPointer;
    DIR* currentDir = opendir(ogPath);
    if(!currentDir) return;
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            if(dirPointer->d_type == 8 && dirPointer->d_name[0] != '.'){
                int fd = open(writeTo, O_RDWR | O_CREAT | O_APPEND);
                writeFileAppend(fd, "0 ");
                writeFileAppend(fd, ogPath);
                writeFileAppend(fd, "/");
                writeFileAppend(fd, dirPointer->d_name);
                writeFileAppend(fd, " ");

                char* relativePath = malloc(sizeof(char) * (strlen(ogPath) + 1 + strlen(dirPointer->d_name) + 1));
                strcat(relativePath, ogPath);
                strcat(relativePath, "/");
                strcat(relativePath, dirPointer->d_name);
                relativePath[strlen(relativePath)] = '\0';
                char* hashTransposed = convertHash(relativePath);
                writeFileAppend(fd, hashTransposed);
                free(hashTransposed);

                writeFileAppend(fd, "\n");
            }
            strcpy(path, ogPath);
            strcat(path, "/");
            strcat(path, dirPointer->d_name);
            fillManifest(path, writeTo);
        }
    }
    closedir(currentDir);
}

char* convertHash(char* filePath){
    char* fileContents = readFile(filePath);
    char hashBuffer[17];
    memset(hashBuffer, '\0', 17);
    md5hash(fileContents, hashBuffer);
    free(fileContents);
    char* hashTransposed = malloc(sizeof(char) * 34);
    memset(hashTransposed, '\0', 34);
    int i;
    for(i = 0; i < strlen(hashBuffer); i++){
        sprintf(&hashTransposed[i], "%02x", hashBuffer[i]);
    }
    return hashTransposed;
}

void sortManifest(char* projectName){
    chdir(projectName);
    char* manifestContents = readFile(".Manifest");
    char* token;
    int i = 1;
    token = strtok(manifestContents, "\n");
    while(token != NULL){
        printf("Token %d: %s\n", i, token);
        token = strtok(NULL, "\n");
        i++;
    }
    chdir(".");
}