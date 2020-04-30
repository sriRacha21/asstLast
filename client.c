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
#include "serverRequests.h"
#include "md5.h"

// definitions
#define FALSE 0
#define TRUE 1
#define PORT 6969

// prototypes
void warning(char* message);
void fatalError(char* message);
int main( int argc, char** argv );
void configure( int argc, char** argv );
void checkout( int argc, char** argv );
void update( int argc, char** argv );
void upgrade( int argc, char** argv );
void commit( int argc, char** argv );
void md5hash(char* input, char* buffer);

/*  HELPERS */
void warning(char* message) { printf("Warning: %s\n"); }
void fatalError(char* message) {
    printf("Error: %s\n", message);
    exit(1);
}

// globals
int sock;

/*  HASH FUNCTION (MD5)  */
void md5hash(char* input, char* buffer) {
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5,input,strlen(input));
    MD5_Final(buffer,&md5);
}

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
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) fatalError("Socket creation error");

    // check if the configuration file exists
    if( access(".configure", F_OK) < 0 ) fatalError(".configure does not exist.");

    // get IP and port from configure file, if possible
    char* configurationFile = readFile(".configure");
    char* ip = strtok(configurationFile, "\n");
    char* port = strtok(NULL, "\n");
    if( DEBUG ) printf("Attempting to connect to IP: %s:%s\n", ip, port);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));

    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) fatalError("Invalid address/address not supported");

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

    if( argc < 3 || argc > 4 ) fatalError("Too few or too many arguments.");
    if( strcmp(argv[1],"checkout") == 0 ) checkout( argc, argv );
    if( strcmp(argv[1],"update") == 0 ) update( argc, argv );
    if( strcmp(argv[1],"upgrade") == 0 ) upgrade( argc, argv );
    if( strcmp(argv[1],"commit") == 0 ) commit( argc, argv );

    // tell server we are done making requests
    done(sock);
    // free
    free(configurationFile);

    return 0;
}

/*  CLIENT COMMANDS */
void configure( int argc, char** argv ) {
    // guard clause for argument count
    if( argc != 4 ) fatalError("Too few or too many arguments for configure! IP and port are required arguments.");
    
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
    if( argc != 3 ) fatalError("Too few or too many arguments for checkout! Project name is a required argument.");

    // ask the server if the project exists
    doesProjectExist(sock, argv[2]);

    // tell server we need the manifest, assume send is blocking
    int manifestRequestLength = strlen("manifest:") + strlen(argv[2]) + 1;
    char* manifestRequest = (char*)malloc(manifestRequestLength);
    memset(manifestRequest,'\0',manifestRequestLength);
    strcat(manifestRequest,"manifest:");
    strcat(manifestRequest,argv[2]);
    // send a request to the server for the manifest
    send(sock, manifestRequest, manifestRequestLength, 0);
    if( DEBUG ) printf("Sent request to server: \"%s\"\n", manifestRequest);
    // read a response from the server
    char* manifest = readManifestFromSocket(sock);
    // write that to ./
    writeFile(".Manifest", manifest);
    
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
        if( DEBUG ) printf("Sent request to server: \"%s\"\n", projectFileName);
        status = rwFileFromSocket(sock);
    } while( status == 0 );

    // free
    free(manifest);
}

