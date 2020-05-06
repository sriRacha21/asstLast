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
#include <sys/stat.h>
#include "fileIO.h"
#include "requestUtils.h"
#include "exitLeaks.h"
#include "manifestControl.h"
#include "md5.h"
#include "LLSort.h"
#include "definitions.h"

#define ARUR S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH // (all read user write)

/*  HASH FUNCTION (MD5)  */
void md5hash(char* input, char* buffer) {
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5,input,strlen(input));
    MD5_Final(buffer,&md5);
}

void commitToManifest(char* projectName, char* commitContents, int newVersion){
    char filePath[256] = {0};
    strcat(filePath, projectName);
    strcat(filePath, "/.Manifest");

    //take a copy of manifest and commit so we can tokenize
    char* manifestCopy = readFile(filePath);
    char* commitCopy = malloc(sizeof(char) * strlen(commitContents)+1);
    strcpy(commitCopy, commitContents);
    commitCopy[strlen(commitCopy)] = '\0';

    //remove manifest if it do be existin already das wild yo
    remove(filePath);

    printf("Remaking manifest\n");
    //go through old manifest and add everything that was there, after topping it with new version number
    createManifest(projectName, newVersion);
    int fd = open(filePath, O_RDWR | O_CREAT | O_APPEND);
    char* token;
    //get old version number out of the way we are not using this
    token = strtok(manifestCopy, "\n");
    //first real token
    token = strtok(NULL, "\n");
    while(token != NULL){ //fill up new manifest with old manifest contents
        writeFileAppend(fd, token);
        writeFileAppend(fd, "\n");
        token = strtok(NULL, "\n");
    }

    printf("Making changes based off commit...\n");
    //now go through commit and make the appropriate changes
    //gonna be repurposing exitNodes for this
    char* token2;
    struct exitNode* commitList = NULL;
    //first token
    token2 = strtok(commitCopy, "\n");
    while(token2 != NULL){
        char mode[2] = {0};
        char data[400] = {0};
        strncpy(mode, token2, 1);
        strncpy(data, token2+2, strlen(token2)-2);
        commitList = insertExit(commitList, createNode(data, mode, 0));
        token2 = strtok(NULL, "\n");
    }
    close(fd);

    printf("Rewriting manifest\n");
    //now we have a linked list of changes and its associated manifest entry by rewriting yet again
    //go thru and make da changes, editing if M, deleting if D, add if 0
    char* manifestSecondCopy = readFile(filePath);
    remove(filePath);
    createManifest(projectName, newVersion);

    int fd2 = open(filePath, O_RDWR | O_CREAT | O_APPEND);
    struct exitNode* current = commitList;
    while(current != NULL){
        if(current->variableData[0] == 'A') {
            writeFileAppend(fd2, current->variableName);
            writeFileAppend(fd2, "\n");
        }
        current = current->next;
    }

    char* token3;
    token3 = strtok(manifestSecondCopy, "\n"); //version token again ignore
    token3 = strtok(NULL, "\n");
    while(token3 != NULL){
        //find the filepath of the token
        char token3Path[400] = {0};
        int i, j;
        for(i = 0; i < strlen(token3); i++){
            if(token3[i] == ' ') break;
        }
        i++;
        for(j = i; j < strlen(token3); j++){
            if(token3[j] == ' ') break;
        }
        strncpy(token3Path, token3+i, j - i);
        //1 = A ; 2 = D ; 3 = M ; -1 = do nothing
        int whatToDo = whatDoWithToken(commitList, token3Path);
        if(whatToDo == -1){ //no change, add it back into the manifest as is
            writeFileAppend(fd2, token3);
            writeFileAppend(fd2, "\n");
        } else if(whatToDo == 1 || whatToDo == 3){ //changed, add the one from the commit LL
            char* toAppend = returnMallocCopyOfName(commitList, token3Path);
            writeFileAppend(fd2, toAppend);
            writeFileAppend(fd2, "\n");
            free(toAppend);
        }
        //token3 = strtok(NULL, "\n");
    }

    close(fd);
    sortManifest(projectName);
    free(manifestCopy);
    free(commitCopy);
    free(manifestSecondCopy);
    freeAllMallocs(commitList);
}

