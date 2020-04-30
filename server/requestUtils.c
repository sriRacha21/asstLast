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
#include "../fileIO.h"
#include "requestUtils.h"
#include "../manifestControl.h"
#include "exitLeaks.h"

char* checkVersion(char* projectName){ //given "current version:<project name>" gives version from manifest. need to free what it gives
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


    free(fileContents);
    free(path);
    return version;
}

int destroyProject(char* projectName){ //checks if project exists, then destroys it if it does. used in "destroy:<pname"
    struct dirent* dirPointer;
    DIR* currentDir = opendir("."); //idk if this is right
    if(currentDir == NULL){
        printf("Can't open directory.\n");
        return -1;
    }
    int found = -342;
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(projectName, dirPointer->d_name) == 0 && dirPointer->d_type == 4){
            closedir(currentDir);
            found = 42069;
        } 
    }
    closedir(currentDir);
    if(found == -342){
        printf("Project not found.\n");
        return -1;
    }
    destroyFiles(projectName);
    destroyFolders(projectName);
    destroyFolders(projectName);
    destroyFolders(projectName);
    return 1;
}

void destroyFiles(char* projectName){
    char path[256];
    struct dirent* dirPointer;
    DIR* currentDir = opendir(projectName);
    if(!currentDir) return; //end recursion
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            strcpy(path, projectName);
            strcat(path, "/");
            strcat(path, dirPointer->d_name);
            printf("%s\n", path);
            remove(path);
            destroyFiles(path);
        }
    }
    closedir(currentDir);
    return;
}

void destroyFolders(char* projectName){
    char path[256];
    struct dirent* dirPointer;
    DIR* currentDir = opendir(projectName);
    if(!currentDir) return; //end recursion
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            strcpy(path, projectName);
            strcat(path, "/");
            strcat(path, dirPointer->d_name);
            printf("%s\n", path);
            rmdir(path);
            destroyFolders(path);
        }
    }
    closedir(currentDir);
    return;
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
        exit(EXIT_FAILURE);
    }
    free(fileContents);
    free(fileSizeStr);
    return fullFileSpecs;
}

void sendProjectFiles(char* projectName, int socket){ //used in "project file:<project name"
    char path[256];
    struct dirent* dirPointer;
    DIR* currentDir = opendir(projectName);
    if(!currentDir) return; // end of recursion
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            if(dirPointer->d_type == 8){
                char* fileSpecs = concatFileSpecsWithPath(dirPointer->d_name, projectName);
                send(socket, fileSpecs, sizeof(char) * strlen(fileSpecs)+1, 0);
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
        exit(EXIT_FAILURE);
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

char* specificFileStringManip(char* msg, int prefixLength, int pathOrProject){ //pathOrProject = 1 for path 0 for project used in "specific project file:<project name"
    int subLength = strlen(msg) - prefixLength;
    char* substring = malloc(sizeof(char) * (subLength+1));
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
        exit(EXIT_FAILURE);
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