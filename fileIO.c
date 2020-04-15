#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "definitions.h"

char* readFile(char *filename);
void writeFile( char *path, char *content );
void writeFileAppend( int fd, char *content );

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