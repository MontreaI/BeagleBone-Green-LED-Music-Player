#include <stdio.h>
#include <string.h>

/******************************************************************************
 **              INTERNAL MACRO DEFINITIONS
 ******************************************************************************/
#define MAX_STACK_SIZE 1024

/******************************************************************************
 **              INTERNAL STRUCT
 ******************************************************************************/
typedef struct shuffle {
    int stack[MAX_STACK_SIZE];
    int top;
} shuffle_t;

shuffle_t shuffleHistory;

/******************************************************************************
 **              INTERNAL FUNCTION PROTOTYPES
 ******************************************************************************/
void Stack_push(int val){
	if (shuffleHistory.top == (MAX_STACK_SIZE - 1)){
		printf("STACK FULL\n");
	}
	else {

		shuffleHistory.top++;
		shuffleHistory.stack[shuffleHistory.top] = val;
	}
}

int Stack_pop(){
	int popped = -1;

	if (shuffleHistory.top == -1){
		printf("STACK EMPTY\n");

	}
	else{
		popped = shuffleHistory.stack[shuffleHistory.top];
		shuffleHistory.top--;
	}

	return popped;
}

void Stack_clear(){
	shuffleHistory.top = -1;
}