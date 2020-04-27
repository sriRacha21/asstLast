struct exitNode{
    char* variableName;
    char* variableData;
    struct exitNode* next;
};

struct exitNode* variableList = NULL;

struct exitNode* createNode(char* vName, char* vData);
struct exitNode* insertExit(struct exitNode* head, struct exitNode* toInsert);
void freeAllMallocs(struct exitNode* head);
struct exitNode* freeVariable(struct exitNode* head, char* vName);
void printList(struct exitNode* head);