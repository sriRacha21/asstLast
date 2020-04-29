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
    writeFile(filePath, ""); //create .Manifest in project folder

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