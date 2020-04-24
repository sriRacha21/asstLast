#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "definitions.h"

void doesProjectExist( int sock, char* projectName );

typedef struct manifestEntry {
    int version;
    char* path;
    char* hash;
} manifestEntry;

void doesProjectExist( int sock, char* projectName ) {
    int projectNameLength = strlen("project:") + strlen(projectName) + 1;
    char* projectNameRequest = (char*)malloc(projectNameLength);
    memset(projectNameRequest,'\0',projectNameLength);
    strcat(projectNameRequest,"project:");
    strcat(projectNameRequest,projectName);
    send(sock, projectNameRequest, projectNameLength, 0);
    char exists[7];
    recv(sock, exists, 7 * sizeof(char), 0);
    if( strcmp(exists, "doesnt") == 0 ) {
        printf("Project does not exist on filesystem.");
        exit(1);
    } else if( DEBUG )
        printf("Project found on filesystem!\n");
}

void done( int sock ) {
    send(sock, "finished", 9);
}