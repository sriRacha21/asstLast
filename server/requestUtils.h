int deleteFilesFromPush(char* commitContents);
int rewriteFileFromSocket(int socket);
char* rwCommitToHistory(int socket, char* projectName);
void createProjectFolder(char* projectName);
char* checkVersion(char* projectName, int socket);
char* getSpecificFileSpecs(char* projectName, char* filePath);
void sendProjectFiles(char* projectName, int socket);
char* concatFileSpecs(char* fileName, char* projectName); //0 = <size>;<content> 1 = <size>;<path>;<content>
char* concatFileSpecsWithPath(char* fileName, char* projectName);
int lengthOfInt(int num);
int getFileSize(char* fileName);
char* getProjectName(char* msg, int prefixLength);
char* specificFileStringManip(char* msg, int prefixLength, int pathOrProject);
int projectExists(char* projectName, int socket);