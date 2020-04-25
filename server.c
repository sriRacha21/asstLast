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
#include "requestUtils.h"
#include "fileIO.h"

#define MAX_THREADS 60

void* clientThread(void* use);

//char buffer[1024] = {0};
char clientMessage[256] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* clientThread(void* use){ //handles each client thread individually via multithreading
    printf("Thread successfully started for client socket.\n");
    int new_socket = *((int*) use);

    pthread_mutex_lock(&lock);

    //client operations below, the server will act accordingly to the client's needs based on the message sent from the client.
    while(1){
        int prefixLength; //variable made so getProjectName() can appropriately find the substring of the project name based on client message
        char* pName; //project name
        int valread = read(new_socket, clientMessage, 1024);
        if(valread < 0){
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        if(strcmp(clientMessage, "finished") == 0) break; //client signals it is done so get out and close thread

        //given "manifest:<project name>" sends the .manifest of a project as a char*
        if(strstr(clientMessage, "manifest:") != NULL){
            prefixLength = 9;
            pName = getProjectName(clientMessage, prefixLength);
            chdir(pName);
            char* manifestContents = concatFileSpecs(".Manifest", pName);
            send(new_socket, manifestContents, sizeof(char) * strlen(manifestContents), 0);
            free(manifestContents);
            free(pName);
        }

        //given "project file:<project name>" by client, sends "<filesize>;<filepath>;<file content>" for project
        else if(strstr(clientMessage, "project file:") != NULL){
            prefixLength = 13;
            pName = getProjectName(clientMessage, prefixLength);
            sendProjectFiles(pName, new_socket);
            send(new_socket, "done;", sizeof(char) * strlen("done;"), 0);
            free(pName);
        }

        //given "project:<project name>" by client, sends 1 if project exists and 0 if it does not exist
        else if(strstr(clientMessage, "project:") != NULL){
            prefixLength = 8;
            pName = getProjectName(clientMessage, prefixLength);
            projectExists(pName, new_socket); //sends "exists" if project exists and "doesnt" if it doesnt exist
            free(pName);
        }
    }
    
    pthread_mutex_unlock(&lock);
    printf("Exited new client thread.\n");
    close(new_socket);
    pthread_exit(NULL);
}


int main(int argc, char** argv){
    int PORT;
    if(argc != 2){
        perror("Invalid arguments");
        exit(EXIT_FAILURE);
    }
    else{
        PORT = atoi(argv[1]);
        if(PORT < 0 || PORT > 65535){
            perror("Invalid port number");
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

    if(listen(server_fd, 3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    pthread_t threadID[60];
    int i = 0;

    printf("Server successfully started.\n");
    while(1){ //loop accepts up to 60 client connections and creates a thread for each one, each client socket is passed to clientThread() for further operation
        if((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addLength)) < 0){
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if(pthread_create(&threadID[i], NULL, clientThread, &new_socket) != 0){
            perror("Thread creation failure");
            exit(EXIT_FAILURE);
        }

        if(i >= MAX_THREADS){
            i = 0;
            while(i < MAX_THREADS) pthread_join(threadID[i++], NULL);
            i = 0;
        }
    }
    
    return 0;
}