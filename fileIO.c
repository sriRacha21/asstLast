#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include "definitions.h"

char* readFile(char *filename);
void writeFile( char *path, char *content );
void writeFileAppend( int fd, char *content );
char* readManifestFromSocket(int sock);
int rwFileFromSocket(int sock);

// read a file and return its contents
char* readFile(char *filename) {
    if( DEBUG )
        printf("Attempting to read file %s\n", filename);
    // open the file for reading
    int fd = open(filename, O_RDONLY);
    if( fd < 0 ) return;
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
    do {
        bytesRead = read(sock, &buffer[cursorPosition], filesize);
        cursorPosition += bytesRead;
    } while( bytesRead > 0 && cursorPosition < filesize);

    return buffer;
}

int rwFileFromSocket(int sock) {
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
    // check if the string received is just "done" so we know to stop receiving
    if( strcmp(sizeStr,"done") == 0 ) return 1;
    // turn the string into unsigned long
    unsigned long filesize = strtoul(sizeStr, NULL, 10);
    if( DEBUG ) printf("file size: %lu\n", filesize);
    // get the file path
    char filenameStr[MAXSIZESIZE];
    cursorPosition = 0;
    laggingCursorPosition = 0;
    do {
        laggingCursorPosition = cursorPosition;
        cursorPosition += read(sock, &filenameStr[cursorPosition], 1);
    } while( filenameStr[cursorPosition] != DELIM );
    filenameStr[laggingCursorPosition] = '\0';
    // read in the rest of the message
    char* buffer = (char*)malloc(filesize + 1);
    memset(buffer, '\0', filesize+1);
    cursorPosition = 0;
    int bytesRead = 0;
    do {
        bytesRead = read(sock, &buffer[cursorPosition], filesize);
        cursorPosition += bytesRead;
    } while( bytesRead > 0 && cursorPosition < filesize);
    // write out the file to the path
    writeFile(filenameStr, buffer);
    // free the buffer
    free(buffer);
    // return success
    return 0;
}