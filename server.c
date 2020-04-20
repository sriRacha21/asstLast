#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <pthread.h>
#define PORT 8080 
#define MAX_THREADS 75

char buffer[1024] = {0};
char clientMessage[1024] = {0};

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* clientThread(void* use){
    int new_socket = *((int*) use);
    int valread = read(new_socket, clientMessage, 1024);
    if(valread < 0){
        perror("Read error");
        exit(EXIT_FAILURE);
    }
    char* hello = "Hello from server";

    //client pooperations
    pthread_mutex_lock(&lock);
    printf("Server message: %s\n", clientMessage);
    pthread_mutex_unlock(&lock);
    send(new_socket, hello, strlen(hello), 0);
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