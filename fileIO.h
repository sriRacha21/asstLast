void createParentDirectories(char* path);
char* readFile(char *filename);
void writeFile( char *path, char *content );
void writeFileAppend( int fd, char *content );
char* readManifestFromSocket(int sock);
int rwFileFromSocket(int sock);
int lineCount(char* path);