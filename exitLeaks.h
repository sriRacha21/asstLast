#define MAX_THREADS 60
struct exitNode{
    char* variableName;
    char* variableData;
    struct exitNode* next;
};

struct exitNode* variableList;
//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threadID[60];
int threadCounter;
//int threadCounter = 0;

void catchSigint(int signal);
char* returnMallocCopyOfName(struct exitNode* head, char* token);
int whatDoWithToken(struct exitNode* head, char* token);
struct exitNode* createNode(char* vName, char* vData, int freeMode);
struct exitNode* insertExit(struct exitNode* head, struct exitNode* toInsert);
struct exitNode* freeVariable(struct exitNode* head, char* vName);
char* getVariableData(struct exitNode* head, char* vName);
void printList(struct exitNode* head);
void freeAllMallocs(struct exitNode* head);
void cleanUp();