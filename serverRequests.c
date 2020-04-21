#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void doesProjectExist( int sock, char* projectName );

void doesProjectExist( int sock, char* projectName ) {
    int projectNameLength = strlen("project:") + strlen(projectName) + 1;
    char* projectNameRequest = (char*)malloc(projectNameLength);
    memset(projectNameRequest,'\0',projectNameLength);
    strcat(projectNameRequest,"project:");
    strcat(projectNameRequest,projectName);
    send(sock, projectNameRequest, projectNameLength, 0);
    char exists[1];
    read(sock, exists, 1);
    if( atoi(exists) == 0 ) {
        printf("Project does not exist on filesystem.");
        exit(1);
    }
}