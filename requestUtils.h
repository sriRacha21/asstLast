void sendProjectFiles(char* projectName, int socket);
char* concatFileSpecs(char* fileName, char* projectName); //0 = <size>;<content> 1 = <size>;<path>;<content>
char* concatFileSpecsWithPath(char* fileName, char* projectName);
int lengthOfInt(int num);
int getFileSize(char* fileName);
char* getProjectName(char* msg, int prefixLength);
char* specificFileStringManip(char* msg, int prefixLength, int pathOrProject);
void projectExists(char* projectName, int socket);