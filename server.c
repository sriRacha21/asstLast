#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <sys/stat.h>
#include <netinet/in.h> 
#include <pthread.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include "exitLeaks.h"
#include "requestUtils.h"
#include "fileIO.h"
#include "manifestControl.h"

void* clientThread(void* use);

char clientMessage[1024] = {0};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* clientThread(void* use){ //handles each client thread individually via multithreading
    printf("Thread successfully started for client socket.\n");
    int new_socket = *((int*) use);

    //client operations below, the server will act accordingly to the client's needs based on the message sent from the client.
    while(1){
        int prefixLength; //variable made so getProjectName() can appropriately find the substring of the project name based on client message
        clientMessage[0] = '\0';
        int valread = read(new_socket, clientMessage, 1024);
        if(valread < 0){
            perror("Read error");
            break;
        }

        if(strcmp(clientMessage, "finished") == 0){
            //send(new_socket, "done", strlen("done") * sizeof(char), 0);
            printf("Received \"finished\", closing thread.\n");
            break; //client signals it is done so get out and close thread
        }

        //given a "push:<project name>", receives the commit then all the changed files from the client. makes changes accordingly
        if(strstr(clientMessage, "push:") != NULL){
            pthread_mutex_lock(&lock);
            printf("Received \"push:<project name>\", pushing commit into history then taking pushed files.\n");
            prefixLength = 5;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            //printf("Project name: %s\n", getVariableData(variableList, "pName"));

            //need to archive old project
            char copyPath[256] = {0};
            strcat(copyPath, "cp -r ");
            strcat(copyPath, getVariableData(variableList, "pName"));
            strcat(copyPath, " backups");
            system(copyPath);

            //append commit to history, and return history contents
            variableList = insertExit(variableList, createNode("commitContents", rwCommitToHistory(new_socket, getVariableData(variableList, "pName")), 1));
            if(strcmp(getVariableData(variableList, "commitContents"), "ERRORERROR123SENDHELP") == 0){
                printf("Error in appending to history.\n");
                variableList = freeVariable(variableList, "pName");
                variableList = freeVariable(variableList, "commitContents");
                pthread_mutex_unlock(&lock);
                break;
            }

            //rewrite all modified/added files
            int rewriteSuccess = 42069;
            int breakOuter = 0;
            while(rewriteSuccess != -342){
                if(rewriteSuccess == -5){
                    send(new_socket, "fail", sizeof(char) * strlen("fail"), 0);
                    printf("failure on push\n");
                    breakOuter = 1;
                }
                if(breakOuter == 1) break;
                rewriteSuccess = rewriteFileFromSocket(new_socket);
            }
            if(breakOuter == 1){
                pthread_mutex_unlock(&lock);
                break;
            }

            //delete all files meant to be deleted, and also return the version number, which is the highest version number given in the commit
            int newVersion = deleteFilesFromPush(getVariableData(variableList, "commitContents"));

            char version[11];
            sprintf(version, "%d", newVersion);
            version[strlen(version)] = '\0';

            //tar it to compress (10 points E.C here TAs :) )
            memset(copyPath, '\0', 256);
            strcat(copyPath, "tar -czvf ");
            strcat(copyPath, getVariableData(variableList, "pName"));
            strcat(copyPath, "-");
            strcat(copyPath, version);
            strcat(copyPath, ".tar.gz backups/");
            strcat(copyPath, getVariableData(variableList, "pName")); //should end up as 'tar -czvf <projectname>-<versnum>.tar.gz in current directory
            system(copyPath); //tar -czvf <project>-<varsnum>.tar.gz backups/<project>

            //move to backups
            memset(copyPath, '\0', 256);
            strcat(copyPath, "mv ");
            strcat(copyPath, getVariableData(variableList, "pName"));
            strcat(copyPath, "-");
            strcat(copyPath, version);
            strcat(copyPath,".tar.gz backups");
            system(copyPath);

            //remove the copied non compressed file in backups
            memset(copyPath, '\0', 256);
            strcat(copyPath, "rm -rf backups/");
            strcat(copyPath, getVariableData(variableList, "pName"));
            system(copyPath);

            //update manifest
            commitToManifest(getVariableData(variableList, "pName"),
             getVariableData(variableList, "commitContents"), newVersion);

            printf("Received push.\n");
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "commitContents");
            send(new_socket, "succ", sizeof(char) * strlen("succ")+1, 0);
            pthread_mutex_unlock(&lock);
        }

        //given "manifest:<project name>" sends the .manifest of a project as a char*
        else if(strstr(clientMessage, "manifest:") != NULL){
            printf("Received \"%s\", getting project name then sending manifest.\n", clientMessage);
            prefixLength = 9;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            //printf("Project name: %s\n", getVariableData(variableList, "pName"));
            chdir(getVariableData(variableList, "pName"));
            variableList = insertExit(variableList, createNode("manifestContents", concatFileSpecs(".Manifest", getVariableData(variableList, "pName")), 1));
            printf("Sending .Manifest data.\n");
            send(new_socket, getVariableData(variableList, "manifestContents"), sizeof(char) * strlen(getVariableData(variableList, "manifestContents")+1), 0);
            variableList = freeVariable(variableList, "manifestContents");
            variableList = freeVariable(variableList, "pName");
            chdir("../");
            printf("Finished sending manifest file.\n");
        }

        //given "specific project file:<project name>:<filepath>" sends <filesize>;<file path>;<file content> for that specific file
        else if(strstr(clientMessage, "specific project file:") != NULL){
            printf("Received \"%s\", sending file specs.\n", clientMessage);
            prefixLength = 22;
            variableList = insertExit(variableList, createNode("pName", specificFileStringManip(clientMessage, 22, 0), 1));
            variableList = insertExit(variableList, createNode("specFilePath", specificFileStringManip(clientMessage, 22, 1), 1));
            //printf("Project name: %s. File's filepath: %s.\n", getVariableData(variableList, "pName"), getVariableData(variableList, "specFilePath"));
            variableList = insertExit(variableList, createNode("fileSpecs", 
                getSpecificFileSpecs(getVariableData(variableList, "pName"), getVariableData(variableList, "specFilePath")), 342));
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "specFilePath");
            printf("Received specs for specific file.  Sending file...\n");
            send(new_socket, getVariableData(variableList, "fileSpecs"), strlen(getVariableData(variableList, "fileSpecs")+1), 0);
            variableList = freeVariable(variableList, "fileSpecs");
            printf("Finished sending specs of specific file.\n");
        }

        //given "project file:<project name>" by client, sends "<filesize>;<filepath>;<file content>" for project, then "done"
        else if(strstr(clientMessage, "project file:") != NULL){
            printf("Received \"%s\", getting project name then sending all project files.\n", clientMessage);
            prefixLength = 13;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            //printf("Project name: %s\n", getVariableData(variableList, "pName"));
            printf("Sending project files...\n");
            sendProjectFiles(getVariableData(variableList, "pName"), new_socket);
            sleep(5); //need to let the recursion finish before sending done
            send(new_socket, "done;", sizeof(char) * strlen("done;") + 1, 0);
            variableList = freeVariable(variableList, "pName");
            printf("Finished sending project files.\n");
            break;
        }

        //given "project:<project name>" by client, sends 1 if project exists and 0 if it does not exist
        else if(strstr(clientMessage, "project:") != NULL){
            printf("Received \"%s\", getting project name then sending whether or not it exists.\n", clientMessage);
            prefixLength = 8;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            //printf("Project name: %s\n", getVariableData(variableList, "pName"));
            int success = projectExists(getVariableData(variableList, "pName"), new_socket); //sends "exists" if project exists and "doesnt" if it doesnt exist
            variableList = freeVariable(variableList, "pName");
            printf("Finished verifying existence.\n");
            if(success == 0){
                printf("Project doesn't exist.  Closing thread...\n");
                // break;
            }
            else printf("Project exists.\n");
        }

        //given "destroy:<project name>" by client, destroys project's files and subdirectories and sends "done" when finished
        else if(strstr(clientMessage, "destroy:") != NULL){
            pthread_mutex_lock(&lock);
            printf("Received \"%s\", destroying project.\n", clientMessage);
            prefixLength = 8;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Destroying...\n", getVariableData(variableList, "pName"));
            // chmod whole folder
            char* chmodStr = (char*)malloc(strlen("chmod -R 777 ") + strlen(getVariableData(variableList,"pname")) + 1);
            sprintf(chmodStr,"chmod -R 777 %s ",getVariableData(variableList,"pName"));
            system(chmodStr);
            //
            // chmod(getVariableData(variableList, "pName"), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            char destructionPath[256] = {0};
            strcat(destructionPath, "rm -rf ");
            strcat(destructionPath, getVariableData(variableList, "pName"));
            system(destructionPath);
            variableList = freeVariable(variableList, "pName");
            printf("Project has been destroyed.  May it rest in peace.\n");
            pthread_mutex_unlock(&lock);
        }

        //given "current version:<project name> by client, retreives and sends project's current version from the active .Manifest"
        else if(strstr(clientMessage, "current version:") != NULL){
            printf("Received \"%s\", fetching and sending...\n", clientMessage);
            prefixLength = 16;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
           // printf("Project name: %s.  Sending project state\n", getVariableData(variableList, "pName"));
            /*variableList = insertExit(variableList, createNode("currentVersion", checkVersion(getVariableData(variableList, "pName")), 0));
            printf("Version number: %s.  Sending...\n", getVariableData(variableList, "currentVersion"));
            send(new_socket, getVariableData(variableList, "currentVersion"), strlen(getVariableData(variableList, "currentVersion"))+1, 0);*/
            //checkVersion(getVariableData(variableList, "pName"), new_socket);
            char path[256] = {0};
            strcat(path, getVariableData(variableList, "pName"));
            strcat(path, "/.Manifest");
            variableList = insertExit(variableList, createNode("manifestContents", concatFileSpecs(path, 
            getVariableData(variableList, "pName")), 0));
            //printf("manifest: %s\n", getVariableData(variableList, "manifestContents"));
            send(new_socket, getVariableData(variableList, "manifestContents"), strlen(getVariableData(variableList, "manifestContents"))+1, 0);
            printf("Sent current version.\n");

            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "manifestContents");
        }

        //given "create:<project name>" by client, retrieves project name, creates the folder, and then initializes a .Manifest that holds "0\n" and sends "done" after
        else if(strstr(clientMessage, "create:") != NULL){
            pthread_mutex_lock(&lock);
            printf("Received \"%s\", fetching project name and creating directory and initializing .Manifest file.\n", clientMessage);
            prefixLength = 7;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Creating...\n", getVariableData(variableList, "pName"));
            createProjectFolder(getVariableData(variableList, "pName"));
            printf("Project has been created.\n");
            variableList = freeVariable(variableList, "pName");
            pthread_mutex_unlock(&lock);
        }

        //given "history:<project name>" by client, retrives and sends the .History file belonging to the project
        else if(strstr(clientMessage, "history:") != NULL){
            printf("Received \"%s\", fetching then sending .History file for the project.\n", clientMessage);
            prefixLength = 8;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));
            printf("Project name: %s.  Getting history...\n", getVariableData(variableList, "pName"));
            chdir(getVariableData(variableList, "pName"));
            variableList = insertExit(variableList, createNode("historyContents", concatFileSpecsWithPath(".History", 
                getVariableData(variableList, "pName")), 0));
            printf("%s\n", getVariableData(variableList, "historyContents"));
            send(new_socket, getVariableData(variableList, "historyContents"), strlen(getVariableData(variableList, "historyContents")+1), 0);
            printf("History has been sent.\n");
            variableList = freeVariable(variableList, "pName");
            variableList = freeVariable(variableList, "historyContents");
            chdir("..");
        }

        //given "rollback:<project name>" sends all the files in the version of that project.
        //<projectname>-<versnum>.tar.gz backups/<projectname>
        else if(strstr(clientMessage, "rollback:") != NULL){
            pthread_mutex_lock(&lock);
            printf("Received \"%s\", rolling server project to specified version.\n", clientMessage);
            int prefixLength = 9;
            variableList = insertExit(variableList, createNode("pName", getProjectName(clientMessage, prefixLength), 1));

            //read in version
            char version[11] = {0};
            read(new_socket, version, 11);
            int versionNum = atoi(version);

            char pathName[256] = {0};
            //first, delete the current project folder
            strcat(pathName, "rm -rf ");
            strcat(pathName, getVariableData(variableList, "pName"));
            system(pathName);

            //now, untar the specified version's tar, it puts it into the backups folder.
            pathName[0] = '\0';
            strcat(pathName, "tar -zxvf backups/");
            strcat(pathName, getVariableData(variableList, "pName"));
            strcat(pathName, "-");
            strcat(pathName, version);
            strcat(pathName, ".tar.gz");
            system(pathName);

            //move it from backups to working
            pathName[0] = '\0';
            strcat(pathName, "mv backups/");
            strcat(pathName, getVariableData(variableList, "pName"));
            strcat(pathName, " .");

            //now go to the backups folder and delete anything with a bigger version number than what was specified.
            struct dirent* dirPointer;
            DIR* currentDir = opendir("backups");
            if(currentDir == NULL){
                //uh oh
                printf("Can't open backups");
                pthread_mutex_unlock(&lock);
                break;
            }
            int pass3 = -1;
            while((dirPointer = readdir(currentDir)) != NULL){
                int i, j;
                int pass1 = 1; int pass2 = 1; 
                pathName[0] = '\0';
                strcat(pathName, "backups/");
                char tempPath[256] = {0};
                strcat(tempPath, dirPointer->d_name);

                //check if the backup is one a backup of the specified projects
                for(i = 0; i < strlen(getVariableData(variableList, "pName")); i++){
                    if(tempPath[i] != getVariableData(variableList, "pName")[i]){
                        pass1 = -1;
                        break;
                    }
                }

                //check the version of the new thing
                char secondVersion[11] = {0};
                for(j = strlen(getVariableData(variableList, "pName")); j < 256; j++){
                    if(tempPath[j] == '.') break;
                }
                strncpy(secondVersion, tempPath+strlen(getVariableData(variableList, "pName"))+1, j - strlen(getVariableData(variableList, "pName"))-1);
                int secondVersionNum = atoi(secondVersion);
                if (secondVersionNum < versionNum) pass2 = -1;
                if(pass1 && (secondVersionNum == versionNum)) pass3 = 1;

                //if its of the same project AND the version number is higher delete it
                if(pass1 == 1 && pass2 == 1){//bye bitch
                    char destructionPath[256] = {0};
                    strcat(destructionPath, "rm -rf backups/");
                    strcat(destructionPath, dirPointer->d_name);
                    system(destructionPath);
                }
            }
            closedir(currentDir);

            if(!pass3){
                printf("Couldn't find specified version");
            }
            else printf("Rollback completed.\n");
            pthread_mutex_unlock(&lock);
        }
    }
    
    freeAllMallocs(variableList);
    printf("Exited new client thread.\n");
    close(new_socket);
    pthread_exit(NULL);
}


int main(int argc, char** argv){
    // change directory
    chdir("server");
    atexit(cleanUp);
    signal(2, catchSigint);
    variableList = NULL;
    threadCounter = 0;

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