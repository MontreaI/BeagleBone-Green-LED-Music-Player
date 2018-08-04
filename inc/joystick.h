// Methods relating to joystick controls

//  Song Menu Library Inputs:
// 	UP		= Prev
// 	DOWN	= Next
// 	LEFT 	= Back
// 	RIGHT 	= Next
// 	IN 		= Select

//  Current Track Display Inputs:
// 	UP		= Toggle Repeat
// 	DOWN	= Toggle Shuffle
// 	LEFT 	= Play Prev / Replay
// 	RIGHT 	= Play Next
// 	IN 		= Play / Plause


#ifndef JOYSTICK_H
#define JOYSTICK_H

typedef enum {
	JOYSTICK_LEFT,
	JOYSTICK_RIGHT,
	JOYSTICK_IN
} Joystick_Input;

void Joystick_init();
void Joystick_cleanup();

#endif
