struct exitNode{
    char** strPtr;
    int freed; //1 if strPtr is freed already 0 if not
    struct exitNode* next;
};

void insertMalloc(struct exitNode** head, char* string);
struct exitNode* createExitN(char* string);
void freeMallocs(struct exitNode* head); //only used atexit.  FREES EVERYTHING
void freeVariable(char* string, struct exitNode* head);
void printList(struct exitNode* head);