void update( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for update! Project name is a required argument.");

    // ask the server if the project exists
    doesProjectExist(sock, argv[2]);

    if( access(".Manifest", F_OK) < 0 ) fatalError(".Manifest does not exist.");

    // tell server we need the manifest, assume send is blocking
    int manifestRequestLength = strlen("manifest:") + strlen(argv[2]) + 1;
    char* manifestRequest = (char*)malloc(manifestRequestLength);
    memset(manifestRequest,'\0',manifestRequestLength);
    strcat(manifestRequest,"manifest:");
    strcat(manifestRequest,argv[2]);
    // send a request to the server for the manifest
    send(sock, manifestRequest, 9, 0);
    // read a response from the server
    char* serverManifest = readManifestFromSocket(sock);
    // get the local manifest
    char* clientManifest = readFile(".Manifest");
    // compare the local manifest to the server manifest
    // check that both manifests are non-empty
    if(strlen(serverManifest) == 0) warning("Server manifest is empty!");
    else if(strlen(clientManifest) == 0) warning("Local manifest is empty!");
    // open the file for .Update
    remove(".Update");
    int fdUpdate = open(".Update", O_APPEND | O_CREAT | O_RDWR);
    // check for full success (same .Manifest versions)
    if( serverManifest[0] == clientManifest[0] ) {
        printf("Server and local manifest have matching versions. There is nothing to update.");
        return;
    }
    // check for difference between local manifest and server manifest
    char serverHash[17], clientHash[17];
    md5hash(serverManifest, serverHash);
    md5hash(clientManifest, clientHash);
    int isDifferent = strcmp(serverHash,clientHash) == 0 ? 0 : 1;
    // return if the manifests are the same
    if( !isDifferent ) { // this should never happen because we compared the first character and returned if they were the same
        printf("Server and local manifest files are complete match. There is nothing to update.");
        return;
    }
    /*  tokenize the server manifest and local manifest to be able to get versions, filepaths, and hashes (in that order per line)   */
    /*  tokenize the server manifest */
    // allocate space for entries in the server manifest, one struct per entry in the server manifest
    manifestEntry* serverManifestEntries = (manifestEntry*)malloc(sizeof(manifestEntry)*lineCount(serverManifest));
    // save pointers since we are using strtok for processing two strings at a time
    char* savePtrServerManifest;
    // throw away the first value because we don't need project version
    char* serverManifestEntry = strtok_r(serverManifest, "\n", &savePtrServerManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    // loop over the entries in the manifest
    int i = 0;
    while( serverManifestEntry != NULL ) {
        serverManifestEntry = strtok_r(NULL,"\n",&savePtrServerManifest);
        // tokenize the entry into version, path, hash
        char* savePtrServerManifestEntry;
        serverManifestEntries[i].version = atoi(strtok_r(serverManifestEntry," ",&savePtrServerManifestEntry));
        serverManifestEntries[i].path = strtok_r(NULL," ",&savePtrServerManifestEntry);
        serverManifestEntries[i].hash = strtok_r(NULL," ",&savePtrServerManifestEntry);
        // increment the counter
        i++;
    }
    int numServerManifestEntries = i;


    /*  tokenize the client manifest    */
    // allocate space for entries in the local manifest, one struct per entry in the server manifest
    manifestEntry* clientManifestEntries = (manifestEntry*)malloc(sizeof(manifestEntry)*lineCount(clientManifest));
    // save pointers since we are using strtok for processing two strings at a time
    char* savePtrClientManifest;
    // throw away the first value because we don't need project version
    char* clientManifestEntry = strtok_r(clientManifest, "\n", &savePtrClientManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    // loop over the entries in the manifest
    i = 0;
    while( clientManifestEntry != NULL ) {
        clientManifestEntry = strtok_r(NULL,"\n",&savePtrClientManifest);
        // tokenize the entry into version, path, hash
        char* savePtrClientManifestEntry;
        clientManifestEntries[i].version = atoi(strtok_r(clientManifestEntry," ",&savePtrClientManifestEntry));
        clientManifestEntries[i].path = strtok_r(NULL," ",&savePtrClientManifestEntry);
        clientManifestEntries[i].hash = strtok_r(NULL," ",&savePtrClientManifestEntry);
        // increment the counter
        i++;
    }
    int numClientManifestEntries = i;

    /*  Check for differences between local and client manifests    */
    int conflicting = FALSE;
    int fdConflict = -1;
    i = 0;
    while( i < numClientManifestEntries || i < numServerManifestEntries ) {
        // if the client has more entries (server has removed files)
        if( i >= numClientManifestEntries ) {
            int serverVersion = serverManifestEntries[i].version;
            char* serverPath = serverManifestEntries[i].path;
            char* serverHash = serverManifestEntries[i].hash;

            char* toWrite = (char*)malloc(strlen(serverPath)+strlen(serverHash)+5);
            sprintf(toWrite, "D %s %s\n",serverPath,serverHash);
            printf("D %s\n",serverPath);
            writeFileAppend(fdUpdate, toWrite);
            free(toWrite);
        // if the server has more entries (server has files that were added)
        } else if( i >= numServerManifestEntries ) {
            int clientVersion = clientManifestEntries[i].version;
            char* clientPath = clientManifestEntries[i].path;
            char* clientHash = clientManifestEntries[i].hash;

            char* toWrite = (char*)malloc(strlen(clientPath)+strlen(clientHash)+5);
            sprintf(toWrite, "A %s %s\n",clientPath,clientHash);
            printf("D %s\n",clientPath);
            writeFileAppend(fdUpdate, toWrite);
            free(toWrite);
        } else {
            int clientVersion = clientManifestEntries[i].version;
            char* clientPath = clientManifestEntries[i].path;
            char* clientHash = clientManifestEntries[i].hash;

            int serverVersion = serverManifestEntries[i].version;
            char* serverPath = serverManifestEntries[i].path;
            char* serverHash = serverManifestEntries[i].hash;

            // if a difference is found, and the user did not change the file add a line to .Update (writeFileAppend) and output information to stdout
            // check if there is a difference between the local and server hash
            if( strcmp(serverHash,clientHash) == 0 ) continue;
            // check if user changed the file
            char* fileContents = readFile(clientPath);
            char liveHash[16];
            md5hash(fileContents,liveHash);
            // if the user did not change the file write out to .Update
            if( strcmp(liveHash,clientHash) == 0 ) {
                char* toWrite = (char*)malloc(strlen(serverPath)+strlen(serverHash)+5);
                sprintf(toWrite, "M %s %s\n", serverPath, serverHash);
                printf("M %s\n", clientPath);
                writeFileAppend(fdUpdate, toWrite);
                free(toWrite);
            // if the user did change the file write out to conflict and turn on the conflicting flag
            } else {
                conflicting = TRUE;
                fdConflict = open(".Conflict", O_APPEND | O_CREAT | O_RDWR);

                char* toWrite = (char*)malloc(strlen(serverPath)+strlen(liveHash)+5);
                sprintf(toWrite, "C %s %s\n", serverPath, liveHash);
                printf("C %s\n", clientPath);
                writeFileAppend(fdUpdate, toWrite);
                free(toWrite);
            }
            // free
            free(fileContents);
        }
    }
    // delete .Update if there is a conflict
    if( conflicting ) remove(".Update");

    // free
    free(serverManifestEntries);
    free(clientManifestEntries);
    free(serverManifest);
    free(clientManifest);
    close(fdUpdate);
}

void upgrade( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for upgrade! Project name is a required argument.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // check if .Update exists, exit if it does not
    if( access( ".Update", F_OK ) < 0 ) fatalError(".Update does not exist. Do an update.");
    // check if .Conflict exists, exit if it does
    if( access( ".Conflict", F_OK ) >= 0 ) fatalError("A conflict exists in your project. Resolve the conflicts and update.");
    
    // check if .Update is empty, delete the file, and tell user project is up-to-date
    char* updateContents = readFile(".Update");
    if( strlen(updateContents) == 0 ) {
        remove(".Update");
        printf("Your project is up-to-date.\n");
        return;
    }
    // parse .Update file for entries
    char* savePtrUpdate;
    char* updateEntry = strtok_r(updateContents,"\n",&savePtrUpdate);
    while( updateEntry != NULL ) {
        // start the pointer
        char* savePtrUpdateEntry;
        // parse the entry for tag, file location, hash
        char* tag = strtok_r(updateEntry," ",&savePtrUpdateEntry);
        char* filepath = strtok_r(NULL," ",&savePtrUpdateEntry);
        char* hash = strtok_r(NULL," ",&savePtrUpdateEntry);
        // decide what to do based on tag
        // check if tag exists
        if( strlen(tag) < 1 ) continue;
        // on D, remove entry from manifest
        if( tag[0] == 'D' ) {
            // make sure the manifest exists
            if( access( ".Conflict", F_OK ) >= 0 ) fatalError("A deletion entry exists in your .Update and there is no .Manifest");
            // open the new manifest file
            int newManifestFd = open( ".Manifest.tmp", O_CREAT | O_APPEND | O_RDWR );
            // read old manifest
            char* oldManifestContents = readFile(".Manifest");
            // save pointer to tokenize
            char* savePtrManifest;
            char* oldManifestEntry = strtok_r(oldManifestContents,"\n",&savePtrManifest);
            writeFileAppend(newManifestFd, oldManifestEntry);
            while( oldManifestEntry != NULL ) {
                // move token
                oldManifestEntry = strtok_r(NULL,"\n",&savePtrManifest);
                // save ptr for entry elements
                char* savePtrManifestEntry;
                // save parts of entry
                int manifestVersion = atoi(strtok_r(oldManifestEntry," ",&savePtrManifestEntry));
                char* manifestFilepath = strtok_r(NULL," ",&savePtrManifestEntry);
                char* manifestHash = strtok_r(NULL," ",&savePtrManifestEntry);
                // if file paths don't match write the entry
                if( strcmp(manifestFilepath,filepath) != 0 )
                    writeFileAppend(newManifestFd, oldManifestEntry);
            }
        } else if( tag[0] == 'M' || tag[0] == 'A' ) {
            if( tag[0] == 'M' ) remove(filepath);
            // build request to server
            int projectFileNameLength = strlen("specific project file:") + strlen(argv[2]) + strlen(":") + strlen(filepath) + 1;
            char* projectFileName = (char*)malloc(projectFileNameLength);
            memset(projectFileName,'\0',projectFileNameLength);
            strcat(projectFileName,"specific project file:");
            strcat(projectFileName,argv[2]);
            strcat(projectFileName,":");
            strcat(projectFileName,filepath);
            // send request to server
            send(sock, projectFileName, projectFileNameLength, 0);
            // read the file and output it
            rwFileFromSocket(sock);
        }
        // update tokenizer
        updateEntry = strtok_r(NULL,"\n",&savePtrUpdate);
    }
    // delete the update file
    remove(".Update");
    // free
    free(updateContents);
}

void commit( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for commit! Project name is a required argument.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // tell server we need the manifest, assume send is blocking
    int manifestRequestLength = strlen("manifest:") + strlen(argv[2]) + 1;
    char* manifestRequest = (char*)malloc(manifestRequestLength);
    memset(manifestRequest,'\0',manifestRequestLength);
    strcat(manifestRequest,"manifest:");
    strcat(manifestRequest,argv[2]);
    // send a request to the server for the manifest
    send(sock, manifestRequest, 9, 0);
    // fail if the client cannot fetch server manifest
    char* serverManifest = readManifestFromSocket(sock);
    if( strlen(serverManifest) == 0 ) fatalError("The server's .Manifest could not be fetched.");
    // fail if the client has a non-empty .Update file
    // fail if the file exists and is non-empty
    if( access(".Update", F_OK) == 0 ) {
        char* updateContents = readFile(".Update");
        if( strlen(updateContents) > 0 )
            fatalError(".Update exists.");
        free(updateContents);
    }
    if( access(".Conflict", F_OK) < 0 ) fatalError(".Conflict exists ");
    // fail if manifest versions don't match
    if( access(".Manifest", F_OK) < 0 ) fatalError("Local .Manifest does not exist.");
    char* clientManifest = readFile(".Manifest");
    int clientManifestVersion = atoi(strtok(clientManifest,"\n"));
    int serverManifestVersion = atoi(strtok(serverManifest,"\n"));
    if( clientManifestVersion != serverManifestVersion ) fatalError("Please update your local project to commit.");
    // open the .Commit file
    int commitFd = open( ".Commit", O_CREAT | O_RDWR | O_APPEND );
    /*  tokenize the server manifest    */
    // allocate space for entries in the server manifest, one struct per entry in the server manifest
    manifestEntry* serverManifestEntries = (manifestEntry*)malloc(sizeof(manifestEntry)*lineCount(serverManifest));
    // save pointers since we are using strtok for processing two strings at a time
    char* savePtrServerManifest;
    // throw away the first value because we don't need project version
    char* serverManifestEntry = strtok_r(serverManifest, "\n", &savePtrServerManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    // loop over the entries in the manifest
    int i = 0;
    while( serverManifestEntry != NULL ) {
        serverManifestEntry = strtok_r(NULL,"\n",&savePtrServerManifest);
        // tokenize the entry into version, path, hash
        char* savePtrServerManifestEntry;
        serverManifestEntries[i].version = atoi(strtok_r(serverManifestEntry," ",&savePtrServerManifestEntry));
        serverManifestEntries[i].path = strtok_r(NULL," ",&savePtrServerManifestEntry);
        serverManifestEntries[i].hash = strtok_r(NULL," ",&savePtrServerManifestEntry);
        // increment the counter
        i++;
    }
    int numServerManifestEntries = i;

    /*  tokenize the client manifest    */
    // allocate space for entries in the local manifest, one struct per entry in the server manifest
    manifestEntry* clientManifestEntries = (manifestEntry*)malloc(sizeof(manifestEntry)*lineCount(clientManifest));
    // save pointers since we are using strtok for processing two strings at a time
    char* savePtrClientManifest;
    // throw away the first value because we don't need project version
    char* clientManifestEntry = strtok_r(clientManifest, "\n", &savePtrClientManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    // loop over the entries in the manifest
    i = 0;
    while( clientManifestEntry != NULL ) {
        clientManifestEntry = strtok_r(NULL,"\n",&savePtrClientManifest);
        // tokenize the entry into version, path, hash
        char* savePtrClientManifestEntry;
        clientManifestEntries[i].version = atoi(strtok_r(clientManifestEntry," ",&savePtrClientManifestEntry));
        clientManifestEntries[i].path = strtok_r(NULL," ",&savePtrClientManifestEntry);
        clientManifestEntries[i].hash = strtok_r(NULL," ",&savePtrClientManifestEntry);
        // increment the counter
        i++;
    }
    int numClientManifestEntries = i;
    // process manifest entries in parallel
    int failure = FALSE;
    i = 0;
    while( i < numClientManifestEntries || i < numServerManifestEntries ) {
        manifestEntry clientManifestEntry = clientManifestEntries[i];
        manifestEntry serverManifestEntry = serverManifestEntries[i];
        // calculate live hash of file
        char liveHash[17];
        char* filecontents = readFile(clientManifestEntry.path);
        md5hash(filecontents, liveHash);
        free(filecontents);
        // server has files that the client does not
        if( i >= numServerManifestEntries ) {
            // write out delete code
            char* toWrite = (char*)malloc(1 + 1 + 4 + 1 + strlen(clientManifestEntry.path) + 1 + strlen(clientManifestEntry.hash) + 1);
            sprintf(toWrite, "D %d %s %s\n",clientManifestEntry.version+1,clientManifestEntry.path,clientManifestEntry.hash);
            printf("D %s\n",clientManifestEntry.path);
            writeFileAppend(commitFd, toWrite);
            free(toWrite);
        }
        else if( i >= numClientManifestEntries ) {
            // write out add code
            char* toWrite = (char*)malloc(1 + 1 + 4 + 1 + strlen(clientManifestEntry.path) + 1 + strlen(clientManifestEntry.hash) + 1);
            sprintf(toWrite, "A %d %s %s\n",clientManifestEntry.version+1,clientManifestEntry.path,clientManifestEntry.hash);
            printf("A %s\n",clientManifestEntry.path);
            writeFileAppend(commitFd, toWrite);
            free(toWrite);
        // compare hashes and write out M if 
        // 1. the server and client have the same hash AND
        // 2. the live hash of the file is different from client hash
        } else if( strcmp(clientManifestEntry.hash,serverManifestEntry.hash) == 0 && strcmp(liveHash,clientManifestEntry.hash) != 0 ) {
            // write out modify code
            char* toWrite = (char*)malloc(1 + 1 + 4 + 1 + strlen(clientManifestEntry.path) + 1 + strlen(clientManifestEntry.hash) + 1);
            sprintf(toWrite, "M %d %s %s\n",clientManifestEntry.version+1,clientManifestEntry.path,clientManifestEntry.hash);
            printf("M %s\n",clientManifestEntry.path);
            writeFileAppend(commitFd, toWrite);
            free(toWrite);
        }
        // fail if 
        // 1. file in server has different hash than client AND 
        // 2. server has a greater file version than conflict
        if( strcmp(serverManifestEntry.hash,clientManifestEntry.hash) != 0 && serverManifestEntry.version > clientManifestEntry.version ) {
            failure = TRUE;
            break;
        }
    }
    if( failure ) {
        printf("Commit failed.\n");
        remove(".Commit");
    }

    // free
    free(serverManifest);
    free(clientManifest);
    free(serverManifestEntries);
    free(clientManifestEntries);
}