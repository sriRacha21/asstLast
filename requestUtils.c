#define _GNU_SOURCE
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
#include "manifestControl.h"
#include "exitLeaks.h"
#include "LLSort.h"
#include "definitions.h"

#define ARUR S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH // (all read user write)

int deleteFilesFromPush(char* commitContents){ //returns greatest version number from the commit
    char* commitCopy = malloc(sizeof(char) * (strlen(commitContents) + 1));
    memset(commitCopy, '\0', (strlen(commitContents) + 1));
    strcat(commitCopy, commitContents);
    commitCopy[strlen(commitCopy)] = '\0';

    char* token;
    token = strtok(commitCopy, "\n");
    int i, j;
    int version = -1;
    //find and delete files to delete
    while(token != NULL){
        if(token[0] == 'D'){
            //found one that we need to delete, grab filepath and remove it
            int counter = 0;
            char filePath[256] = {0}; //this is the one we are erasing from the face of the universe
            sleep(1);
            for(i = 0; i < strlen(token); i++){
                if(token[i] == ' ') counter ++;
                if(counter == 2) break;
            }
            i++;
            sleep(1);
            for(j = i; j < strlen(token); j++){
                if(token[j] == ' ') break;
            }
            strncpy(filePath, token+i, j-i);
            printf("%s\n", filePath);
            remove(filePath);
        }
        char versionStr[11] = {0};
        for(i = 0; i < strlen(token); i++){
            if(token[i] == ' ') break;
        }
        i++;
        for(j = i; j < strlen(token); j++){
            if(token[j] == ' ') break;
        }
        strncpy(versionStr, token+i, j-i);
        int currentVers = atoi(versionStr);
        if(currentVers > version) version = currentVers;
        token = strtok(NULL, "\n");
    }
    free(commitCopy);
    return version;
}

int rewriteFileFromSocket(int socket){
    // get the size of the file
    char sizeStr[11] = {0};
    int cursorPosition = 0;
    int laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(socket, &sizeStr[cursorPosition], 1);
    }
    while( sizeStr[laggingCursorPosition] != ';' );
    sizeStr[laggingCursorPosition] = '\0';
    // check if the string received is just "done" so we know to stop receiving
    if( strcmp(sizeStr,"done") == 0 ) {
        printf("received 'done', ending push process\n");
        return -342;
    }
    // turn the string into int
    int filesize = atoi(sizeStr);
    if( filesize == 0 ) {
        printf("Warning attempting to retrieve empty or non-existent file from server!");
        return -5;
    }
    // get the file path
    char filenameStr[256] = {0};
    cursorPosition = 0;
    laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(socket, &filenameStr[cursorPosition], 1);
    } while( filenameStr[cursorPosition-1] != ';' );
    filenameStr[laggingCursorPosition] = '\0';
    createParentDirectories(filenameStr);
    
    // read in the rest of the message
    char* fileContent = (char*)malloc(filesize + 1);
    memset(fileContent, '\0', filesize+1);
    recv(socket, fileContent, filesize, 0);

    //rewrite file
    remove(filenameStr);
    writeFile(filenameStr, fileContent);
    printf("Overwrote %s\n", filenameStr);
    free(fileContent);
    return 1;
}

