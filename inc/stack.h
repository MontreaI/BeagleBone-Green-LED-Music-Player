// Methods relating to shuffle song history

#ifndef STACK_H
#define STACK_H

void Stack_push(int val);			// val is the currentSong pushed

int Stack_pop();					// return the last shuffled song played
									// returns -1 if the last shuffled song is empty

void Stack_clear();					// Empties the stack

#endif