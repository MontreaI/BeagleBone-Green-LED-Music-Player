// Methods relating to potentiometer controls

// Turning Potentiometer adjusts volume

#ifndef POT_H
#define POT_H

void POT_init();
void POT_cleanup();

int POT_getVolume(); // Returns a volume in [10,100] based on potentiometer voltage

#endif
