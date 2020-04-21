// standard imports
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// networking imports
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
// file IO
#include <sys/stat.h>
#include <fcntl.h>
// errors
#include <errno.h>
// my own imports
#include "definitions.h"
#include "fileIO.h"

// definitions
#define FALSE 0
#define TRUE 1
#define PORT 8080

// prototypes
void stopped();
void fatalError(char* message);
int main( int argc, char** argv );
void configure( int argc, char** argv );
void checkout( int argc, char** argv );

/*  HELPERS */
void stopped() { printf("Interrupt signal received\n"); }
void fatalError(char* message) {
    printf("Error: %s\n", message);
    exit(1);
}

// globals
int sock;

/*  PROGRAM BODY    */
int main( int argc, char** argv ) {
    // if we need to configure, configure the file and stop
    if(argc >= 2 && strcmp(argv[1],"configure") == 0) {
        configure(argc, argv);
        return 0;
    }
    // open socket connection
    sock = 0;
    int valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\nSocket creation error.\n");
        exit(1);
    }

    // check if the configuration file exists
    if( access(".configure", F_OK) < 0 ) {
        printf("Configuration file does not exist.\n");
        exit(1);
    }
    // get IP and port from configure file, if possible
    char* configurationFile = readFile(".configure");
    char* ip = strtok(configurationFile, "\n");
    char* port = strtok(NULL, "\n");
    if( DEBUG ) printf("Attempting to connect to IP: %s:%s\n", ip, port);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));

    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0){
        printf("\nInvalid address/Address not supported.\n");
        exit(1);
    }

    // connect to server, reattempting every 3 seconds
    while( TRUE ){
        int connectionStatus = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if( connectionStatus < 0 ) {
            perror("Connection to server failed");
            printf("Trying again in 3 seconds...\n");
            sleep(3);
        } else if( connectionStatus == 0 ) {
            printf("Successfully connected to server!\n");
            break;
        }
    }

    if( argc < 3 || argc > 4 )
        fatalError("Too few or too many arguments.");
    if( strcmp(argv[1],"checkout") == 0 ) checkout( argc, argv );
    if( strcmp(argv[1],"update") == 0 ) update( argc, argv );

    // free
    free(configurationFile);

    return 0;
}

/*  CLIENT COMMANDS */
void configure( int argc, char** argv ) {
    // guard clause for argument count
    if( argc != 4 ) fatalError("Too few or too many arguments for client configure! IP and port are required arguments.");
    
    // store path as variable
    char configurePath[13] = ".configure";
    // delete file in case it exists already
    remove(configurePath);
    // open the file
    int fd = open( configurePath, O_RDWR | O_CREAT | O_APPEND );
    // write the IP and host to the file, separated by a new line
    writeFileAppend(fd, argv[2]); // host
    writeFileAppend(fd, "\n"); // new line
    writeFileAppend(fd, argv[3]); // port
    // chmod for viewing
    chmod( configurePath, S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH );
    // close the file
    close(fd);
}

void checkout( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for client checkout! Project name is a required argument.");

    // tell server we need the manifest, assume send is blocking
    int manifestRequestLength = strlen("manifest:") + strlen(argv[2]) + 1;
    char* manifestRequest = (char*)malloc(manifestRequestLength);
    memset(manifestRequest,'\0',manifestRequestLength);
    strcat(manifestRequest,"manifest:");
    strcat(manifestRequest,argv[2]);

    send(sock, manifestRequest, 9, 0);
    // read a response from the server
    char* manifest = readManifestFromSocket(sock);
    // write that to ./
    writeFile("./Manifest", manifest);
    
    // ask the server if the project exists
    doesProjectExist(sock, argv[2]);

    // build a string that will be used to request all the files in the project from the server
    int projectFileNameLength = strlen("project file:") + strlen(argv[2]) + 1;
    char* projectFileName = (char*)malloc(projectFileNameLength);
    memset(projectFileName,'\0',projectFileNameLength);
    strcat(projectFileName,"project file:");
    strcat(projectFileName,argv[2]);
    // read in all files from server and write them out
    int status;
    do {
        send(sock, projectFileName, 13, 0);
        status = rwFileFromSocket(sock);
    } while( status == 0 );

    // free
    free(manifest);
}

void update( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for client update! Proejct name is a required argument.");
}