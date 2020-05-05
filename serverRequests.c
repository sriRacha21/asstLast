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
    if( DEBUG ) printf("Sent request to server: \"%s\"\n", projectNameRequest);
    char exists[7];
    memset(exists,'\0',7);
    recv(sock, exists, 7 * sizeof(char), 0);
    if( DEBUG ) printf("Received from server: \"%s\"\n",exists);
    if( strcmp(exists, "doesnt") == 0 ) {
        printf("Project does not exist on filesystem.\n");
        exit(1);
    } else if( DEBUG )
        printf("Project found on filesystem!\n");
}
void doesntProjectExist( int sock, char* projectName ) {
    int projectNameLength = strlen("project:") + strlen(projectName) + 1;
    char* projectNameRequest = (char*)malloc(projectNameLength);
    memset(projectNameRequest,'\0',projectNameLength);
    strcat(projectNameRequest,"project:");
    strcat(projectNameRequest,projectName);
    send(sock, projectNameRequest, projectNameLength, 0);
    if( DEBUG ) printf("Sent request to server: \"%s\"\n", projectNameRequest);
    char exists[7];
    memset(exists,'\0',7);
    recv(sock, exists, 7 * sizeof(char), 0);
    if( DEBUG ) printf("Received from server: \"%s\"\n",exists);
    if( strcmp(exists, "doesnt") == 0 ) {
        if( DEBUG ) printf("Project does not exist on filesystem.\n");
    } else {
        printf("Project already exists on filesystem.\n");
        exit(1);
    }
}

void done( int sock ) {
    send(sock, "finished", 9, 0);
    if( DEBUG ) printf("Sent request to server: \"finished\"\n");
}
