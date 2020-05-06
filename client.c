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
#include "manifestControl.h"
#include "requestUtils.h"

// definitions
#define FALSE 0
#define TRUE 1
#define PORT 6969
#define ARUR S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH // (all read user write)

// prototypes
void warning(char* message);
void fatalError(char* message);
int main( int argc, char** argv );
void configure( int argc, char** argv );
void checkout( int argc, char** argv );
void update( int argc, char** argv );
void upgrade( int argc, char** argv );
void commit( int argc, char** argv );
void push( int argc, char** argv );
void create( int argc, char** argv );
void destroy( int argc, char** argv );
void add( int argc, char** argv );
void removePr( int argc, char** argv );
void currentVersion( int argc, char** argv );
void history( int argc, char** argv );
void rollback( int argc, char** argv );

/*  HELPERS */
void warning(char* message) { printf("Warning: %s\n"); }
void fatalError(char* message) {
    printf("Error: %s\n", message);
    exit(1);
}

// globals
int sock;

void doneMain() {
    done(sock);
}

/*  PROGRAM BODY    */
int main( int argc, char** argv ) {
    // chdir
    chdir("client");
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

    // make it so if the program exits we tell the server we are done
    atexit(doneMain);
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
    if( strcmp(argv[1],"push") == 0 ) push( argc, argv );
    if( strcmp(argv[1],"create") == 0 ) create( argc, argv );
    if( strcmp(argv[1],"destroy") == 0 ) destroy( argc, argv );
    if( strcmp(argv[1],"add") == 0 ) add( argc, argv );
    if( strcmp(argv[1],"remove") == 0 ) removePr( argc, argv );
    if( strcmp(argv[1],"currentversion") == 0 ) currentVersion( argc, argv );
    if( strcmp(argv[1],"history") == 0 ) history( argc, argv );
    if( strcmp(argv[1],"rollback") == 0 ) rollback( argc, argv );

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

    // fail if the folder already exists on client side
    if( access("testFolder/", F_OK) >= 0 ) fatalError("Project folder already exists.");
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
    // create a folder for the project
    char* folderCreationString = (char*)malloc(strlen("mkdir ") + strlen(argv[2]) + strlen(" > /dev/null 2>&1") + 1);
    memset(folderCreationString,'\0',strlen("mkdir ") + strlen(argv[2]) + strlen(" > /dev/null 2>&1") + 1);
    sprintf(folderCreationString,"mkdir %s > /dev/null 2>&1",argv[2]);
    system(folderCreationString);
    free(folderCreationString);
    // write that to new folder
    int manifestLocationLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestLocation = (char*)malloc(manifestLocationLength);
    sprintf(manifestLocation,"%s/.Manifest",argv[2]);
    writeFile(manifestLocation, manifest);
    chmod(manifestLocation, ARUR);
    free(manifestLocation);
    
    // build a string that will be used to request all the files in the project from the server
    int projectFileNameLength = strlen("project file:") + strlen(argv[2]) + 1;
    char* projectFileName = (char*)malloc(projectFileNameLength);
    memset(projectFileName,'\0',projectFileNameLength);
    strcat(projectFileName,"project file:");
    strcat(projectFileName,argv[2]);
    // read in all files from server and write them out
    int status;
    do {
        if( DEBUG ) printf("Sent request to server: \"%s\"\n", projectFileName);
        send(sock, projectFileName, projectFileNameLength, 0);
        status = rwFileFromSocket(sock);
    } while( status == 0 );
    // tell server we got all the files
    send(sock, "all files received", 19,0);

    // free
    free(manifest);
}

