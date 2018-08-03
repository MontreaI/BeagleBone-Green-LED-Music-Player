// Methods relating to potentiometer controls

// Turning Potentiometer adjusts volume

#ifndef POT_H
#define POT_H

void POT_init();
void POT_cleanup();

int POT_getVolume(); 	// Return Volume [0, 100]

#endif