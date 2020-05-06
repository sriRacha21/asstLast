#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include "exitLeaks.h"

void catchSigint(int signal){
    printf("\nCaught SIGINT.  Cleaning up and ending...\n");
    exit(0);
}

char* returnMallocCopyOfName(struct exitNode* head, char* token){
    struct exitNode* current = head;
    while(current != NULL){
        if(strstr(current->variableName, token) != NULL){
            char* toReturn = malloc(sizeof(char) * strlen(current->variableName)+1);
            memset(toReturn, '\0', sizeof(char) * strlen(current->variableName)+1);
            strcpy(toReturn, current->variableName);
            toReturn[strlen(toReturn)] = '\0';
            return toReturn;
        }
        current = current->next;
    }
}

int whatDoWithToken(struct exitNode* head, char* token){
    int toReturn = -1;
    struct exitNode* current = head;
    while(current != NULL){
        if(strstr(current->variableName, token) != NULL){
            if(current->variableData[0] == 'A') return 1;
            else if(current->variableData[0] == 'D') return 2;
            else if(current->variableData[0] == 'M') return 3;
        }
        current = current->next;
    }
    return toReturn;
}

struct exitNode* createNode(char* vName, char* vData, int freeMode){
    struct exitNode* toReturn = malloc(sizeof(struct exitNode));
    toReturn->variableName = malloc(sizeof(char) * strlen(vName) + 1);
    memset(toReturn->variableName, '\0', sizeof(char) * strlen(vName) + 1);
    strcpy(toReturn->variableName, vName);
    toReturn->variableName[strlen(toReturn->variableName)] = '\0';

    toReturn->variableData = malloc(sizeof(char) * strlen(vData) + 1);
    memset(toReturn->variableData, '\0', sizeof(char) * strlen(vData) + 1);
    strcpy(toReturn->variableData, vData);
    toReturn->variableData[strlen(toReturn->variableData)] = '\0';

    toReturn->next = NULL;
    if(freeMode == 1) free(vData); //so do NOT PASS A STRING LITERAL AS the SECOND ARUGMENT!
    else if(freeMode == 2) freeVariable(variableList, vName);
    return toReturn;
}

struct exitNode* insertExit(struct exitNode* head, struct exitNode* toInsert){
    if(head == NULL) return toInsert;
    struct exitNode* current = head;
    while(current->next != NULL){
        current = current->next;
    }
    current->next = toInsert;
    return head;
}

struct exitNode* freeVariable(struct exitNode* head, char* vName){ //frees a nodes variable name, data, and node itself and deletes it from LL
    if(strcmp(head->variableName, vName) == 0){
        struct exitNode* lagger = head->next;
        free(head->variableData);
        free(head->variableName);
        free(head);
        return lagger; //if its the first node delete/free everything then return the next thing
    }
    struct exitNode* current = head;
    struct exitNode* lagger = NULL;
    while(current != NULL){
        if(strcmp(current->variableName, vName) == 0){
            lagger->next = current->next;
            free(current->variableData);
            free(current->variableName);
            free(current);
            return head;
        }
        lagger = current;
        current = current->next;
    }
    return head;
}

char* getVariableData(struct exitNode* head, char* vName){
    struct exitNode* current = head;
    while(current != NULL){
        if(strcmp(vName, current->variableName) == 0) return current->variableData;
        current = current->next;
    }
    return NULL;
}

void printList(struct exitNode* head){
    struct exitNode* pointer = head;
    while(pointer != NULL){
        printf("Node: %s: %s\n", pointer->variableName, pointer->variableData);
        pointer = pointer->next;
    }
}

void freeAllMallocs(struct exitNode* head){
    struct exitNode* current = head;
    struct exitNode* lagger = NULL;
    while(current != NULL){
        lagger = current;
        current = current->next;
        free(lagger->variableData);
        free(lagger->variableName);
        free(lagger);
    }
    return;
}

void cleanUp(){ //call at atexit
    freeAllMallocs(variableList);
    if(threadCounter >= MAX_THREADS){
        threadCounter = 0;
        while(threadCounter < MAX_THREADS) pthread_join(threadID[threadCounter++], NULL);
        threadCounter = 0;
    }
    printf("Server is now shutting down...\n");
}

/*int main(int argc, char** argv){
    struct exitNode* list = NULL;
    list = insertExit(list, createNode("testStr", "alabama"));
    list = insertExit(list, createNode("testStr2", "arkansas"));
    list = insertExit(list, createNode("testStr3", "united states of america"));
    list = insertExit(list, createNode("testStr4", "brunswick"));
    printList(list);
    list = freeVariable(list ,"testStr");
    printList(list);
    freeAllMallocs(list);
}*/