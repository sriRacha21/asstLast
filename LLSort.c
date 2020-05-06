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
#include "LLSort.h"

void freeMList(struct manifestNode* head){ //frees linked list
    struct manifestNode* current = head;
    struct manifestNode* current2;
    while(current != NULL){
        current2 = current;
        current = current->next;
        free(current2->data);
        free(current2);
    }
}

int stringCmp(void* first, void* second){ //comparator for 2 strings
    char* arg1 = (char*) first;
    char* arg2 = (char*) second;
    int length1 = strlen(first);
    int length2 = strlen(second);
    int lowLength, i;
    if(length1 == length2) lowLength = length1;
    else if(length1 > length2) lowLength = length2;
    else lowLength = length1; //determining the size of the shorter string
    for(i = 0; i < lowLength; i++){
        if(arg1[i] < arg2[i]) return -1;
        else if(arg1[i] > arg2[i]) return 1;
    }
    if(length1 < length2) return -1;
    else if(length1 > length2) return 1;
    return 0;
}

struct manifestNode* insertManifest(struct manifestNode* head, struct manifestNode* toInsert){ //simple insert at the end of a LL
    if(head == NULL) {
        head = toInsert; //if empty return manifestNode
        toInsert->index = 0;
        globalIndex++;
    }
    else{
        struct manifestNode* current = head;
        while(current->next != NULL) {
            current = current->next;
        }
        current->next = toInsert;
        toInsert->prev = current;
        toInsert->index = globalIndex;
        globalIndex++;
    }
    return head;
}

void printMList(struct manifestNode* head){
    struct manifestNode* current = head;
    while(current != NULL){
        printf("Node %d: %s\n", current->index, current->data);
        current = current->next;
    }
}

struct manifestNode* createMNode(char* str){ //createMNodes new manifestNode with argument as data
    struct manifestNode* toInsert = malloc(sizeof(struct manifestNode));
    if( toInsert == NULL )
        printf( "Warning: malloc returned null! Continuing...\n");
    toInsert->next = NULL;
    toInsert->prev = NULL;
    toInsert->data = malloc(sizeof(char) * strlen(str)+1);
    strcpy(toInsert->data, str);
    toInsert->data[strlen(toInsert->data)] = '\0';
    toInsert->index = -1;
    return toInsert;
}

int insertionSort(void* toSort, int (*comparator)(void*, void*)){
    struct manifestNode* current = toSort;
    struct manifestNode* backTrav = NULL;
    if(current->next == NULL || current == NULL) return 1; //if empty linked list or linked list only contains 1 node return
    current = current->next;
    while(current != NULL){
        backTrav = current;
        while(backTrav != NULL){
            if(backTrav->prev != NULL){
                //get the filepaths to compare instead of comparing the whole ass thing
                char filePath1[256] = {0};
                char filePath2[256] = {0};
                int i, j;
                for(i = 0; i < strlen(backTrav->data); i++){
                    if(backTrav->data[i] == ' ') break;
                }
                i++;
                for(j = i; j < strlen(backTrav->data); j++){
                    if(backTrav->data[j] == ' ') break;
                }
                strncpy(filePath1, backTrav->data+i, j - i);
                filePath1[strlen(filePath1)] = '\0';

                for(i = 0; i < strlen(backTrav->prev->data); i++){
                    if(backTrav->prev->data[i] == ' ') break;
                }
                i++;
                for(j = i; j < strlen(backTrav->prev->data); j++){
                    if(backTrav->prev->data[j] == ' ') break;
                }
                strncpy(filePath2, backTrav->prev->data+i, j-i);
                filePath2[strlen(filePath2)] = '\0';

                if( comparator(filePath1, filePath2) < 0 ) {
                    char* temp = backTrav->data;
                    backTrav->data = backTrav->prev->data;
                    backTrav->prev->data = temp;
                }
                else break;
            }
            backTrav = backTrav->prev;
        }
        current = current->next;
    }
    //printf("tried to sort\n");
    return 1;
}