typedef struct manifestEntry {
    int version;
    char* path;
    char* hash;
} manifestEntry;
void doesProjectExist( int sock, char* projectName );