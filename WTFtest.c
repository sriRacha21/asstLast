#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <pthread.h>

void* serverRoutine(void* arg);
void* clientRoutine(void* arg);

struct threadargs{
    char* port;
};

int main(int argc, char** argv){
    //REQUIRES ARGUMENT OF PORT NUMBER
    pthread_t ptid;
    pthread_t ptid2;

    if(argc != 2){ //these if statements look for a valid port number as the argument in argv[1]
        printf("Invalid argument count, need portnum for server\n");
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
        int PORT = atoi(argv[1]);
        if(PORT < 0 || PORT > 65535){
            printf("Valid port numbers are between 0 and 65535\n");
            exit(EXIT_FAILURE);
        }
    }

    struct threadargs* toPass = (struct threadargs*)malloc(sizeof(struct threadargs));
    toPass->port = argv[1];

    pthread_create(&ptid, NULL, serverRoutine, (void*)toPass);
    sleep(1);
    pthread_create(&ptid2, NULL, clientRoutine, NULL);

    pthread_join(ptid, NULL);
    pthread_join(ptid2, NULL);
    printf("Test concluded.\n");
    return 0;
}

void* serverRoutine(void* arg){
    char port[256] = {0};
    strcat(port, "./WTFserver ");
    char hold[256] = {0};
    strcat(port, ((struct threadargs*)arg)->port);
    system(port);
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
    system("echo \"appending this here\" >> server/testFolder/melt.txt");
    system("echo \"overwriting these here thang\" > server/testFolder/testSub/asunoyo.txt");
    system("./WTF update testFolder");
    system("./WTF upgrade testFolder");
    system("./WTF currentversion testFolder");
    system("./WTF history testFolder");
    system("./WTF destroy testFolder");
    //delete the file in order to test checkout and rollback
    system("chmod -R 777 client/testFolder");
    system("rm -rf client/testFolder");
    system("./WTF rollback testFolder 1");
    return;
    system("./WTF checkout testFolder");
    system("killall WTFserver");
}
