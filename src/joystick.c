#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "joystick.h"
#include "audio.h"

#define JOYSTICK_EXPORT "/sys/class/gpio/export"
#define UP 26
#define RIGHT 47
#define DOWN 46
#define LEFT 65
#define IN 27
#define BUFFSIZE 1024
#define MAX_VOLUME 100
#define MIN_VOLUME 0

static pthread_t joystickThreadId;
static _Bool stop = false;

// Private functions forward declarations
static void* Joystick_thread(void* arg);
static void Joystick_init();
static void writeFile(char *filePath, int value);
static _Bool Joystick_isPressed(int direction);

void Joystick_startPolling()
{
    if (pthread_create(&joystickThreadId, NULL, Joystick_thread, NULL))
        printf("ERROR cannot create a new Joystick thread");
}

void Joystick_stopPolling()
{
	stop = true;
    pthread_join(joystickThreadId, NULL);
}

static void* Joystick_thread(void* arg)
{
    Joystick_init();

    while (!stop)
    {

        if (Joystick_isPressed(UP)){
            int currentVolume = Audio_getVolume() + 5;
            currentVolume = (currentVolume > MAX_VOLUME) ? MAX_VOLUME : currentVolume;
            currentVolume = (currentVolume < MIN_VOLUME) ? MIN_VOLUME : currentVolume;
            Audio_setVolume(currentVolume);
        }
            
        else if (Joystick_isPressed(DOWN))
        {
        	int currentVolume = Audio_getVolume() - 5;
            currentVolume = (currentVolume > MAX_VOLUME) ? MAX_VOLUME : currentVolume;
            currentVolume = (currentVolume < MIN_VOLUME) ? MIN_VOLUME : currentVolume;
            Audio_setVolume(currentVolume);
        }

        else if (Joystick_isPressed(RIGHT))
        {
            // next track
        }
            
        else if (Joystick_isPressed(LEFT))
        {
            // previous track
        }

        else if (Joystick_isPressed(IN))
        {
        	Audio_togglePlayback();
        }
        nanosleep((const struct timespec[]){{0, 100000000}}, NULL);
    }
    return NULL;
}

static void Joystick_init()
{
    writeFile(JOYSTICK_EXPORT, UP);
    writeFile(JOYSTICK_EXPORT, RIGHT);
    writeFile(JOYSTICK_EXPORT, DOWN);
    writeFile(JOYSTICK_EXPORT, LEFT);
    writeFile(JOYSTICK_EXPORT, IN);
}

static void writeFile(char *filePath, int value)
{
    FILE *file = fopen(filePath, "w");
    if (file == NULL)
    {
        printf("Error opening file (%s) for write\n", filePath);
        exit(1);
    }

    int dataWritten = fprintf(file, "%d", value);
    if (dataWritten <= 0)
    {
        printf("Error writing data\n");
        exit(1);
    }
    
    fclose(file);
}

static _Bool Joystick_isPressed(int direction)
{
    char *filePath;
    if (asprintf(&filePath, "/sys/class/gpio/gpio%d/value", direction) == -1)
        printf("ERROR Joystick_isPressed: cannot allocate a temporary string");

	FILE *file = fopen(filePath, "r");
	if (file == NULL)
    {
		printf("Error opening file (%s) for read\n", filePath);
		exit(-1);
	}
	
	// read joystick value
    int pressedValue = 1;
	fscanf(file, "%d", &pressedValue);
    pressedValue = (pressedValue == 1) ? 0 : 1;

	fclose(file);

    free(filePath);
    filePath = NULL;
	return pressedValue;
}