char* rwCommitToHistory(int socket, char* projectName){ //returns projects new version
    // get the size of the file
    char sizeStr[11] = {0};
    int cursorPosition = 0;
    int laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(socket, &sizeStr[cursorPosition], 1);
    }
    while( sizeStr[laggingCursorPosition] != DELIM );
    sizeStr[laggingCursorPosition] = '\0';

    // turn the string into int
    int filesize = atoi(sizeStr);

    // read in filepath
    char filenameStr[256] = {0};
    cursorPosition = 0;
    laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(socket, &filenameStr[cursorPosition], 1);
    } while( filenameStr[cursorPosition-1] != DELIM );
    filenameStr[laggingCursorPosition] = '\0';

    // read in the rest of the message
    char* fileContent = (char*)malloc(filesize + 1);
    memset(fileContent, '\0', filesize+1);
    recv(socket, fileContent, filesize, 0);

    //find the version of the new push
    int i, j;
    for(i = 0; i < strlen(fileContent); i++){
        if(fileContent[i] == ' ') break;
    }
    for(j = i + 1; j < strlen(fileContent); j++){
        if(fileContent[j] == ' ') break;
    }
    char newVersion[11] = {0};
    strncpy(newVersion, fileContent+i+1, j-i); //if filecontent is mangled its bc of this
    newVersion[strlen(newVersion)] = '\0';

    // turn it into history filepath
    memset(filenameStr, '\0', 256);
    strcat(filenameStr, projectName);
    strcat(filenameStr, "/.History");
    filenameStr[strlen(filenameStr)] = '\0';

    chmod(filenameStr, ARUR);

    //write onto history
    int fd = open(filenameStr, O_RDWR | O_CREAT | O_APPEND);
    writeFileAppend(fd, newVersion);
    writeFileAppend(fd, "\n");
    writeFileAppend(fd, fileContent);
    writeFileAppend(fd, "\n");
    close(fd);
    return fileContent; //return the commit content
}

void createProjectFolder(char* projectName){ //used in "create:<project name>"
    mkdir(projectName, S_IRWXU | S_IRWXG | S_IRWXO);
    createManifest(projectName, 0);
    createHistory(projectName);
}

char* checkVersion(char* projectName, int socket){ //given "current version:<project name>" gives version from manifest. need to free what it gives
    char* path = malloc(sizeof(char) * (strlen(projectName) + 1 + 9 + 1));
    memset(path, '\0', sizeof(char) * (strlen(projectName) + 1 + 9 + 1));
    strcat(path, projectName);
    strcat(path, "/");
    strcat(path, ".Manifest");
    path[strlen(path)] = '\0';

    char* fileContents = readFile(path);

    char* token;
    token = strtok(fileContents, "\n");
    char* version = malloc(sizeof(char) * (strlen(token)+1));
    memset(version, '\0', sizeof(char) * (strlen(token)+1));
    strcat(version, token);
    version[strlen(version)] = '\0'; //get version number from first line
    send(socket, version, strlen(version)+1, 0);//send version number

    while(token != NULL){
        int i, j;
        char version[11] = {0};
        char filePath[256] = {0};
        for(i = 0; i < strlen(token); i++){
            if(token[i] == ' ')break;
        }
        strncpy(version, token, i);
        i++;
        for(j = i; j < strlen(token); j++){
            if(token[j] == ' ') break;
        }
        strncpy(filePath, token+i, j - i);
        send(socket, version, strlen(version)+1, 0);
        send(socket, filePath, strlen(filePath)+1, 0);

        token = strtok(NULL, "\n");
    }

    free(fileContents);
    free(path);
    return version;
}

char* getSpecificFileSpecs(char* projectName, char* filePath){
    int fileSize = getFileSize(filePath);
    int fileSizeIntLength = lengthOfInt(fileSize);
    char* fileSizeStr = malloc(sizeof(char) * (fileSizeIntLength+1));
    sprintf(fileSizeStr, "%d", fileSize);
    fileSizeStr[strlen(fileSizeStr)] = '\0';

    char* fileContents = readFile(filePath);

    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileSizeStr) + 1 + strlen(filePath) + 1 + strlen(fileContents) + 1));
    memset(fullFileSpecs, '\0', sizeof(char) * (strlen(fileSizeStr) + 1 + strlen(filePath) + 1 + strlen(fileContents) + 1));
    //build string
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, filePath);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        return "ERRORERROR123SENDHELP";
    }
    free(fileContents);
    free(fileSizeStr);
    return fullFileSpecs;
}

void sendProjectFiles(char* projectName, int socket){ //used in "project file:<project name"
    char path[256] = {0};
    struct dirent* dirPointer;
    DIR* currentDir = opendir(projectName);
    if(!currentDir) return; // end of recursion
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            strcpy(path, projectName);
            strcat(path, "/");
            strcat(path, dirPointer->d_name);
            if(dirPointer->d_type == 8 && dirPointer->d_name[0] != '.'){
                char* fileSpecs = concatFileSpecsWithPath(path, projectName);
                send(socket, fileSpecs, sizeof(char) * strlen(fileSpecs)+1, 0);
                //printf("%s\n", fileSpecs);
                free(fileSpecs);
            }
            sendProjectFiles(path, socket);
        }
    }
    closedir(currentDir);
}

