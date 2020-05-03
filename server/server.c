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
#include <ctype.h>
#include "exitLeaks.h"
#include "requestUtils.h"
#include "../fileIO.h"
#include "../manifestControl.h"

#define MAX_THREADS 60

void* clientThread(void* use);

char buffer[1024] = {0};
char clientMessage[256] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threadID[60];
int threadCounter = 0;

void* clientThread(void* use){ //handles each client thread individually via multithreading
    printf("Thread successfully started for client socket.\n");
    int new_socket = *((int*) use);

    pthread_mutex_lock(&lock); //this entire thread is a critical section screw it

    //client operations below, the server will act accordingly to the client's needs based on the message sent from the client.
    while(1){
        int prefixLength; //variable made so getProjectName() can appropriately find the substring of the project name based on client message
        clientMessage[0] = '\0';
        int valread = read(new_socket, clientMessage, 1024);
        if(valread < 0){
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        if(strcmp(clientMessage, "finished") == 0){
            //send(new_socket, "done", strlen("done") * sizeof(char), 0);
            printf("Received \"finished\", closing thread.\n");
            break; //client signals it is done so get out and close thread
        }

        //given a "push:<project name>", receives the commit then all the changed files from the client. makes changes accordingly
        if(strstr(clientMessage, "push:") != NULL){
            printf("Received \"push:<project name>\", pushing commit into history then taking pushed files.\n");
            prefixLength = 5;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s\n", getVariableData(variableList, "pName"));

            //need to archive old project



            //append commit to history, and return history contents
            variableList = insertExit(variableList, createNode("commitContents", rwCommitToHistory(new_socket, getVariableData(variableList, "pName")), 1));
            if(strcmp(getVariableData(variableList, "commitContents"), "ERRORERROR123SENDHELP") == 0){
                printf("Error in appending to history.\n");
                variableList = freeVariable(variableList, "pName");
                variableList = freeVariable(variableList, "commitContents");
                break;
            }

            //rewrite all modified/added files
            int rewriteSuccess = 42069;
            while(rewriteSuccess != -342){
                rewriteSuccess = rewriteFileFromSocket(new_socket);
            }

            //delete all files meant to be deleted, and also return the version number
            int newVersion = deleteFilesFromPush(getVariableData(variableList, "commitContents"));

            //remake manifest and fill it with the new files
            createManifest(getVariableData(variableList, "pName"), newVersion);

            printf("Received push.\n");
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "commitContents");
        }

        //given "manifest:<project name>" sends the .manifest of a project as a char*
        else if(strstr(clientMessage, "manifest:") != NULL){
            printf("Received \"manifest:<projectname>\", getting project name then sending manifest.\n");
            prefixLength = 9;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s\n", getVariableData(variableList, "pName"));
            chdir(getVariableData(variableList, "pName"));
            variableList = insertExit(variableList, createNode("manifestContents", concatFileSpecs(".Manifest", getVariableData(variableList, "pName")), 1));
            printf("Sending .Manifest data.\n");
            send(new_socket, getVariableData(variableList, "manifestContents"), sizeof(char) * strlen(getVariableData(variableList, "manifestContents")+1), 0);
            variableList = freeVariable(variableList, "manifestContents");
            variableList = freeVariable(variableList, "pName");
            chdir(".");
            printf("Finished sending manifest file.\n");
        }

        //given "specific project file:<project name>:<filepath>" sends <filesize>;<file path>;<file content> for that specific file
        else if(strstr(clientMessage, "specific project file:") != NULL){
            printf("Received \"specific project file:<projectname>:<filepath>\", sending file specs.\n");
            prefixLength = 22;
            variableList = insertExit(variableList, createNode("pName", specificFileStringManip(clientMessage, 22, 0), 1));
            variableList = insertExit(variableList, createNode("specFilePath", specificFileStringManip(clientMessage, 22, 1), 1));
            printf("Project name: %s. File's filepath: %s.\n", getVariableData(variableList, "pName"), getVariableData(variableList, "specFilePath"));
            variableList = insertExit(variableList, createNode("fileSpecs", 
                getSpecificFileSpecs(getVariableData(variableList, "pName"), getVariableData(variableList, "specFilePath")), 342));
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "specFilePath");
            printf("Received specs for specific file.  Sending file...\n");
            send(new_socket, getVariableData(variableList, "fileSpecs"), strlen(getVariableData(variableList, "fileSpecs")), 0);
            variableList = freeVariable(variableList, "fileSpecs");
            printf("Finished sending specs of specific file.\n");
        }

        //given "project file:<project name>" by client, sends "<filesize>;<filepath>;<file content>" for project, then "done"
        else if(strstr(clientMessage, "project file:") != NULL){
            printf("Received \"project file:<projectname>\", getting project name then sending all project files.\n");
            prefixLength = 13;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s\n", getVariableData(variableList, "pName"));
            printf("Sending project files...\n");
            sendProjectFiles(getVariableData(variableList, "pName"), new_socket);
            send(new_socket, "done", sizeof(char) * strlen("done") + 1, 0);
            variableList = freeVariable(variableList, "pName");
            printf("Finished sending project files.\n");
        }

        //given "project:<project name>" by client, sends 1 if project exists and 0 if it does not exist
        else if(strstr(clientMessage, "project:") != NULL){
            printf("Received \"project:<projectname>\", getting project name then sending whether or not it exists.\n");
            prefixLength = 8;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s\n", getVariableData(variableList, "pName"));
            int success = projectExists(getVariableData(variableList, "pName"), new_socket); //sends "exists" if project exists and "doesnt" if it doesnt exist
            variableList = freeVariable(variableList, "pName");
            printf("Finished verifying existence.\n");
            if(success == 0){
                printf("Project doesn't exist.  Closing thread...\n");
                break;
            }
            else printf("Project exists.\n");
        }

        //given "destroy:<project name>" by client, destroys project's files and subdirectories and sends "done" when finished
        else if(strstr(clientMessage, "destroy:") != NULL){
            printf("Received \"destroy:<project name>\", removing and files and subdirectories of project.\n");
            prefixLength = 8;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Destroying...\n", getVariableData(variableList, "pName"));
            int success = destroyProject(getVariableData(variableList, "pName"));
            variableList = freeVariable(variableList, "pName");
            if(success) printf("Project has been destroyed.  May it rest in peace.\n");
            else{
                printf("Could not destroy project. Closing thread...\n");
                break;
            }
            send(new_socket, "done", sizeof(char) * strlen("done") + 1, 0);
        }

        //given "current version:<project name> by client, retreives and sends project's current version from the active .Manifest"
        else if(strstr(clientMessage, "current version:") != NULL){
            printf("Received \"current version:<project name>\", fetching and sending current version of the project as listed in the .Manifest.\n");
            prefixLength = 16;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Getting version number...\n", getVariableData(variableList, "pName"));
            variableList = insertExit(variableList, createNode("currentVersion", checkVersion(getVariableData(variableList, "pName")), 0));
            printf("Version number: %s.  Sending...\n", getVariableData(variableList, "currentVersion"));
            send(new_socket, getVariableData(variableList, "currentVersion"), strlen(getVariableData(variableList, "currentVersion"))+1, 0);
            printf("Sent version number.\n");
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "currentVersion");
        }

        //given "create:<project name>" by client, retrieves project name, creates the folder, and then initializes a .Manifest that holds "0\n" and sends "done" after
        else if(strstr(clientMessage, "create:") != NULL){
            printf("Received \"create:<project name>\", fetching project name and creating directory and initializing .Manifest file.\n");
            prefixLength = 7;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Creating...\n", getVariableData(variableList, "pName"));
            createProjectFolder(getVariableData(variableList, "pName"));
            printf("Project has been created.\n");
            send(new_socket, "done", sizeof(char) * (strlen("done")+1), 0);
            variableList = freeVariable(variableList, "pName");
        }

        //given "history:<project name>" by client, retrives and sends the .History file belonging to the project
        else if(strstr(clientMessage, "history:") != NULL){
            printf("Received \"history:<project name>\", fetching then sending .History file for the project.\n");
            prefixLength = 8;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Getting history...\n", getVariableData(variableList, "pName"));
            chdir(getVariableData(variableList, "pName"));
            variableList = insertExit(variableList, createNode("historyContents", concatFileSpecsWithPath(".History", 
                getVariableData(variableList, "pName")), 0));
            send(new_socket, getVariableData(variableList, "historyContents"), strlen(getVariableData(variableList, "historyContents")+1), 0);
            printf("History has been sent.\n");
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "historyContents");
        }
    }
    
    freeAllMallocs(variableList);
    pthread_mutex_unlock(&lock);
    printf("Exited new client thread.\n");
    close(new_socket);
    pthread_exit(NULL);
}


