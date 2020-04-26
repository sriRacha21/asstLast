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
#include <exitLeaks.h>

void insertMalloc(struct exitNode** head, char* string){ //inserts at front
    if(*head == NULL) *head = createExitN(string);
    else{
        struct exitNode* pointer = *head;
        while(pointer->next != NULL){
            if(strcmp(string, *pointer->strPtr) == 0){
                pointer->freed = 0;
                pointer->strPtr = &string;
                return;
            }
            pointer = pointer->next;
        }
        pointer->next = createExitN(string);
    }
}

struct exitNode* createExitN(char* string){
    struct exitNode* toReturn = malloc(sizeof(struct exitNode));
    toReturn->strPtr = &string;
    toReturn->freed = 0;
    toReturn->next = NULL;
    return toReturn;
}

void freeMallocs(struct exitNode* head){ //only called at exit
    struct exitNode* pointer = head;
    struct exitNode* lagger = NULL;
    while(pointer != NULL){
        lagger = pointer;
        pointer = pointer->next;
        if(pointer->freed == 0) free(*lagger->strPtr); //if it hasnt been freed already
        free(lagger);
    }
}

void freeVariable(char* string, struct exitNode* head){
    struct exitNode* pointer = head;
    while(pointer != NULL){
        if(strcmp(string, *pointer->strPtr) == 0){
            pointer->freed = 1;
            free(*pointer->strPtr);
            return;
        }
        pointer = pointer->next;
    }
}

void printList(struct exitNode* head){
    struct exitNode* pointer = head;
    while(pointer != NULL){
        printf("%s\n", *pointer->strPtr);
        pointer = pointer->next;
    }
}

int main(int argc, char** argv){
    char* testStr = malloc(sizeof(char) * 9);
    strcpy(testStr, "elective");
    char* testStr2 = malloc(sizeof(char) * 5);
    strcpy(testStr2, "cool");
    char* testStr3 = malloc(sizeof(char) * 3);
    strcpy(testStr3, "in");
    struct exitNode* linkedList = NULL;
    insertMalloc(&linkedList, testStr);
    insertMalloc(&linkedList, testStr2);
    insertMalloc(&linkedList, testStr3);
    printList(linkedList);
    //freeMallocs(linkedList);
    /*char** strPtr = &testStr;
    free(*strPtr);*/
}