char* concatFileSpecs(char* fileName, char* projectName){ //used in manifest:<project name>
    int fileSize = getFileSize(fileName); //size in bytes
    int fileSizeIntLength = lengthOfInt(fileSize); //used for string concatenation and to convert to string
    char* fileSizeStr = malloc(sizeof(char) * fileSizeIntLength+1); //allocate char array for int to string conversion
    sprintf(fileSizeStr, "%d", fileSize); //convert int of file size (bytes) to a string
    fileSizeStr[strlen(fileSizeStr)] = '\0';

    char* fileContents = readFile(fileName); //read in file contents

    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileContents) + 1 + fileSizeIntLength + 1)); //allocate for final returned thing 
    memset(fullFileSpecs, '\0', sizeof(char) * (strlen(fileContents) + 1 + fileSizeIntLength + 1));
    //building string to return
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        return;
    }
    free(fileSizeStr);
    free(fileContents);
    return fullFileSpecs; 
}

char* concatFileSpecsWithPath(char* fileName, char* projectName){ //used in project file:<projectname>
    int fileSize = getFileSize(fileName); 
    int fileSizeIntLength = lengthOfInt(fileSize); 
    char* fileSizeStr = malloc(sizeof(char) * fileSizeIntLength+1); 
    sprintf(fileSizeStr, "%d", fileSize); 
    fileSizeStr[strlen(fileSizeStr)] = '\0';

    char* fileContents = readFile(fileName); //get contents of file

    //finding relative path via string manip on absolute path
    char* absolutePath = realpath(fileName, NULL);
    int startIndex = strstr(absolutePath, projectName) - absolutePath;
    int pathLength = strlen(absolutePath) - startIndex;
    char* relativePath = malloc(sizeof(char) * pathLength+1);
    memset(relativePath, '\0', sizeof(char) * pathLength+1);
    int i;
    for(i = 0; i < pathLength; i++){
        relativePath[i] = absolutePath[i+startIndex];
    }
    relativePath[strlen(relativePath)] = '\0';

    //creating string to return
    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileContents) + 1 + pathLength + 1 + fileSizeIntLength + 1)); 
    memset(fullFileSpecs, '\0', sizeof(char) * (strlen(fileContents) + 1 + pathLength + 1 + fileSizeIntLength + 1));
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, relativePath);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        return;
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

char* specificFileStringManip(char* msg, int prefixLength, int pathOrProject){ //pathOrProject = 1 for path 0 for project used in "specific project file:<project name"
    int subLength = strlen(msg) - prefixLength;
    char* substring = malloc(sizeof(char) * (subLength+1));
    memset(substring, '\0', sizeof(char) * (subLength+1));
    strncpy(substring, msg+prefixLength, subLength);
    substring[strlen(substring)] = '\0';

    int placement;
    for(placement = 0; placement < strlen(substring); placement++){
        if(substring[placement] == ':') break;
    }
    int infoLength;
    if(!pathOrProject) infoLength = placement;
    else infoLength = subLength - placement;

    char* info = malloc(sizeof(char) * (infoLength+1));
    memset(info, '\0', sizeof(char) * (infoLength+1));
    if(!pathOrProject) strncpy(info, substring, infoLength); //copy project name
    else strncpy(info, substring + placement+1, infoLength-1); //copy path
    info[strlen(info)] = '\0';
    free(substring);
    return info;
}

int projectExists(char* projectName, int socket){//used in "project:<project name>"
    struct dirent* dirPointer;
    DIR* currentDir = opendir("."); //idk if this is right
    if(currentDir == NULL){
        perror("Can't open directory");
        return 0;
    }
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(projectName, dirPointer->d_name) == 0 && dirPointer->d_type == 4){
            closedir(currentDir);
            send(socket, "exists", strlen("exists") * sizeof(char)+1, 0);
            return 1;
        } 
    }
    closedir(currentDir);
    send(socket, "doesnt", strlen("exists") * sizeof(char)+1, 0);
    return 0;
}