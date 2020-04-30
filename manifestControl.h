void md5hash(char* input, char* buffer);
void createManifest(char* projectName);
void fillManifest(char* ogPath, char* writeTo);
void sortManifest(char* projectName);
char* convertHash(char* filePath);
char* convertHashGivenHash(char* hash);