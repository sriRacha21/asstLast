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

struct exitNode* variableList = NULL;

struct exitNode* createNode(char* vName, char* vData){
    struct exitNode* toReturn = malloc(sizeof(struct exitNode));
    toReturn->variableName = malloc(sizeof(char) * strlen(vName) + 1);
    strcpy(toReturn->variableName, vName);
    toReturn->variableName[strlen(toReturn->variableName)] = '\0';

    toReturn->variableData = malloc(sizeof(char) * strlen(vData) + 1);
    strcpy(toReturn->variableData, vData);
    toReturn->variableData[strlen(toReturn->variableData)] = '\0';

    toReturn->next = NULL;
    free(vData); //so do NOT PASS A STRING LITERAL AS the SECOND ARUGMENT!
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

void printList(struct exitNode* head){
    struct exitNode* pointer = head;
    while(pointer != NULL){
        printf("%s: %s\n", pointer->variableName, pointer->variableData);
        pointer = pointer->next;
    }
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
    printf("Server is now shutting down.\n");
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