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
#define PORT 8080 
#define MAX_THREADS 75

char* getProjectName(char* msg, int prefixLength);
int projectExists(char* projectName);
void* clientThread(void* use);

//char buffer[1024] = {0};
char clientMessage[512] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

char* getProjectName(char* msg, int prefixLength){
    int newLength = strlen(msg) - prefixLength;
    char* pName = malloc(sizeof(char) * newLength+1);
    strncpy(pName, msg+prefixLength, newLength);
    pName[newLength] = '\0'; //need null terminator to end string
    return pName;
}

int projectExists(char* projectName){//goes through current directory and tries to find if projectName exists
    struct dirent* dirPointer;
    DIR* currentDir = opendir("."); //idk if this is right
    if(currentDir == NULL){
        perror("Can't open directory");
        exit(EXIT_FAILURE);
    }
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(projectName, dirPointer->d_name) == 0) return 1; //exists
    }
    return 0; //nope
}

void* clientThread(void* use){
    int new_socket = *((int*) use);
    int valread = read(new_socket, clientMessage, 1024);
    if(valread < 0){
        perror("Read error");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&lock);
    printf("Server message: %s\n", clientMessage);

    //client operations below, the server will act accordingly to the client's needs based on the message sent from the client.
    int prefixLength; //variable made so getProjectName() can appropriately find the substring of the project name based on client demand

    //given "manifest:<project name>" sends the .manifest of a project
    if(strstr(clientMessage, "manifest:") != NULL){
        prefixLength = 9;
    }

    //given "project file:<project name>" by client, sends "<filesize>;<filepath>;<file content>" for project
    else if(strstr(clientMessage, "project file:") != NULL){
        prefixLength = 13;
    }

    //given "project:<project name>" by client, sends 1 if project exists and 0 if it does not exist
    else if(strstr(clientMessage, "project:") != NULL){
        char* pName = getProjectName(clientMessage, 8);
        int exists = projectExists(pName);
        if(exists == 1) send(new_socket, "1", 1, 0); //project exists, sending "1" to client
        else send(new_socket, "0", 1, 0); //project doesnt exist, sending "0" to client
    }
    pthread_mutex_unlock(&lock);
    printf("Exited new client thread.\n");
    close(new_socket);
    pthread_exit(NULL);
}


int main(int argc, char** argv){
    int server_fd, new_socket;// valread;
    struct sockaddr_in address;
    int opt = 1;
    int addLength = sizeof(address);
    //char* hello = "Hello from server";

    //create socket fd
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    //attach socket to port 8080
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

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
    while(1){
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

        /*valread = read(new_socket, buffer, 1024);
        printf("%s\n", buffer);
        send(new_socket, hello, strlen(hello), 0);
        printf("Send hello \n");*/
    }
    
    return 0;
}