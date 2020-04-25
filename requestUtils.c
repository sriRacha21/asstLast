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
#include "fileIO.h"
#include "requestUtils.h"

void sendProjectFiles(char* projectName, int socket){
    char path[256];
    struct dirent* dirPointer;
    DIR* currentDir = opendir(projectName);
    if(currentDir == NULL){
        perror("Can't open directory");
        exit(EXIT_FAILURE);
    }
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            if(dirPointer->d_type == 8){
                char* fileSpecs = concatFileSpecsWithPath(dirPointer->d_name, projectName);
                send(socket, fileSpecs, sizeof(char) * strlen(fileSpecs), 0);
                free(fileSpecs);
            }
            strcpy(path, projectName);
            strcat(path, "/");
            strcat(path, dirPointer->d_name);
            sendProjectFiles(path, socket);
        }
    }
    closedir(currentDir);
}

char* concatFileSpecs(char* fileName, char* projectName){
    int fileSize = getFileSize(fileName); //size in bytes
    int fileSizeIntLength = lengthOfInt(fileSize); //used for string concatenation and to convert to string
    char* fileSizeStr = malloc(sizeof(char) * fileSizeIntLength); //allocate char array for int to string conversion
    sprintf(fileSizeStr, "%d", fileSize); //convert int of file size (bytes) to a string
    char* fileContents = readFile(fileName); //read in file contents
    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileContents) + 1 + fileSizeIntLength + 1)); //allocate 
    //building string to return
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        exit(EXIT_FAILURE);
    }
    free(fileSizeStr);
    free(fileContents);
    return fullFileSpecs; 
}

char* concatFileSpecsWithPath(char* fileName, char* projectName){
    int fileSize = getFileSize(fileName); 
    int fileSizeIntLength = lengthOfInt(fileSize); 
    char* fileSizeStr = malloc(sizeof(char) * fileSizeIntLength); 
    sprintf(fileSizeStr, "%d", fileSize); 
    char* fileContents = readFile(fileName); 
    //finding relative path via string manip on absolute path
    char* absolutePath = realpath(fileName, NULL);
    int startIndex = strstr(absolutePath, projectName) - absolutePath;
    int pathLength = strlen(absolutePath) - startIndex;
    char* relativePath = malloc(sizeof(char) * pathLength);
    int i;
    for(i = 0; i < pathLength; i++){
        relativePath[i] = absolutePath[i+startIndex];
    }
    //creating string to return
    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileContents) + 1 + pathLength + 1 + fileSizeIntLength + 1)); 
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, relativePath);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        exit(EXIT_FAILURE);
    }
    free(fileSizeStr);
    free(fileContents);
    free(absolutePath);
    free(relativePath);
    return fullFileSpecs; 
}

int lengthOfInt(int num){
    int i = 0;
    while(num > 0){
        num /= 10;
        i ++;
    }
    return i;
}

int getFileSize(char* fileName){
    int fp = open(fileName, O_RDONLY);
    if(fp < 0) return -1; //error
    int position = (int) lseek(fp, 0, SEEK_END);
    lseek(fp, 0, SEEK_SET);
    close(fp);
    return position+1;
}

char* getProjectName(char* msg, int prefixLength){
    int newLength = strlen(msg) - prefixLength;
    char* pName = malloc(sizeof(char) * (newLength+1));
    strncpy(pName, msg+prefixLength, newLength);
    pName[newLength] = '\0'; //need null terminator to end string
    return pName;
}

void projectExists(char* projectName, int socket){//goes through current directory and tries to find if projectName exists
    struct dirent* dirPointer;
    DIR* currentDir = opendir("."); //idk if this is right
    if(currentDir == NULL){
        perror("Can't open directory");
        exit(EXIT_FAILURE);
    }
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(projectName, dirPointer->d_name) == 0 && dirPointer->d_type == 4){
            closedir(currentDir);
            send(socket, "exists", strlen("exists") * sizeof(char), 0);
            return;
        } 
    }
    closedir(currentDir);
    send(socket, "doesnt", strlen("exists") * sizeof(char), 0);
}