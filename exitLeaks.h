struct exitNode{
    char* variableName;
    char* variableData;
    struct exitNode* next;
};

struct exitNode* createNode(char* vName, char* vData);
struct exitNode* insertExit(struct exitNode* head, struct exitNode* toInsert);
struct exitNode* freeVariable(struct exitNode* head, char* vName);
void printList(struct exitNode* head);
void freeAllMallocs(struct exitNode* head);
void cleanUp();