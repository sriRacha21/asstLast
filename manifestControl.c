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
#include "server/requestUtils.h"
#include "server/exitLeaks.h"
#include "manifestControl.h"
#include "md5.h"
#include "server/LLSort.h"

/*  HASH FUNCTION (MD5)  */
void md5hash(char* input, char* buffer) {
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5,input,strlen(input));
    MD5_Final(buffer,&md5);
}

void createManifest(char* projectName, int versionNum){
    char* filePath = malloc(sizeof(char) * (strlen(projectName) + 12));
    memset(filePath, '\0', sizeof(char) * (strlen(projectName) + 12));
    strcat(filePath, projectName);
    strcat(filePath, "/.Manifest"); //build filepath

    remove(filePath); //if it already exists, rebuild it

    char version[11] = {0};
    sprintf(version, "%s", versionNum);
    version[strlen(version)] = '\0';

    writeFile(filePath, version); //create .Manifest in project folder and write the version number at the top of it
    int fd = open(filePath, O_RDWR | O_CREAT | O_APPEND);
    writeFileAppend(fd, "\n");

    fillManifest(projectName, filePath, versionNum);
    sortManifest(projectName);
    free(filePath);
}

void createHistory(char* projectName){
    char* filePath = malloc(sizeof(char) * (strlen(projectName) + 11));
    memset(filePath, '\0', sizeof(char) * (strlen(projectName) + 11));
    strcat(filePath, projectName);
    strcat(filePath, "/.History");
    writeFile(filePath, ""); ////create .History file
    free(filePath);
}

void fillManifest(char* ogPath, char* writeTo, int version){
    char path[256] = {0};
    char versionStr[11] = {0};
    sprintf(versionStr, "%s", version);
    versionStr[strlen(versionStr)] = '\0';

    struct dirent* dirPointer;
    DIR* currentDir = opendir(ogPath);
    if(!currentDir) return;
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            if(dirPointer->d_type == 8 && dirPointer->d_name[0] != '.'){
                int fd = open(writeTo, O_RDWR | O_CREAT | O_APPEND);
                writeFileAppend(fd, versionStr);
                writeFileAppend(fd, " ");
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
            fillManifest(path, writeTo, version);
        }
    }
    closedir(currentDir);
}

char* convertHash(char* filePath){
    char* fileContents = readFile(filePath);
    unsigned char hashBuffer[17];
    memset(hashBuffer, '\0', 17);
    md5hash(fileContents, hashBuffer);
    free(fileContents);
    char* hashTransposed = malloc(sizeof(char) * 33);
    memset(hashTransposed, '\0', 33);
    int i;
    for(i = 0; i < 16; i++){
        sprintf(&hashTransposed[i*2], "%02x", hashBuffer[i]);
    }
    return hashTransposed;
}

char* convertHashGivenHash(char* hash) {
    char* hashTransposed = malloc(sizeof(char) * 33);
    memset(hashTransposed, '\0', 33);
    int i;
    for(i = 0; i < 16; i++){
        sprintf(&hashTransposed[i*2], "%02x", hash[i]);
    }
    return hashTransposed;
}

void sortManifest(char* projectName){
    globalIndex = 0;
    struct manifestNode* tokenList = NULL;
    chdir(projectName);

    char* manifestContents = readFile(".Manifest");
    remove(".Manifest");
    writeFile(".Manifest", "");
    int fd = open(".Manifest", O_RDWR | O_CREAT | O_APPEND);

    char* token;
    token = strtok(manifestContents, "\n");
    char* version = malloc(sizeof(char) * (strlen(token)+1));
    memset(version, '\0', sizeof(char) * (strlen(token)+1));
    strcat(version, token);
    version[strlen(version)] = '\0'; //get version number from first line
    printf("Re-sorting .Manifest version: %s\n", version);
    while(token != NULL){ //get all tokens of manifest
        token = strtok(NULL, "\n");
        if(token == NULL) break;
        char* forNode = malloc(sizeof(char) * (strlen(token)+1));
        memset(forNode, '\0', sizeof(char) * (strlen(token)+1));
        strcat(forNode, token);
        forNode[strlen(forNode)] = '\0';
        tokenList = insertManifest(tokenList, createMNode(forNode));
    }
    int (*comparator)(void*, void*);
    (comparator) = stringCmp;
    insertionSort(tokenList, comparator); //sort stuff from da Linked List Baybee.

    //rebuild
    writeFileAppend(fd, version);
    writeFileAppend(fd, "\n");
    struct manifestNode* current = tokenList;
    while(current != NULL){
        writeFileAppend(fd, current->data);
        writeFileAppend(fd, "\n");
        current = current->next;
    }

    free(version);
    free(manifestContents);
    freeMList(tokenList);
    chdir(".");
}

void addEntryToManifest( int manifestFd, char* entry ) {

}

int removeEntryFromManifest( char* manifestPath, char* filename ) {
    // count lines in manifest before manipulation
    int initialLineCount = lineCount(manifestPath);
    // build string for tmp manifest
    int manifestTmpPathLength = strlen(manifestPath) + strlen(".tmp") + 1;
    char* manifestTmpPath = (char*)malloc(manifestTmpPathLength);
    sprintf(manifestTmpPath,"%s.tmp",manifestPath);
    // create tmp manifest
    int manifestTmpFd = open( manifestTmpPath, O_RDWR | O_CREAT | O_APPEND );
    // for each entry in the old manifest, if the entry does not contain the path write it out
    char* manifestContent = readFile(manifestPath);
    char* manifestEntry = strtok(manifestContent,"\n");
    // free
    free(manifestContent);
}