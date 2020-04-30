struct exitNode{
    char* variableName;
    char* variableData;
    struct exitNode* next;
};

struct exitNode* variableList;

struct exitNode* createNode(char* vName, char* vData, int freeMode);
struct exitNode* insertExit(struct exitNode* head, struct exitNode* toInsert);
struct exitNode* freeVariable(struct exitNode* head, char* vName);
char* getVariableData(struct exitNode* head, char* vName);
void printList(struct exitNode* head);
void freeAllMallocs(struct exitNode* head);
void cleanUp();