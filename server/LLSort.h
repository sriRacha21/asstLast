struct manifestNode* insertManifest(struct manifestNode* head, struct manifestNode* toInsert);
void printMList(struct manifestNode* head);
void freeMList(struct manifestNode* head);
struct manifestNode* createMNode(char* data);
int insertionSort(void* toSort, int (*comparator)(void*, void*));
int stringCmp(void* first, void* second);

struct manifestNode{
    struct manifestNode* next;
    struct manifestNode* prev;
    char* data;
    int index;
};

int globalIndex;