void createManifest(char* projectName, int versionNum){
    char* filePath = malloc(sizeof(char) * (strlen(projectName) + 12));
    memset(filePath, '\0', sizeof(char) * (strlen(projectName) + 12));
    strcat(filePath, projectName);
    strcat(filePath, "/.Manifest"); //build filepath

    remove(filePath); //if it already exists, rebuild it

    char version[11] = {0};
    sprintf(version, "%d", versionNum);
    version[strlen(version)] = '\0';

    writeFile(filePath, version); //create .Manifest in project folder and write the version number at the top of it
    int fd = open(filePath, O_RDWR | O_CREAT | O_APPEND);
    writeFileAppend(fd, "\n");

    // close
    close(fd);
    //fillManifest(projectName, filePath, versionNum);
    //sortManifest(projectName);
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
    if( DEBUG ) printf("Re-sorting .Manifest version: %s\n", version);
    while(token != NULL){ //get all tokens of manifest
        token = strtok(NULL, "\n");
        if(token == NULL) break;
        char* forNode = malloc(sizeof(char) * (strlen(token)+1));
        memset(forNode, '\0', sizeof(char) * (strlen(token)+1));
        strcat(forNode, token);
        forNode[strlen(forNode)] = '\0';
        tokenList = insertManifest(tokenList, createMNode(forNode));
        // free(forNode);
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
    chdir("..");
}

char* getEntryFromManifest( char* manifestPath, char* filename ) {
    // for each entry in the old manifest, if the entry does not contain the path write it out
    char* manifestContent = readFile(manifestPath);
    char* manifestEntry = strtok(manifestContent,"\n");
    manifestEntry = strtok(NULL,"\n");
    while( manifestEntry != NULL ) {
        // check if path is in entry
        if( strstr(manifestEntry,filename) != NULL ) {
            char* manifestEntryCopy = (char*)malloc(strlen(manifestEntry)+1);
            strcpy(manifestEntryCopy,manifestEntry);
            free(manifestContent);
            return manifestEntryCopy;
        }
        // move string for tokenizer
        manifestEntry = strtok(NULL,"\n");
    }
    free(manifestContent);
    return "";
}

void incrementManifestVersion( char* manifestPath ) {
    char* manifestContents = readFile(manifestPath);
    char* manifestVersionStr = strtok(manifestContents,"\n");
    int manifestVersion = atoi(manifestVersionStr);
    manifestVersion++;
    remove(manifestPath);
    int fd = open( manifestPath, O_RDWR | O_APPEND | O_CREAT );
    char* manifestVersionNewStr = (char*)malloc(strlen(manifestVersionStr)+3);
    sprintf(manifestVersionNewStr,"%d\n",manifestVersion);
    writeFileAppend(fd,manifestVersionNewStr);
    char* token = strtok(NULL,"\n");
    while( token != NULL ) {
        writeFileAppend(fd,token);
        writeFileAppend(fd,"\n");
        token = strtok(NULL,"\n");
    }

    // chmod
    chmod( manifestPath, ARUR );
    // close
    close(fd);
    // free
    free(manifestContents);
}

int removeEntryFromManifest( char* manifestPath, char* filename ) {
    // build string for tmp manifest
    int manifestTmpPathLength = strlen(manifestPath) + strlen(".tmp") + 1;
    char* manifestTmpPath = (char*)malloc(manifestTmpPathLength);
    sprintf(manifestTmpPath,"%s.tmp",manifestPath);
    // create tmp manifest
    int manifestTmpFd = open( manifestTmpPath, O_RDWR | O_CREAT | O_APPEND );
    // for each entry in the old manifest, if the entry does not contain the path write it out
    char* manifestContent = readFile(manifestPath);
    // count lines in manifest before manipulation
    int initialLineCount = lineCount(manifestContent);
    // prep for looping on entries
    char* manifestEntry = strtok(manifestContent,"\n");
    writeFileAppend(manifestTmpFd,manifestEntry);
    writeFileAppend(manifestTmpFd,"\n");
    manifestEntry = strtok(NULL,"\n");
    while( manifestEntry != NULL ) {
        // check if path is in entry
        if( strstr(manifestEntry,filename) == NULL ) {
            writeFileAppend(manifestTmpFd,manifestEntry);
            writeFileAppend(manifestTmpFd,"\n");
        }
        // move string for tokenizer
        manifestEntry = strtok(NULL,"\n");
    }
    chmod(manifestTmpPath,ARUR);
    char* manifestTmpFileContent = readFile(manifestTmpPath);
    // count lines in new manifest
    int finalLineCount = lineCount(manifestTmpFileContent);
    // rename new one
    rename(manifestTmpPath,manifestPath);
    chmod( manifestPath, ARUR );
    // close
    close(manifestTmpFd);
    // free
    free(manifestTmpPath);
    free(manifestTmpFileContent);
    free(manifestContent);
    // if the final line count is equal to initial line count, return 0, otherwise 1
    return initialLineCount - finalLineCount;
}