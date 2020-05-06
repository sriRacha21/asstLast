#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "definitions.h"
#include "exitLeaks.h"

// definitions
#define ARUR S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH // (all read user write)

void createParentDirectories(char* path);
char* readFile(char *filename);
void writeFile( char *path, char *content );
void writeFileAppend( int fd, char *content );
char* readManifestFromSocket(int sock);
int rwFileFromSocket(int sock);
int lineCount(char* path);
char* buildManifestPath(char* projectName);

void createParentDirectories(char* path){ //takes in a file path and makes the parent directories of the file if tey dont exist alreadykrt
    int i, j;
    char filePath[256] = {0};
    strcat(filePath, "mkdir -p ");
    for(i = strlen(path); i >= 0; i--){
        if(path[i] == '/') break;
    }
    strncpy(filePath + strlen(filePath), path, strlen(path) - (strlen(path) - i-1));
    if( DEBUG ) printf("%s\n", filePath);
    system(filePath);
}

// read a file and return its contents
char* readFile(char *filename) {
    if( DEBUG )
        printf("Attempting to read file %s\n", filename);
    // open the file for reading
    int fd = open(filename, O_RDONLY);
    if( fd < 0 ) {
        printf("Could not read file %s.\n", filename);
        exit(1);
    }
    // find out how big the file is
    int position = (int) lseek( fd, 0, SEEK_END );
    // reset the cursor to the beginning of the file
    lseek( fd, 0, SEEK_SET );
    // allocate that size for the buffer
    char *buffer = (char*)malloc((size_t) (position + 1));
    if( buffer == NULL )
        warn("malloc returned null!");
    // set all the bytes of the buffer to terminating character
    memset( buffer, '\0', (size_t) (position + 1));
    // if the file can be read from, read bytes until we've read them all
    int bytesRead = 0;
    int cursorPosition = 0;
    do {
        bytesRead = read( fd, buffer, position - cursorPosition );
        cursorPosition += bytesRead;
    } while(bytesRead > 0 && cursorPosition < position);
    if( position == 1 )
        warn("Empty file");
    // close file
    close(fd);
    return buffer;
}

void writeFile( char *path, char *content ) {
    if( DEBUG )
        printf("Writing to file: %s\n", path);
    // open file <fileName>.test
    int fd = open(path, O_RDWR | O_CREAT);
    // write to file
    int bytesWritten = 0;
    int cursorPosition = 0;
    do {
        bytesWritten = write( fd, content, strlen(content) );
        cursorPosition += bytesWritten;
    } while(bytesWritten > 0 && cursorPosition < strlen(content));
    // chmod to allow us to read
    chmod(path, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
    // close file, clean-up
    close(fd);
}

void writeFileAppend( int fd, char *content ) {
    // write to file
    int bytesWritten = 0;
    int cursorPosition = 0;
    do {
        // printf("writing content %s to file\n", content);
        bytesWritten = write( fd, content, strlen(content) );
        cursorPosition += bytesWritten;
    } while(bytesWritten > 0 && cursorPosition < strlen(content));
}

char* readManifestFromSocket(int sock) {
    // get the size of the file
    char sizeStr[MAXSIZESIZE];
    int cursorPosition = 0;
    int laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(sock, &sizeStr[cursorPosition], 1);
    }
    while( sizeStr[laggingCursorPosition] != DELIM );
    sizeStr[laggingCursorPosition] = '\0';
    // turn the string into unsigned long
    unsigned long filesize = strtoul(sizeStr, NULL, 10);
    if( DEBUG ) printf("file size: %lu\n", filesize);

    // read in the rest of the message
    char* buffer = (char*)malloc(filesize + 1);
    memset(buffer, '\0', filesize + 1);
    cursorPosition = 0;
    int bytesRead = 0;
    recv(sock, buffer, filesize+1, 0);
    if( DEBUG ) printf("Manifest contents: \n```\n%s\n```\n",buffer);

    return buffer;
}

int rwFileFromSocket(int sock) {
    // get the size of the file
    char sizeStr[MAXSIZESIZE];
    int cursorPosition = 0;
    int laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += recv(sock, &sizeStr[cursorPosition], 1, 0);
    }
    while( sizeStr[laggingCursorPosition] != DELIM );
    sizeStr[laggingCursorPosition] = '\0';
    // check if the string received is just "done" so we know to stop receiving
    if( strcmp(sizeStr,"done") == 0 ) {
        if( DEBUG ) printf("Server says done sending files.\n");
        return 1;
    }
    // turn the string into unsigned long
    unsigned long filesize = strtoul(sizeStr, NULL, 10);
    if( DEBUG ) printf("file size: %lu\n", filesize);
    if( filesize == 0 ) {
        printf("Warning attempting to retrieve empty or non-existent file from server!");
        return 0;
    }
    // get the file path
    char filenameStr[MAXSIZESIZE];
    cursorPosition = 0;
    laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(sock, &filenameStr[cursorPosition], 1);
    } while( filenameStr[cursorPosition-1] != DELIM );
    filenameStr[laggingCursorPosition] = '\0';
    // if( DEBUG ) printf("file name: %s\n",filenameStr);
    // read in the rest of the message
    char* buffer = (char*)malloc(filesize + 1);
    memset(buffer, '\0', filesize+1);
    recv(sock, buffer, filesize, 0);
    // if( DEBUG ) printf("Received from server: \"%s\"\n",buffer);
    // make the folder if it doesn't exist
    int folderCreateCmdLength = strlen("mkdir -p ") + strlen(filenameStr) + 1;
    char* folderCreateCmd = (char*)malloc(folderCreateCmdLength);
    memset(folderCreateCmd,'\0',folderCreateCmdLength);
    strcat(folderCreateCmd,"mkdir -p ");
    // copy string for use for strtok
    char* filenameStrCopy = (char*)malloc(strlen(filenameStr)+1);
    strcpy(filenameStrCopy,filenameStr);
    char* folder = strtok(filenameStrCopy,"/");
    char* laggingFolder = NULL;
    do {
        if( laggingFolder != NULL ) {
            // add a / to the end of that
            char* folderWithSlash = (char*)malloc(strlen(laggingFolder) + 1  + 1);
            memset(folderWithSlash,'\0',strlen(laggingFolder) + 1  + 1);
            strcat(folderWithSlash,laggingFolder);
            strcat(folderWithSlash,"/");
            // append it to the string
            strcat(folderCreateCmd,folderWithSlash);
            // free
            free(folderWithSlash);
        }
        // set lagging token
        laggingFolder = (char*)malloc(strlen(folder)+1);
        strcpy(laggingFolder,folder);
        // get another token
        folder = strtok(NULL,"/");
    } while( folder != NULL );
    system(folderCreateCmd);
    // write out the file to the path
    writeFile(filenameStr, buffer);
    // free
    free(buffer);
    free(filenameStrCopy);
    free(folderCreateCmd);
    // return success
    return 0;
}

int lineCount(char* str) {
    int newLineCounter = 0;
    int i;
    for( i = 0; i < strlen(str); i++ )
        if( str[i] == '\n' )
            newLineCounter += 1;
    return newLineCounter;
}

char* buildManifestPath(char* projectName) {
    int manifestPathSize = strlen(projectName) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(sizeof(char) * manifestPathSize);
    memset(manifestPath,'\0',manifestPathSize);
    snprintf(manifestPath,manifestPathSize,"%s/.Manifest",projectName);
    return manifestPath;
}