void update( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for update! Project name is a required argument.");

    // ask the server if the project exists
    doesProjectExist(sock, argv[2]);

    // build path to manifest
    // char* manifestPath = buildManifestPath(argv[2]); >:( i don't like compiler warning why is that happening
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    memset(manifestPath,'\0',manifestPathLength);
    snprintf(manifestPath,manifestPathLength,"%s/.Manifest",argv[2]);
    // check if manifest exists
    if( access(manifestPath, F_OK) < 0 ) fatalError(".Manifest does not exist.");

    // tell server we need the manifest, assume send is blocking
    int manifestRequestLength = strlen("manifest:") + strlen(argv[2]) + 1;
    char* manifestRequest = (char*)malloc(manifestRequestLength);
    memset(manifestRequest,'\0',manifestRequestLength);
    strcat(manifestRequest,"manifest:");
    strcat(manifestRequest,argv[2]);
    // send a request to the server for the manifest
    send(sock, manifestRequest, manifestRequestLength, 0);
    // read a response from the server
    char* serverManifest = readManifestFromSocket(sock);
    // get the local manifest
    char* clientManifest = readFile(manifestPath);
    // compare the local manifest to the server manifest
    // check that both manifests are non-empty
    if(strlen(serverManifest) == 0) warning("Server manifest is empty!");
    else if(strlen(clientManifest) == 0) warning("Local manifest is empty!");
    // build string for writing out to update within project directory
    int updatePathLength = strlen(argv[2]) + strlen("/.Update") + 1;
    char* updatePath = (char*)malloc(updatePathLength);
    memset(updatePath,'\0',updatePathLength);
    sprintf(updatePath,"%s/.Update",argv[2]);
    // remove .Update if it already exists
    remove(updatePath);
    // remove .Conflict if it already exists
    int conflictPathLength = strlen(argv[2]) + strlen("/.Conflict") + 1;
    char* conflictPath = (char*)malloc(conflictPathLength);
    memset(conflictPath,'\0',conflictPathLength);
    sprintf(conflictPath,"%s/.Conflict",argv[2]);
    remove(conflictPath);
    // open the file for .Update
    int fdUpdate = open(updatePath, O_APPEND | O_CREAT | O_RDWR);
    // check for full success (same .Manifest versions)
    // get first line for server manifest
    char* serverManifestCopy = (char*)malloc(strlen(serverManifest+1));
    strcpy(serverManifestCopy,serverManifest);
    char* serverManifestVersion = strtok(serverManifestCopy,"\n");
    // get first line for local manifest
    char* clientManifestCopy = (char*)malloc(strlen(clientManifest+1));
    strcpy(clientManifestCopy,clientManifest);
    char* clientManifestVersion = strtok(clientManifestCopy,"\n");
    // compare
    if( DEBUG ) printf("Client manifest version: %s\tServer manifest version: %s\n",clientManifestVersion,serverManifestVersion);
    if( strcmp(clientManifestVersion,serverManifestVersion) == 0 ) {
        printf("Server and local manifest have matching versions. There is nothing to update.\n");
        return;
    }
    // check for difference between local manifest and server manifest
    char serverHash[17], clientHash[17];
    md5hash(serverManifest, serverHash);
    md5hash(clientManifest, clientHash);
    int isDifferent = strcmp(serverHash,clientHash) == 0 ? 0 : 1;
    // return if the manifests are the same
    if( !isDifferent ) { // this should never happen because we compared the first character and returned if they were the same
        printf("Server and local manifest files are complete match. There is nothing to update.\n");
        return;
    }
    /*  tokenize the server manifest and local manifest to be able to get versions, filepaths, and hashes (in that order per line)   */
    /*  tokenize the server manifest */
    // allocate space for entries in the server manifest, one struct per entry in the server manifest
    manifestEntry* serverManifestEntries = (manifestEntry*)malloc(sizeof(manifestEntry)*lineCount(serverManifest));
    // save pointers since we are using strtok for processing two strings at a time
    char* savePtrServerManifest;
    // throw away the first value because we don't need project version
    // make a copy of the string for strtok to use
    serverManifestCopy = (char*)malloc(strlen(serverManifest)+1);
    strcpy(serverManifestCopy,serverManifest);
    char* serverManifestEntry = strtok_r(serverManifestCopy, "\n", &savePtrServerManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    serverManifestEntry = strtok_r(NULL,"\n",&savePtrServerManifest);
    // loop over the entries in the manifest
    int i = 0;
    while( serverManifestEntry != NULL ) {
        // tokenize the entry into version, path, hash
        char* savePtrServerManifestEntry;
        serverManifestEntries[i].version = atoi(strtok_r(serverManifestEntry," ",&savePtrServerManifestEntry));
        serverManifestEntries[i].path = strtok_r(NULL," ",&savePtrServerManifestEntry);
        serverManifestEntries[i].hash = strtok_r(NULL," ",&savePtrServerManifestEntry);
        if( DEBUG ) printf("Server manifest entry %d:\n\tversion: %d\n\tpath: %s\n\thash: %s\n",i,serverManifestEntries[i].version,serverManifestEntries[i].path,serverManifestEntries[i].hash);
        // move to the next string
        serverManifestEntry = strtok_r(NULL,"\n",&savePtrServerManifest);
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
    clientManifestCopy = (char*)malloc(strlen(clientManifest)+1);
    strcpy(clientManifestCopy,clientManifest);
    char* clientManifestEntry = strtok_r(clientManifestCopy, "\n", &savePtrClientManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    clientManifestEntry = strtok_r(NULL,"\n",&savePtrClientManifest);
    // loop over the entries in the manifest
    i = 0;
    while( clientManifestEntry != NULL ) {
        // tokenize the entry into version, path, hash
        char* savePtrClientManifestEntry;
        clientManifestEntries[i].version = atoi(strtok_r(clientManifestEntry," ",&savePtrClientManifestEntry));
        clientManifestEntries[i].path = strtok_r(NULL," ",&savePtrClientManifestEntry);
        clientManifestEntries[i].hash = strtok_r(NULL," ",&savePtrClientManifestEntry);
        if( DEBUG ) printf("Client manifest entry %d:\n\tversion: %d\n\tpath: %s\n\thash: %s\n",i,clientManifestEntries[i].version,clientManifestEntries[i].path,clientManifestEntries[i].hash);
        // move to the next string
        clientManifestEntry = strtok_r(NULL,"\n",&savePtrClientManifest);
        // increment the counter
        i++;
    }
    int numClientManifestEntries = i;

    /*  Check for differences between local and client manifests    */
    int conflicting = FALSE;
    int fdConflict = -1;
    for( i = 0; i < numClientManifestEntries || i < numServerManifestEntries; i++ ) {
        if( DEBUG ) printf("Comparing client and server entries for index %d\n",i);
        // if the client has more entries (server has removed files)
        if( i >= numClientManifestEntries ) {
            int serverVersion = serverManifestEntries[i].version;
            char* serverPath = serverManifestEntries[i].path;
            char* serverHash = serverManifestEntries[i].hash;
            // write hash to hex
            char* hexHash = convertHash(serverPath);

            char* toWrite = (char*)malloc(strlen(serverPath)+strlen(hexHash)+5);
            sprintf(toWrite, "D %s %s\n",serverPath,hexHash);
            printf("D %s\n",serverPath);
            writeFileAppend(fdUpdate, toWrite);
            free(toWrite);
            free(hexHash);
        // if the server has more entries (server has files that were added)
        } else if( i >= numServerManifestEntries ) {
            int clientVersion = clientManifestEntries[i].version;
            char* clientPath = clientManifestEntries[i].path;
            char* clientHash = clientManifestEntries[i].hash;
            // write hash to hex
            char* hexHash = convertHash(clientPath);

            char* toWrite = (char*)malloc(strlen(clientPath)+strlen(hexHash)+5);
            sprintf(toWrite, "A %s %s\n",clientPath,hexHash);
            printf("A %s\n",clientPath);
            writeFileAppend(fdUpdate, toWrite);
            free(toWrite);
            free(hexHash);
        } else {
            int clientVersion = clientManifestEntries[i].version;
            char* clientPath = clientManifestEntries[i].path;
            char* clientHash = clientManifestEntries[i].hash;

            int serverVersion = serverManifestEntries[i].version;
            char* serverPath = serverManifestEntries[i].path;
            char* serverHash = serverManifestEntries[i].hash;

            // if the paths don't match then there was an addition AND deletion
            if( strcmp(clientPath,serverPath) != 0 ) {
                // write out add to .Update
                char* toWriteAdd = (char*)malloc(strlen(serverPath) + strlen(serverHash) + 5);
                sprintf(toWriteAdd, "A %s %s\n",serverPath, serverHash);
                printf("A %s\n",serverPath);
                writeFileAppend(fdUpdate, toWriteAdd);
                // write out deletion to .Update
                char* toWriteRemove = (char*)malloc(strlen(clientPath) + strlen(clientHash) + 5);
                sprintf(toWriteRemove, "D %s %s\n",clientPath,clientHash);
                printf("D %s\n",clientPath);
                writeFileAppend(fdUpdate, toWriteRemove);
                // free
                free( toWriteAdd );
                free( toWriteRemove );
                // continue
                continue;
            }
            // if a difference is found, and the user did not change the file add a line to .Update (writeFileAppend) and output information to stdout
            // check if there is a difference between the local and server hash
            if( DEBUG ) printf("Comparing for file %s\nserver hash:\t%s\nclient hash:\t%s\n",clientPath,serverHash,clientHash);
            if( strcmp(serverHash,clientHash) == 0 ) continue;
            // check if user changed the file
            char* liveHexHash = convertHash(clientPath);
            // if the user did not change the file write out to .Update
            if( DEBUG ) printf("Comparing for file %s\nlive hash:\t%s\nclient hash:\t%s\n",clientPath,liveHexHash,clientHash);
            if( strcmp(liveHexHash,clientHash) == 0 ) {
                char* toWrite = (char*)malloc(strlen(serverPath)+strlen(serverHash)+5);
                sprintf(toWrite, "M %s %s\n", serverPath, serverHash);
                printf("M %s\n", serverPath);
                writeFileAppend(fdUpdate, toWrite);
                free(toWrite);
            // if the user did change the file write out to conflict and turn on the conflicting flag
            } else {
                conflicting = TRUE;
                // open the file for writing
                fdConflict = open(conflictPath, O_APPEND | O_CREAT | O_RDWR);
                chmod(conflictPath,ARUR);

                char* toWrite = (char*)malloc(strlen(serverPath)+strlen(liveHexHash)+5);
                sprintf(toWrite, "C %s %s\n", serverPath, liveHexHash);
                printf("C %s\n", clientPath);
                writeFileAppend(fdConflict, toWrite);
                free(toWrite);
                
            }
            // free
            free(liveHexHash);
        }
    }

    // chmod to make all files accessible
    chmod(updatePath,ARUR);

    // delete .Update if there is a conflict
    if( conflicting ) remove(updatePath);

    // free
    free(updatePath);
    free(conflictPath);
    free(manifestPath);
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

    // build path to conflict
    int updatePathLength = strlen(argv[2]) + strlen("/.Update") + 1;
    char* updatePath = (char*)malloc(updatePathLength);
    memset(updatePath,'\0',updatePathLength);
    sprintf(updatePath,"%s/.Update",argv[2]);
    // build path to conflict
    int conflictPathLength = strlen(argv[2]) + strlen("/.Conflict") + 1;
    char* conflictPath = (char*)malloc(conflictPathLength);
    memset(conflictPath,'\0',conflictPathLength);
    sprintf(conflictPath,"%s/.Conflict",argv[2]);
    // build path to manifest
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    memset(manifestPath,'\0',manifestPathLength);
    snprintf(manifestPath,manifestPathLength,"%s/.Manifest",argv[2]);
    // check if .Update exists, exit if it does not
    if( access( updatePath, F_OK ) < 0 ) fatalError(".Update does not exist. Do an update.");
    // check if .Conflict exists, exit if it does
    if( access( conflictPath, F_OK ) >= 0 ) fatalError("A conflict exists in your project. Resolve the conflicts and update.");
    
    // open manifest file for writing
    int manifestFd = open(manifestPath, O_RDWR | O_APPEND);

    // check if .Update is empty, delete the file, and tell user project is up-to-date
    char* updateContents = readFile(updatePath);
    if( strlen(updateContents) == 0 ) {
        remove(updatePath);
        printf("Your project is up-to-date.\n");
        return;
    }
    // increment the version number in it
    incrementManifestVersion(manifestPath);
    // parse .Update file for entries
    char* savePtrUpdate;
    // copy string for strtok
    char* updateContentsCopy = (char*)malloc(strlen(updateContents)+1);
    strcpy(updateContentsCopy,updateContents);
    char* updateEntry = strtok_r(updateContentsCopy,"\n",&savePtrUpdate);
    while( updateEntry != NULL ) {
        if( DEBUG ) printf("Update entry: %s\n",updateEntry);
        // prevent bad reads
        if( strlen(updateEntry) < 32 + strlen(argv[2]) + 1 + 1 ) {
            updateEntry = strtok_r(NULL,"\n",&savePtrUpdate);
            continue;
        }
        // start the pointer
        char* savePtrUpdateEntry;
        // parse the entry for tag, file location, hash
        char* tag = strtok_r(updateEntry," ",&savePtrUpdateEntry);
        char* filepath = strtok_r(NULL," ",&savePtrUpdateEntry);
        char* hash = strtok_r(NULL," ",&savePtrUpdateEntry);
        // decide what to do based on tag
        // check if tag exists
        if( strlen(tag) < 1 ) {
            updateEntry = strtok_r(NULL,"\n",&savePtrUpdate);
            continue;
        }
        // on D delete the entry from the manifest
        if( tag[0] == 'D' ) removeEntryFromManifest(manifestPath, filepath);
        // on M or A write the file out
        else if( tag[0] == 'M' || tag[0] == 'A' ) {
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
            if( DEBUG ) printf("Sending message to server: %s\n",projectFileName);
            send(sock, projectFileName, projectFileNameLength, 0);
            // read the file and output it
            rwFileFromSocket(sock);
            // if the tag is M add a new entry with an incremented file version and remove the original entry
            if( tag[0] == 'M' ) {
                // save pointer for manifestEntry
                char*  savePtrManifestEntry;
                // get the entry from the manifest and the version so we can write out with incremented file version
                char* manifestEntry = getEntryFromManifest(manifestPath, filepath);
                if( strcmp(manifestEntry,"") == 0 ) {
                    updateEntry = strtok_r(NULL,"\n",&savePtrUpdate);
                    continue;
                }
                int version = atoi(strtok_r(manifestEntry," ",&savePtrManifestEntry));
                char* path = strtok_r(NULL," ",&savePtrManifestEntry);
                char* newHash = strtok_r(NULL," ",&savePtrManifestEntry);
                // calculate the livehash
                char* liveHash = convertHash(path);
                // remove the entry
                int linesRemoved = removeEntryFromManifest(manifestPath, filepath);
                int manifestFd = open(manifestPath, O_RDWR | O_APPEND);
                // construct a new entry with incremented version
                version++;
                char* newEntry = (char*)malloc(100 + 1 + strlen(filepath) + strlen(liveHash));
                memset(newEntry,'\0',100 + 1 + strlen(filepath) + strlen(liveHash));
                sprintf(newEntry,"%d %s %s",version,filepath,liveHash);
                // add the new one
                writeFileAppend(manifestFd, newEntry);
                writeFileAppend(manifestFd, "\n");
            }
            // if the tag is A just add a new entry with 0 file version
            if( tag[0] == 'A' ) {
                char* newEntry = (char*)malloc(strlen("0 ") + strlen(filepath) + strlen(hash));
                memset(newEntry,'\0',strlen("0 ") + strlen(filepath) + strlen(hash));
                sprintf(newEntry,"0 %s %s",filepath,hash);
                writeFileAppend(manifestFd, newEntry);
            }
        }
        // update tokenizer
        updateEntry = strtok_r(NULL,"\n",&savePtrUpdate);
    }
    // sort the manifest
    sortManifest(argv[2]);
    // delete the update file
    remove(updatePath);
    // close file
    close(manifestFd);
    // free
    free(updateContents);
    free(updateContentsCopy);
    free(updatePath);
    free(conflictPath);
    free(manifestPath);
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
    send(sock, manifestRequest, manifestRequestLength, 0);
    // fail if the client cannot fetch server manifest
    char* serverManifest = readManifestFromSocket(sock);
    if( strlen(serverManifest) == 0 ) fatalError("The server's .Manifest could not be fetched.");
    // fail if the client has a non-empty .Update file
    // build the path to conflict
    int conflictPathLength = strlen(argv[2]) + strlen("/.Conflict") + 1;
    char* conflictPath = (char*)malloc(conflictPathLength);
    memset(conflictPath,'\0',conflictPathLength);
    sprintf(conflictPath,"%s/.Conflict",argv[2]);
    // build the path to manifest
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    memset(manifestPath,'\0',manifestPathLength);
    snprintf(manifestPath,manifestPathLength,"%s/.Manifest",argv[2]);
    // build the path to Commit
    int commitPathLength = strlen(argv[2]) + strlen("/.Commit") + 1;
    char* commitPath = (char*)malloc(commitPathLength);
    memset(commitPath,'\0',commitPathLength);
    snprintf(commitPath,commitPathLength,"%s/.Commit",argv[2]);
    // build the path to update
    int updatePathLength = strlen(argv[2]) + strlen("/.Update") + 1;
    char* updatePath = (char*)malloc(updatePathLength);
    memset(updatePath,'\0',updatePathLength);
    sprintf(updatePath,"%s/.Update",argv[2]);
    // fail if the file exists and is non-empty
    if( access(updatePath, F_OK) == 0 ) {
        char* updateContents = readFile(updatePath);
        if( strlen(updateContents) > 0 )
            fatalError(".Update exists.");
        free(updateContents);
    }
    if( access(conflictPath, F_OK) == 0 ) fatalError(".Conflict exists ");
    // fail if manifest versions don't match
    if( access(manifestPath, F_OK) < 0 ) fatalError("Local .Manifest does not exist.");
    char* clientManifest = readFile(manifestPath);
    // copy client and server manifests for usage by strtok
    char* clientManifestCopy = (char*)malloc(strlen(clientManifest)+1);
    char* serverManifestCopy = (char*)malloc(strlen(serverManifest)+1);
    strcpy(clientManifestCopy,clientManifest);
    strcpy(serverManifestCopy,serverManifest);
    // use strtok to tokenize client and server manifest
    char* clientManifestVersion = strtok(clientManifestCopy,"\n");
    char* serverManifestVersion = strtok(serverManifestCopy,"\n");
    if( strcmp(clientManifestVersion,serverManifestVersion) != 0 ) fatalError("Please update your local project to commit.");
    // open the .Commit file
    remove(commitPath);
    int commitFd = open( commitPath, O_CREAT | O_RDWR | O_APPEND );
    /*  tokenize the server manifest    */
    // allocate space for entries in the server manifest, one struct per entry in the server manifest
    manifestEntry* serverManifestEntries = (manifestEntry*)malloc(sizeof(manifestEntry)*lineCount(serverManifest));
    // save pointers since we are using strtok for processing two strings at a time
    char* savePtrServerManifest;
    // throw away the first value because we don't need project version
    // use copy of server manifest for tokenizing string
    serverManifestCopy = (char*)malloc(strlen(serverManifest)+1);
    strcpy(serverManifestCopy,serverManifest);
    char* serverManifestEntry = strtok_r(serverManifestCopy, "\n", &savePtrServerManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    serverManifestEntry = strtok_r(NULL,"\n",&savePtrServerManifest);
    // loop over the entries in the manifest
    int i = 0;
    while( serverManifestEntry != NULL ) {
        // tokenize the entry into version, path, hash
        char* savePtrServerManifestEntry;
        serverManifestEntries[i].version = atoi(strtok_r(serverManifestEntry," ",&savePtrServerManifestEntry));
        serverManifestEntries[i].path = strtok_r(NULL," ",&savePtrServerManifestEntry);
        serverManifestEntries[i].hash = strtok_r(NULL," ",&savePtrServerManifestEntry);
        // move the string for tokenizer
        serverManifestEntry = strtok_r(NULL,"\n",&savePtrServerManifest);
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
    // use copy of client manifest for tokenizing string
    clientManifestCopy = (char*)malloc(strlen(clientManifest)+1);
    strcpy(clientManifestCopy,clientManifest);
    char* clientManifestEntry = strtok_r(clientManifestCopy, "\n", &savePtrClientManifest); // 32 bytes for hash, 1 byte for the file version, 2 bytes for spaces, and 100 bytes for file path
    clientManifestEntry = strtok_r(NULL,"\n",&savePtrClientManifest);
    // loop over the entries in the manifest
    i = 0;
    while( clientManifestEntry != NULL ) {
        // tokenize the entry into version, path, hash
        char* savePtrClientManifestEntry;
        clientManifestEntries[i].version = atoi(strtok_r(clientManifestEntry," ",&savePtrClientManifestEntry));
        clientManifestEntries[i].path = strtok_r(NULL," ",&savePtrClientManifestEntry);
        clientManifestEntries[i].hash = strtok_r(NULL," ",&savePtrClientManifestEntry);
        // move the string for the tokenizer
        clientManifestEntry = strtok_r(NULL,"\n",&savePtrClientManifest);
        // increment the counter
        i++;
    }
    int numClientManifestEntries = i;
    // process manifest entries in parallel
    int failure = FALSE;
    for( i = 0; i < numClientManifestEntries || i < numServerManifestEntries; i++ ) {
        manifestEntry clientManifestEntry = clientManifestEntries[i];
        manifestEntry serverManifestEntry = serverManifestEntries[i];
        // calculate live hash of file
        char* clientHexHash = clientManifestEntry.hash;
        char* liveHexHash = convertHash(clientManifestEntry.path);
        if( DEBUG ) printf("Comparing server hash %s\tclient hash %s\t and live hash %s\n",serverManifestEntry.hash,clientManifestEntry.hash,liveHexHash);
        // client has files that the server does not
        if( i >= numServerManifestEntries ) {
            // write out delete code
            char* toWrite = (char*)malloc(1 + 1 + 4 + 1 + strlen(clientManifestEntry.path) + 1 + strlen(clientHexHash) + 1);
            sprintf(toWrite, "A %d %s %s\n",clientManifestEntry.version+1,clientManifestEntry.path,clientHexHash);
            printf("A %s\n",clientManifestEntry.path);
            writeFileAppend(commitFd, toWrite);
            free(toWrite);
        // server has files that client does not
        } else if( i >= numClientManifestEntries ) {
            // write out add code
            char* toWrite = (char*)malloc(1 + 1 + 4 + 1 + strlen(clientManifestEntry.path) + 1 + strlen(clientHexHash) + 1);
            sprintf(toWrite, "D %d %s %s\n",clientManifestEntry.version+1,clientManifestEntry.path,clientHexHash);
            printf("D %s\n",clientManifestEntry.path);
            writeFileAppend(commitFd, toWrite);
            free(toWrite);
        // compare hashes and write out M if 
        // 1. the server and client have the same hash AND
        // 2. the live hash of the file is different from client hash
        } else if( strcmp(clientManifestEntry.hash,serverManifestEntry.hash) == 0 && strcmp(liveHexHash,clientManifestEntry.hash) != 0 ) {
            // write out modify code
            char* toWrite = (char*)malloc(1 + 1 + 4 + 1 + strlen(clientManifestEntry.path) + 1 + strlen(clientHexHash) + 1);
            sprintf(toWrite, "M %d %s %s\n",clientManifestEntry.version+1,clientManifestEntry.path,clientHexHash);
            printf("M %s\n",clientManifestEntry.path);
            writeFileAppend(commitFd, toWrite);
            free(toWrite);
        }
        // fail if 
        // 1. file in server has different hash than client AND 
        // 2. server has a greater file version than conflict
        if( serverManifestEntry.hash != NULL && strcmp(serverManifestEntry.hash,clientManifestEntry.hash) != 0 && serverManifestEntry.version > clientManifestEntry.version ) {
            failure = TRUE;
            free(liveHexHash);
            break;
        }
        free(liveHexHash);
    }
    if( failure ) {
        printf("Commit failed.\n");
        remove(commitPath);
    }
    
    // chmod
    chmod(commitPath, ARUR);

    // close
    close(commitFd);
    // free
    free(serverManifest);
    free(clientManifest);
    free(serverManifestEntries);
    free(clientManifestEntries);
    free(clientManifestCopy);
    free(serverManifestCopy);
    free(manifestPath);
    free(commitPath);
    free(updatePath);
    free(conflictPath);
}

void push( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for push! Project name is a required argument.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // build the path to Commit
    int commitPathLength = strlen(argv[2]) + strlen("/.Commit") + 1;
    char* commitPath = (char*)malloc(commitPathLength);
    memset(commitPath,'\0',commitPathLength);
    snprintf(commitPath,commitPathLength,"%s/.Commit",argv[2]);
    // fail if there is no .Commit file
    if( access(commitPath, F_OK) < 0 ) fatalError(".Commit does not exist.");

    // tell server we are starting push command (push:<project name>)
    char* tellPush = (char*)malloc(strlen("push:") + strlen(argv[2]) + 1);
    strcat(tellPush,"push:");
    strcat(tellPush,argv[2]);
    send(sock, tellPush, strlen(tellPush) + 1, 0);
    // send over commit file
    char* commitFileSend = concatFileSpecsWithPath(commitPath,argv[2]);
    send(sock, commitFileSend, strlen(commitFileSend) + 1, 0);
    // read commit file
    char* commitContents = readFile(commitPath);
    // send over every file in .Commit
    // tokenize commit
    char* savePtrCommitEntry;
    // copy commitContents for use with strtok
    char* commitContentsCopy = (char*)malloc(strlen(commitContents)+1);
    strcpy(commitContentsCopy,commitContents);
    char* commitEntry = strtok_r(commitContentsCopy,"\n",&savePtrCommitEntry);
    while( commitEntry != NULL ) {
        char* savePtrCommitEntryData;
        // tokenize parts within commit entry
        char* changeCode = strtok_r(commitEntry," ",&savePtrCommitEntryData);
        int version = atoi(strtok_r(NULL," ",&savePtrCommitEntryData));
        char* filepath = strtok_r(NULL," ",&savePtrCommitEntryData);
        char* hash = strtok_r(NULL," ",&savePtrCommitEntryData);
        // get the file contents and send them to server
        char* filecontentFormatted = concatFileSpecsWithPath(filepath, argv[2]);
        if( DEBUG ) printf("Sent to server: %s\n",filecontentFormatted);
        send(sock, filecontentFormatted, strlen(filecontentFormatted) + 1, 0);
        free(filecontentFormatted);
        // get next entry
        commitEntry = strtok_r(NULL,"\n",&savePtrCommitEntry);
    }
    // tell server we are done sending files
    send(sock,"done;",strlen("done;")+1,0);
    // get server success
    char maybeSucc[5];
    read(sock,maybeSucc,5);
    if( strcmp(maybeSucc,"fail") == 0 ) {
        printf("Push failed.\n");
    }
    // get manifest from remote
    int manifestRequestLength = strlen("manifest:") + strlen(argv[2]) + 1;
    char* manifestRequest = (char*)malloc(manifestRequestLength);
    memset(manifestRequest,'\0',manifestRequestLength);
    strcat(manifestRequest,"manifest:");
    strcat(manifestRequest,argv[2]);
    send(sock,manifestRequest,manifestRequestLength,0);
    // read manifest from socket
    char* serverManifest = readManifestFromSocket(sock);
    // build manifest path
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    sprintf(manifestPath,"%s/.Manifest",argv[2]);
    writeFile(manifestPath,serverManifest);
    // remove commit on either response
    remove(commitPath);
    // free
    free(serverManifest);
    free(manifestRequest);
    free(manifestPath);
    free(commitContents);
    free(commitFileSend);
    free(commitPath);
}

void create( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for create! Project name is a required argument.");

    // check if the project exists on the server
    doesntProjectExist(sock, argv[2]);
    
    // build request string
    char* createRequest = (char*)malloc(strlen(argv[2]) + strlen("create:") + 1);
    memset(createRequest,'\0',strlen(createRequest));
    strcat(createRequest,"create:");
    strcat(createRequest,argv[2]);
    // send request
    send(sock, createRequest, strlen(createRequest)+1, 0);

    // create folder locally
    int makeProjectFolderCommandLength = strlen("mkdir ") + strlen(argv[2]) + 1;
    char* makeProjectFolderCommand = (char*)malloc(makeProjectFolderCommandLength);
    sprintf(makeProjectFolderCommand,"mkdir %s",argv[2]);
    system(makeProjectFolderCommand);

    // write manifest to that folder
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    sprintf(manifestPath,"%s/.Manifest",argv[2]);
    writeFile(manifestPath,"0\n");

    // chmod
    chmod(manifestPath,ARUR);
    
    // free
    free(createRequest);
    free(makeProjectFolderCommand);
}

void destroy( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for destroy! Project name is a required argument.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // build request string
    char* destroyRequest = (char*)malloc(strlen(argv[2]) + strlen("destroy:") + 1);
    memset(destroyRequest,'\0',strlen(destroyRequest));
    strcat(destroyRequest,"destroy:");
    strcat(destroyRequest,argv[2]);
    // send request
    send(sock, destroyRequest, strlen(destroyRequest)+1, 0);
}

void add( int argc, char** argv ) {
    if( argc != 4 ) fatalError("Too few or too many arguments for add! Project name and filename are required arguments.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // build path to manifest
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    memset(manifestPath,'\0',manifestPathLength);
    snprintf(manifestPath,manifestPathLength,"%s/.Manifest",argv[2]);
    // append to manifest file
    int manifestFd = open(manifestPath, O_RDWR | O_APPEND );
    // if manifest doesn't exist, exit
    if( manifestFd < 0 ) fatalError(".Manifest was not found. Make sure there is a project in this directory.");

    // calculate hash of file
    char* hashTransposed = convertHash(argv[3]);
    // build string we are going to write
    int toWriteSize = strlen("0") + 1 + strlen(argv[3]) + 1 + strlen(hashTransposed) + 1;
    char* toWrite = (char*)malloc(toWriteSize);
    memset(toWrite,'\0',toWriteSize);
    sprintf(toWrite,"0 %s %s\n",argv[3],hashTransposed);
    // write string
    writeFileAppend(manifestFd, toWrite);
    // sort manifest
    sortManifest(argv[2]);
    // free
    free(hashTransposed);
}

void removePr( int argc, char** argv ) {
    if( argc != 4 ) fatalError("Too few or too many arguments for remove! Project name and filename are required arguments.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // build path to manifest
    int manifestPathLength = strlen(argv[2]) + strlen("/.Manifest") + 1;
    char* manifestPath = (char*)malloc(manifestPathLength);
    memset(manifestPath,'\0',manifestPathLength);
    snprintf(manifestPath,manifestPathLength,"%s/.Manifest",argv[2]);
    // remove entry
    removeEntryFromManifest(manifestPath, argv[3]);
}

void currentVersion( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for current version! Project name is a required argument.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);
    // tell server we want current version
    int tellVersionSize = strlen("current version:") + strlen(argv[2]) + 1;
    char* tellVersion = (char*)malloc(tellVersionSize);
    memset(tellVersion,'\0',tellVersionSize);
    strcat(tellVersion,"current version:");
    strcat(tellVersion,argv[2]);
    send(sock,tellVersion,tellVersionSize,0);
    // read manifest from server
    char* manifestContent = readManifestFromSocket(sock);
    // for each line in the manifest, print it out
    char* savePtrManifest;
    char* manifestVersion = strtok_r(manifestContent,"\n",&savePtrManifest);
    printf("Project version: %s\n",manifestVersion);
    char* manifestEntry = strtok_r(NULL,"\n",&savePtrManifest);
    while( manifestEntry != NULL ) {
        // tokenize each entry for version and path
        char* savePtrManifestEntry;
        char* version = strtok_r(manifestEntry," ",&savePtrManifestEntry);
        char* path = strtok_r(NULL," ",&savePtrManifestEntry);
        // print out version + path
        printf("version: %s\tpath: %s\n", version, path);
        // move pointer
        manifestEntry = strtok_r(NULL,"\n",&savePtrManifest);
    }
    // free
    free(manifestContent);
}

void history( int argc, char** argv ) {
    if( argc != 3 ) fatalError("Too few or too many arguments for history! Project name is a required argument.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // ask for history
    int historyRequestLength = strlen("history:") + strlen(argv[2]) + 1;
    char* historyRequest = (char*)malloc(historyRequestLength);
    memset(historyRequest,'\0',historyRequestLength);
    sprintf(historyRequest,"history:%s",argv[2]);
    send(sock,historyRequest,historyRequestLength,0);

    // save history
    rwFileFromSocket(sock);
    
    // build path to history
    int historyPathLength = strlen(argv[2]) + strlen("/.History") + 1;
    char* historyPath = (char*)malloc(historyPathLength);
    sprintf(historyPath,"%s/.History",argv[2]);
    // print out history
    char* history = readFile(historyPath);
    printf("History for project %s:\n%s\n",argv[2],history);

    // remove history
    remove(historyPath);

    // free
    free( historyRequest );
    free( historyPath );
    free( history );
}

void rollback( int argc, char** argv ) {
    if( argc != 4 ) fatalError("Too few or too many arguments for rollback! Project name and version are required arguments.");

    // check if the project exists on the server
    doesProjectExist(sock, argv[2]);

    // send rollback request
    int rollbackRequestLength = strlen("rollback:") + strlen(argv[2]) + 1;
    char* rollbackRequest = (char*)malloc(rollbackRequestLength);
    memset(rollbackRequest,'\0',rollbackRequestLength);
    sprintf(rollbackRequest,"rollback:%s",argv[2]);
    send(sock,rollbackRequest,rollbackRequestLength,0);

    // tell the server what version
    send(sock,argv[3],strlen(argv[3])+1,0);

    // free
    free(rollbackRequest);
}