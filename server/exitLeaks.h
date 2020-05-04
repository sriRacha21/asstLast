struct exitNode{
    char* variableName;
    char* variableData;
    struct exitNode* next;
};

struct exitNode* variableList;

char* returnMallocCopyOfName(struct exitNode* head, char* token);
int whatDoWithToken(struct exitNode* head, char* token);
struct exitNode* createNode(char* vName, char* vData, int freeMode);
struct exitNode* insertExit(struct exitNode* head, struct exitNode* toInsert);
struct exitNode* freeVariable(struct exitNode* head, char* vName);
char* getVariableData(struct exitNode* head, char* vName);
void printList(struct exitNode* head);
void freeAllMallocs(struct exitNode* head);
void cleanUp();