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


void* serverRoutine(void* arg);
void* clientRoutine(void* arg);

int main(int argc, char** argv){
    pthread_t ptid;
    pthread_t ptid2;

    pthread_create(&ptid, NULL, serverRoutine, NULL);
    sleep(1);
    pthread_create(&ptid2, NULL, clientRoutine, NULL);

    pthread_join(ptid, NULL);
    pthread_join(ptid2, NULL);
    printf("Test concluded.\n");
    return 0;
}

void* serverRoutine(void* arg){
    system("server/WTFserver 6969");
}

void* clientRoutine(void* arg){
    system("client/WTF configure 127.0.0.1 6969");
    system("client/WTF create testFolder");
    //system("client/WTF ")
}
