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
    system("./WTFserver 6969");
}

void* clientRoutine(void* arg){
    // configure the client
    system("./WTF configure 127.0.0.1 6969");
    // create the initial project
    system("./WTF create testFolder");
    // write out files to the project, simulating a user adding files to their new project
    system("echo \"i uckin melted ya\" > client/testFolder/melt.txt ");
    system("echo \"wow\" > client/testFolder/somethin.txt");
    system("mkdir client/testFolder/testSub");
    system("echo -e \"kibun shidai desu boku wa\nsonna boku o oite\nare kara sekai wa kawattatte \" > client/testFolder/testSub/asunoyo.txt");
    system("echo \"another on\" > client/testFolder/testSub/banotherone.txt");
    system("echo \"there would be some C code here if I wasn't so lazy\" > client/testFolder/testSub/test.c");
    system("./WTF update testFolder");
    system("./WTF upgrade testFolder");
    system("./WTF add testFolder testFolder/testSub/asunoyo.txt");
    system("./WTF add testFolder testFolder/testSub/banotherone.txt");
    system("./WTF add testFolder testFolder/testSub/test.c");
    system("./WTF add testFolder testFolder/melt.txt");
    system("./WTF add testFolder testFolder/somethin.txt");
    system("./WTF commit testFolder");
    system("./WTF push testFolder");
    //modifying existing client files for another push
    system("echo \"appending this here\" >> client/testFolder/melt.txt");
    system("echo \"overwriting these here thang\" > client/testFolder/testSub/asunoyo.txt");
    return;
    system("./WTF commit testFolder");
    system("./WTF push testFolder");
    system("./WTF currentversion testFolder");
    system("./WTF history testFolder");
    system("./WTF destroy testFolder");
    //delete the file in order to test checkout and rollback
    system("rm -rf client/testFolder");
    system("./WTF rollback testFolder 0");
    system("./WTF checkout testFolder");
}