int main(int argc, char** argv){
    atexit(cleanUp);
    variableList = NULL;

    int PORT;
    
    if(argc != 2){ //these if statements look for a valid port number as the argument in argv[1]
        printf("Invalid argument count\n");
        exit(EXIT_FAILURE);
    }
    else{
        int length = strlen(argv[1]);
        int j = 0;
        for(j = 0; j < length; j++){
            if(!isdigit(argv[1][j])){
                printf("Invalid argument, please enter valid port number.\n");
                exit(EXIT_FAILURE);
            }
        }
        PORT = atoi(argv[1]);
        if(PORT < 0 || PORT > 65535){
            printf("Valid port numbers are between 0 and 65535\n");
            exit(EXIT_FAILURE);
        }
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addLength = sizeof(address);

    //create socket fd
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    //attach socket to designated port
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Bind failure");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 60) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server successfully started.\nAwaiting connection...\n");
    while(1){ //loop accepts up to 60 client connections and creates a thread for each one, each client socket is passed to clientThread() for further operation
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addLength)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if(pthread_create(&threadID[threadCounter], NULL, clientThread, &new_socket) != 0){
            perror("Thread creation failure");
            exit(EXIT_FAILURE);
        }

        if(threadCounter >= MAX_THREADS){
            threadCounter = 0;
            while(threadCounter < MAX_THREADS) pthread_join(threadID[threadCounter++], NULL);
            threadCounter = 0;
        }
    }
    
    return 0;
}