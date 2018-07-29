// Methods relating to joystick controls

// Pressing IN will play/pause the music

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
