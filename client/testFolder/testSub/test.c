#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>

typedef struct node{
  char name[21];
  char type[15];
  int* bits;
  int column;
  int selector;
  struct node* next;
} node;

int** allocate_matrix(int rows, int cols);
void free_matrix(int** arr1, int rows);
void printArray(int** arr1, int rows, int columns);
void storeValue(char* name, char* type, int colNum, int numBits); 
node* store2(node* location, char* name, int numBits, char* type);
void grayFill(int numInputs);
void revRow(int *row, int columns);
void revArray(int **array, int columns, int rows);
void decToB(int x, int n, int row);
void grayFill(int inputs);
void free_list();
void swap(int *a, int *b);
void printList();
int not(int arg1);
int and(int arg1, int arg2);
int or(int arg1, int arg2);
int nand(int arg1, int arg2);
int nor(int arg1, int arg2);
int xor(int arg1, int arg2);
int xnor(int arg1, int arg2);
int binToDec(int n);
node* search(char* name);
node* make(char* name, char* type);

int** truthTable;
node* head;

int main(int argc, char** argv){
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL){
        printf("error"); //file doesn't exist program go boom
        return 0;
    }
    char operation[25];
    char varName[25];
    char numTemp[5];
    int numInputs, numOutputs, rows, columns;
    //store inputs
    while(!feof(fp)){
      fscanf(fp, "%s", operation);
      if(!strcmp(operation, "INPUTVAR")){
        fscanf(fp, "%s", numTemp);
        numInputs = atoi(numTemp);
        rows = pow(2, numInputs);
        int counter = 0;
        for(counter = 0; counter < numInputs; counter++){
          fscanf(fp, "%s", varName);
          storeValue(varName, "INPUT", counter, rows);
        }
        break;
      }
    }
    fclose(fp);
    //store outputs
    fp = fopen(argv[1], "r");
    while(!feof(fp)){
      fscanf(fp, "%s", operation);
      if(!strcmp(operation, "OUTPUTVAR")){
        fscanf(fp, "%s", numTemp);
        numOutputs = atoi(numTemp);
        columns = numInputs + numOutputs;
        int counter = 0;
        for(counter = 0; counter < numOutputs; counter++){
          fscanf(fp, "%s", varName);
          storeValue(varName, "OUTPUT", counter + numInputs, rows);
        }
        break;
      }
    }
    fclose(fp);

    truthTable = allocate_matrix(rows, columns);
    grayFill(numInputs);
    //putting the input graycode into the variables in the LL
    int i, j;
    node* temp = head;
    for(i = 0; i < numInputs; i++){
       for(j = 0; j < rows; j++){
         temp->bits[j] = truthTable[j][i];
      }
      temp = temp->next;
    }
    
    char varName1[25];
    char varName2[25];
    char outputVar[25];
    node* temp1;
    node* temp2;
    int success = 0;
    while(success == 0){
      fp = fopen(argv[1], "r");
      while(!feof(fp)){
        int input1 = -1; int input2 = -1;
        fscanf(fp, "%s", operation);
        if(!strcmp("INPUTVAR", operation) || !strcmp("OUTPUTVAR", operation)){
          fscanf(fp, "%*[^\n]\n");
         }
         else if(!strcmp("AND", operation)){
           fscanf(fp, "%s %s %s", varName1, varName2, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else temp1 = search(varName1);

          if(temp1 == NULL && input1 == -1) continue;

           if(varName2[0] == '0') input2 = 0;
           else if(varName2[0] == '1') input2 = 1;
           else temp2 = search(varName2);

           if(temp2 == NULL && input2 == -1) continue;
           
            storeValue(outputVar, "TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1 && input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = and(input1, input2); 
            else if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = and(input1, temp2->bits[i]);    
            else if(input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = and(temp1->bits[i], input2);   
            else for(i = 0; i < rows; i++) tempOut->bits[i] = and(temp1->bits[i], temp2->bits[i]);      
         }
         else if(!strcmp("NOT", operation)){
           fscanf(fp, "%s %s", varName1, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else{
            temp1 = search(varName1);
           }

           if(temp1 == NULL && input1 == -1) continue;
            storeValue(outputVar,"TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = not(input1);
            else for(i = 0; i < rows; i++) tempOut->bits[i] = not(temp1->bits[i]);
         }
         else if(!strcmp("OR", operation)){
           fscanf(fp, "%s %s %s", varName1, varName2, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else temp1 = search(varName1);
           if(temp1 == NULL && input1 == -1) continue;

           if(varName2[0] == '0') input2 = 0;
           else if(varName2[0] == '1') input2 = 1;
           else temp2 = search(varName2);
           if(temp2 == NULL && input2 == -1) continue;
           
            storeValue(outputVar, "TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1 && input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = or(input1, input2); 
            else if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = or(input1, temp2->bits[i]);    
            else if(input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = or(temp1->bits[i], input2);   
            else for(i = 0; i < rows; i++) tempOut->bits[i] = or(temp1->bits[i], temp2->bits[i]);      
         }
         else if(!strcmp("XOR", operation)){
           fscanf(fp, "%s %s %s", varName1, varName2, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else temp1 = search(varName1);
           if(temp1 == NULL && input1 == -1) continue;

           if(varName2[0] == '0') input2 = 0;
           else if(varName2[0] == '1') input2 = 1;
           else temp2 = search(varName2);
           if(temp2 == NULL && input2 == -1) continue;
           
            storeValue(outputVar, "TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1 && input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = xor(input1, input2); 
            else if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = xor(input1, temp2->bits[i]);    
            else if(input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = xor(temp1->bits[i], input2);   
            else for(i = 0; i < rows; i++) tempOut->bits[i] = xor(temp1->bits[i], temp2->bits[i]);  
         }
         else if(!strcmp("NOR", operation)){
           fscanf(fp, "%s %s %s", varName1, varName2, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else temp1 = search(varName1);
           
           if(temp1 == NULL && input1 == -1) continue;

           if(varName2[0] == '0') input2 = 0;
           else if(varName2[0] == '1') input2 = 1;
           else temp2 = search(varName2);
           if(temp2 == NULL && input1 == -1) continue;
           
            storeValue(outputVar, "TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1 && input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = nor(input1, input2); 
            else if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = nor(input1, temp2->bits[i]);    
            else if(input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = nor(temp1->bits[i], input2);   
            else for(i = 0; i < rows; i++) tempOut->bits[i] = nor(temp1->bits[i], temp2->bits[i]);  
         }
         else if(!strcmp("XNOR", operation)){
           fscanf(fp, "%s %s %s", varName1, varName2, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else temp1 = search(varName1);
           if(temp1 == NULL && input1 == -1) continue;

           if(varName2[0] == '0') input2 = 0;
           else if(varName2[0] == '1') input2 = 1;
           else temp2 = search(varName2);
           if(temp2 == NULL && input2 == -1) continue;
           
            storeValue(outputVar, "TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1 && input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = xnor(input1, input2); 
            else if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = xnor(input1, temp2->bits[i]);    
            else if(input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = xnor(temp1->bits[i], input2);   
            else for(i = 0; i < rows; i++) tempOut->bits[i] = xnor(temp1->bits[i], temp2->bits[i]);   
         }
         else if(!strcmp("NAND", operation)){
           fscanf(fp, "%s %s %s", varName1, varName2, outputVar);
           if(varName1[0] == '0') input1 = 0;
           else if(varName1[0] == '1') input1 = 1;
           else temp1 = search(varName1);

           if(temp1 == NULL && input2 == -1) continue;

           if(varName2[0] == '0') input2 = 0;
           else if(varName2[0] == '1') input2 = 1;
           else temp2 = search(varName2);

           if(temp2 == NULL && input2 == -1) continue;
           
            storeValue(outputVar, "TEMP", -1, rows);
            node* tempOut = search(outputVar);
            if(input1 != -1 && input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = nand(input1, input2); 
            else if(input1 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = nand(input1, temp2->bits[i]);    
            else if(input2 != -1) for(i = 0; i < rows; i++) tempOut->bits[i] = nand(temp1->bits[i], input2);   
            else for(i = 0; i < rows; i++) tempOut->bits[i] = nand(temp1->bits[i], temp2->bits[i]);     
         }
         else if(!strcmp("MULTIPLEXER", operation)){
             int muxInputs, muxSelectors;
             fscanf(fp, "%d", &muxInputs);
             muxSelectors = log2(muxInputs);
             node** mInputs = malloc(muxInputs * sizeof(node));
             node** mSelectors = malloc(muxSelectors * sizeof(node));
             for(i = 0; i < muxInputs; i++) mInputs[i] = NULL;
             for(i = 0; i < muxInputs; i++){
                 fscanf(fp, "%s", varName);
                 node* toInsert = malloc(sizeof(node));
                 strcpy(toInsert->name, varName);
                 toInsert->bits = malloc(rows * sizeof(int));
                 toInsert->selector = i^(i>>1);
                 if(varName[0] == '0') for(j = 0; j < rows; j++) toInsert->bits[j] = 0;
                 else if(varName[0] == '1') for(j = 0; j < rows; j++) toInsert->bits[j] = 1;
                 else{
                     temp1 = search(varName);
                     for(j = 0; j < rows; j++) toInsert->bits[j] = temp1->bits[j];
                 }
                 mInputs[i] = toInsert;
             }
            for(i = 0; i < muxSelectors; i++){
                fscanf(fp, "%s", varName);
                node* toInsert = malloc(sizeof(node));
                strcpy(toInsert->name, varName);
                 toInsert->bits = malloc(rows * sizeof(int));
                 if(varName[0] == '0') for(j = 0; j < rows; j++) toInsert->bits[j] = 0;
                 else if(varName[0] == '1') for(j = 0; j < rows; j++) toInsert->bits[j] = 1;
                 else{
                     temp1 = search(varName);
                     for(j = 0; j < rows; j++) toInsert->bits[j] = temp1->bits[j];
                 }
                 mSelectors[i] = toInsert;
            }
            fscanf(fp, "%s", varName);
            node* muxOutput = malloc(sizeof(node));
            strcpy(muxOutput->name, varName);
            muxOutput->bits = malloc(rows * sizeof(int));
            storeValue(muxOutput->name, "TEMP", -1, rows);
            for(i = 0; i < rows; i++){
                int binary = 0;
                for(j = 0; j < muxSelectors; j++){
                    binary += mSelectors[j]->bits[i] * pow(10, muxSelectors - j - 1);
                }
                binary = binToDec(binary);
                for(j = 0; j < muxInputs; j++){
                    if(mInputs[j]->selector == binary) muxOutput->bits[i] = mInputs[j]->bits[i];
                }
            }
            temp1 = search(muxOutput->name);
            for(i = 0; i < rows; i++) temp1->bits[i] = muxOutput->bits[i];
            for(i = 0; i < muxInputs; i++){
              free(mInputs[i]->bits);
              free(mInputs[i]);
            }
            free(mInputs);
            for(i = 0; i < muxSelectors; i++){
              free(mSelectors[i]->bits);
              free(mSelectors[i]);
            }
            free(mSelectors);
            free(muxOutput->bits);
            free(muxOutput);
         }
         else if(!strcmp("DECODER", operation)){
            node* decInputHead = NULL;
            node* decOutputHead = NULL;
            int decInputs, decOutputs;
            fscanf(fp, "%s", numTemp);
             decInputs = atoi(numTemp);
             decOutputs = pow(2, decInputs);
             for(i = 0; i < decInputs; i++){
                 fscanf(fp, "%s", varName);
                 if(varName[0] == '0') decInputHead = store2(decInputHead, varName, rows, "ZERO");
                 else if(varName[0] == '1') decInputHead = store2(decInputHead, varName, rows, "ONE");
                 else decInputHead = store2(decInputHead, varName, rows, "NORMAL");
             }
            for(i = 0; i < decOutputs; i++){
                fscanf(fp, "%s", varName);
                if(varName[0] == '0') decOutputHead = store2(decOutputHead, varName, rows, "ZERO");
                else if(varName[0] == '1') decOutputHead = store2(decOutputHead, varName, rows, "ONE");
                else decOutputHead = store2(decOutputHead, varName, rows, "NORMAL");
            }
            node* temp3 = decOutputHead;
            for(i = 0; i < decOutputs; i++){
                temp3->selector = i^(i>>1);
                temp3 = temp3->next;
            }
            temp3 = decInputHead;
            while(temp3 != NULL){
                if(!strcmp(temp3->type, "ZERO")) for(j = 0; j < rows; j++) temp3->bits[j] = 0;
                else if(!strcmp(temp3->type, "ONE")) for(j = 0; j < rows; j++) temp3->bits[j] = 1;
                else{
                    temp1 = search(temp3->name);
                    for(j = 0; j < rows; j++) temp3->bits[j] = temp1->bits[j];
                }
                temp3 = temp3->next;
            }
            for(i = 0; i < rows; i++){
                int binary = 0;
                temp3 = decInputHead;
                for(j = 0; j < decInputs; j++){
                    binary += temp3->bits[i] * pow(10, decInputs - j - 1);
                    temp3 = temp3->next;
                }
                int whichOne = binToDec(binary);
                temp3 = decOutputHead;
                while(temp3 != NULL){
                    if(temp3->selector == whichOne) temp3->bits[i] = 1;
                    else temp3->bits[i] = 0;
                    temp3 = temp3->next;
                }
            }
            temp3 = decOutputHead;
            for(i = 0; i < decOutputs; i++){
                storeValue(temp3->name, "TEMP", -1, rows);
                temp1 = search(temp3->name);
                for(j = 0; j < rows; j++){
                    temp1->bits[j] = temp3->bits[j];
                }
                temp3 = temp3->next;
            }
            free_list(decInputHead);
            free_list(decOutputHead);
         }
      }
      fclose(fp);
      node* temp = head;
      while(temp != NULL){
        if(!strcmp(temp->type, "OUTPUT")){
          if(temp->bits[0] == -1) success = 0;
          else success = 342;
        }
        temp = temp->next;
      }
    }
    
    //filling in the output columns
    temp = head;
    while(temp != NULL){
        if(!strcmp(temp->type, "OUTPUT")){
            for(j = 0; j < rows; j++) 
                truthTable[j][temp->column] = temp->bits[j];
        }
        temp = temp->next;
    }
    
    printArray(truthTable, rows, columns);
    free_list(head);
    free_matrix(truthTable, rows);
    return 0;
}

node* search(char* name){
    node* toReturn = head;
    while(toReturn != NULL){
        if(!strcmp(toReturn->name, name)) return toReturn;
        toReturn = toReturn->next;
    }
    return NULL;
}

node* store2(node* location, char* name, int numBits, char* type){
    node* toInsert = malloc(sizeof(node));
    strcpy(toInsert->name, name);
    strcpy(toInsert->type, type);
    toInsert->bits = (int*)malloc(numBits * sizeof(int));
    int i;
    for(i = 0; i < numBits; i++) toInsert->bits[i] = -1;
    toInsert->column = -1;
    toInsert->next = NULL;
    toInsert->selector = -1;
    if(location == NULL){
        location = toInsert;
        return location;
    }
    else{
        node* temp = location;
        while(temp->next != NULL) {
            if((!strcmp(temp->name, name) && temp->name[0] != '0' && temp->name[0] != '1')){
                free(toInsert->bits);
                free(toInsert);
                return location;
            }
            temp = temp->next;
        }
        temp->next = toInsert;
        return location;
    }
}

void storeValue(char* name, char* type, int colNum, int numBits){
  node* toInsert = malloc(sizeof(node));
  strcpy(toInsert->name, name);
  strcpy(toInsert->type, type);
  toInsert->bits = malloc(numBits * sizeof(int));
  int i;
  for(i = 0; i < numBits; i++) toInsert->bits[i] = -1;
  toInsert->column = colNum;
  toInsert->next = NULL;
  toInsert->selector = -1;
  if(head == NULL){
    head = toInsert;
  }
  else{
    node* temp = head;
    while(temp->next != NULL) {
      if(!strcmp(temp->name, name) || !strcmp(head->name, name)){
          free(toInsert->bits);
          free(toInsert);
          return;
      }
      temp = temp->next;
    }
    temp->next = toInsert;
  }
}

void printList(){
  node* temp;
  printf("LIST: \n");
  for(temp = head; temp != NULL; temp = temp->next) printf("%s %d\n", temp->name, temp-> column);
  printf("\n");
}

int not(int arg1){
  return !(arg1);
}
int and(int arg1, int arg2){
  return (arg1 & arg2);
}
int or(int arg1, int arg2){
  return (arg1 | arg2);
}
int nand(int arg1, int arg2){
  return !(arg1 & arg2);
}
int nor(int arg1, int arg2){
  return !(arg1 | arg2);
}
int xor(int arg1, int arg2){
  return (arg1^arg2);
}
int xnor(int arg1, int arg2){
  return !(arg1^arg2);
}
int binToDec(int n){
    int i = 0;
    int j = 0;
    int remainder;
    while(n != 0){
        remainder = n % 10;
        n /= 10;
        i += remainder * pow(2, j);
        j++;
    }
    return i;
}
void swap(int *a, int *b){
    int temp = *a;
    *a = *b;
    *b = temp; 
}
void revRow(int *row, int columns){
    int index;
    for(index = 0; index < columns/2; index++) swap(row + index, row+columns-1-index);
}

void revArray(int **array, int columns, int rows){
    int row;
    for (row = 0; row < rows; row++) revRow(array[row], columns);
}
void decToB(int x, int n,int row){
    int* binary = malloc(x * sizeof(int));
    int i = 0;
    while(x > 0){
        binary[i] = x % 2;
        x = x/2;
        i++;
    }
    int j;
    for(j = 0; j < n - i; j++) truthTable[row][j] = 0;
    for(j = i - 1; j >=0; j--) truthTable[row][j] = binary[j];
    free(binary);
}

void grayFill(int inputs){
    int rows = 1 << inputs;
    int i;
    for(i = 0; i < rows; i++){
        int x = i ^ (i >> 1);
        decToB(x, inputs, i);
    }
    revArray(truthTable, inputs, rows);
}

int** allocate_matrix(int rows, int cols){
  int** ret_val = (int**)malloc(rows * sizeof(int*));
  int i, j;
  for(i = 0; i < rows; i++){
    ret_val[i] = (int*)malloc(cols * sizeof(int));
  }
  for(i = 0; i < rows; i++){
    for( j = 0; j < cols; j++){
      ret_val[i][j] = 0;
    }
  }
  return ret_val;
}
void free_matrix(int** arr1, int rows){
  int i;
  for(i = 0; i < rows; i++){
    free(arr1[i]);
  }
  free(arr1);
}
void printArray(int** arr1, int rows, int columns){
  int i, j;
  for(i = 0; i < rows; i++){
    for(j = 0; j < columns; j++){
      int toPrint;
      toPrint = arr1[i][j];
      printf("%d ", toPrint);
    }
    printf("\n");
  }
}
void free_list(node* location){
  node* temp = location;
  while(temp != NULL){
    node* temp2 = temp;
    temp = temp->next;
    free(temp2->bits);
    free(temp2);
  }
}