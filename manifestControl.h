void md5hash(char* input, char* buffer);
void createManifest(char* projectName, int versionNum);
void createHistory(char* projectName);
void fillManifest(char* ogPath, char* writeTo, int version);
void sortManifest(char* projectName);
char* convertHash(char* filePath);
char* convertHashGivenHash(char* hash);