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
#include "fileIO.c"
#define PORT 6969 
#define MAX_THREADS 75

void sendProjectFiles(char* projectName, int socket);
char* concatFileSpecs(char* fileName, char* projectName); //0 = <size>;<content> 1 = <size>;<path>;<content>
char* concatFileSpecsWithPath(char* fileName, char* projectName);
int lengthOfInt(int num);
int getFileSize(char* fileName);
char* getProjectName(char* msg, int prefixLength);
int projectExists(char* projectName);
void* clientThread(void* use);

//char buffer[1024] = {0};
char clientMessage[256] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int dirType(char* dName){ //this doesnt work correctly right now
    DIR* currentDir = opendir(dName);
    if(currentDir != NULL){
        closedir(currentDir);
        return 0; //its a directory
    }
    else if(errno == ENOTDIR) return 1;
    return -342; //???? somethin happened here and it aint good lmao
}

void sendProjectFiles(char* projectName, int socket){
    char path[256];
    struct dirent* dirPointer;
    DIR* currentDir = opendir(projectName);
    if(currentDir == NULL){
        perror("Can't open directory");
        exit(EXIT_FAILURE);
    }
    while((dirPointer = readdir(currentDir)) != NULL){
        if(strcmp(dirPointer->d_name, ".") != 0 && strcmp(dirPointer->d_name, "..") != 0){
            if(dirPointer->d_type == 8){
                char* fileSpecs = concatFileSpecs(dirPointer->d_name, projectName);
                send(socket, fileSpecs, sizeof(char) * strlen(fileSpecs), 0);
                free(fileSpecs);
            }
            strcpy(path, projectName);
            strcat(path, "/");
            strcat(path, dirPointer->d_name);
            sendProjectFiles(path, socket);
        }
    }
    closedir(currentDir);
}

char* concatFileSpecs(char* fileName, char* projectName){
    int fileSize = getFileSize(fileName); //size in bytes
    int fileSizeIntLength = lengthOfInt(fileSize); //used for string concatenation and to convert to string
    char* fileSizeStr = malloc(sizeof(char) * fileSizeIntLength); //allocate char array for int to string conversion
    sprintf(fileSizeStr, "%d", fileSize); //convert int of file size (bytes) to a string
    char* fileContents = readFile(fileName); //read in file contents
    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileContents) + 1 + fileSizeIntLength + 1)); //allocate 
    //building string to return
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        exit(EXIT_FAILURE);
    }
    free(fileSizeStr);
    free(fileContents);
    return fullFileSpecs; 
}

char* concatFileSpecsWithPath(char* fileName, char* projectName){
    int fileSize = getFileSize(fileName); 
    int fileSizeIntLength = lengthOfInt(fileSize); 
    char* fileSizeStr = malloc(sizeof(char) * fileSizeIntLength); 
    sprintf(fileSizeStr, "%d", fileSize); 
    char* fileContents = readFile(fileName); 
    //finding relative path via string manip on absolute path
    char* absolutePath = realpath(fileName, NULL);
    int startIndex = strstr(absolutePath, projectName) - absolutePath;
    int pathLength = strlen(absolutePath) - startIndex;
    char* relativePath = malloc(sizeof(char) * pathLength);
    int i;
    for(i = 0; i < pathLength; i++){
        relativePath[i] = absolutePath[i+startIndex];
    }
    //creating string to return
    char* fullFileSpecs = malloc(sizeof(char) * (strlen(fileContents) + 1 + pathLength + 1 + fileSizeIntLength + 1)); 
    strcat(fullFileSpecs, fileSizeStr);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, relativePath);
    strcat(fullFileSpecs, ";");
    strcat(fullFileSpecs, fileContents);
    fullFileSpecs[strlen(fullFileSpecs)] = '\0';
    if(fileContents == NULL || fullFileSpecs == NULL){
        perror("File read error");
        exit(EXIT_FAILURE);
    }
    free(fileSizeStr);
    free(fileContents);
    free(absolutePath);
    free(relativePath);
    return fullFileSpecs; 
}

int lengthOfInt(int num){
    int i = 0;
    while(num > 0){
        num /= 10;
        i ++;
    }
    return i;
}

int getFileSize(char* fileName){
    int fp = open(fileName, O_RDONLY);
    if(fp < 0) return -1; //error
    int position = (int) lseek(fp, 0, SEEK_END);
    lseek(fp, 0, SEEK_SET);
    close(fp);
    return position+1;
}

char* getProjectName(char* msg, int prefixLength){
    int newLength = strlen(msg) - prefixLength;
    char* pName = malloc(sizeof(char) * (newLength+1));
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
        if(strcmp(projectName, dirPointer->d_name) == 0){
            closedir(currentDir);
            return 1; //exists
        } 
    }
    closedir(currentDir);
    return 0; //nope
}

void* clientThread(void* use){ //handles each client thread individually via multithreading
    printf("Thread successfully started for client socket.\n");
    int new_socket = *((int*) use);
    int valread = read(new_socket, clientMessage, 1024);
    if(valread < 0){
        perror("Read error");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&lock);
    printf("Server message: %s\n", clientMessage);

    //client operations below, the server will act accordingly to the client's needs based on the message sent from the client.
    int prefixLength; //variable made so getProjectName() can appropriately find the substring of the project name based on client message
    char* pName;

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
        int exists = projectExists(pName);
        if(exists == 1) send(new_socket, "1", 1, 0); //project exists, sending "1" to client
        else send(new_socket, "0", 1, 0); //project doesnt exist, sending "0" to client
        free(pName);
    }
    
    pthread_mutex_unlock(&lock);
    printf("Exited new client thread.\n");
    close(new_socket);
    pthread_exit(NULL);
}


int main(int argc, char** argv){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addLength = sizeof(address);

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
    while(1){ //loop accepts up to 75 client connections and creates a thread for each one, each client socket is passed to clientThread() for further operation
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