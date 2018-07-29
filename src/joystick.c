#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

#include "joystick.h"
#include "audio.h"
#include "led_matrix.h"
#include "song_data.h"

#define JOYSTICK_EXPORT "/sys/class/gpio/export"
#define POLL_SPEED_NS 100000000     // 100ms
#define UP 26
#define RIGHT 47
#define DOWN 46
#define LEFT 65
#define IN 27

static pthread_t joystickThreadId;
static pthread_mutex_t joystickMutex = PTHREAD_MUTEX_INITIALIZER;
static _Bool stopping = false;
_Bool isMenu = true;                                            // Menu mode
static _Bool isInfo = false;                                            // Info Mode                                        

// Private functions forward declarations
static void* Joystick_thread(void* arg);
static void Joystick_export(char *filePath, int value);
static _Bool Joystick_isPressed(int direction);

void Joystick_init(){
    printf("Joystick_init()\n");
    Joystick_export(JOYSTICK_EXPORT, UP);
    Joystick_export(JOYSTICK_EXPORT, RIGHT);
    Joystick_export(JOYSTICK_EXPORT, DOWN);
    Joystick_export(JOYSTICK_EXPORT, LEFT);
    Joystick_export(JOYSTICK_EXPORT, IN);

    if (pthread_create(&joystickThreadId, NULL, Joystick_thread, NULL))
        printf("ERROR cannot create a new Joystick thread");
}

void Joystick_cleanup(){
    stopping = true;
    pthread_join(joystickThreadId, NULL);
}

static void Joystick_enterInfo(){
    isInfo = true;
    ledMatrix_display_info(0);
}

static void Joystick_exitInfo(){
    isInfo = false;
    ledMatrix_clear_infoIndex();
    ledMatrix_clear();
    ledMatrix_display_back();
}

static void Joystick_enterMenu(){
    isMenu = true;
    ledMatrix_display_song_list();
}

static void Joystick_exitMenu(){
    isMenu = false;   
    // ledMatrix_clear_menu();
    ledMatrix_clear();
    Song_data_exitMenuDisplay();
}

static void* Joystick_thread(void* arg){
    int count = 0, debounce = 5;        // debounce time == 500 ms (5 * 100 (poll speed))
    _Bool triggered = false;

    while (!stopping) {
        if (!triggered) {
            triggered = true;
            // UP
            if (Joystick_isPressed(UP)) {
                pthread_mutex_lock(&joystickMutex);
                {
                    // printf("UP\n");

                    if(isMenu){
                        if(isInfo){
                            printf("Prev Info\n");
                            ledMatrix_display_prev_info();
                        }
                        else{
                            printf("Prev Song\n");
                            ledMatrix_display_prev_song();
                        }
                    }
                    else{
                        // Song_data_toggleRepeat();
                        printf("Display Song List: %d\n", ledMatrix_getCurrentSong());  // Always prints zero
                        Joystick_enterMenu();
                    }

                    nanosleep((const struct timespec[]){{0, POLL_SPEED_NS}}, NULL);
                }
                pthread_mutex_unlock(&joystickMutex);
            }
            // DOWN
            else if (Joystick_isPressed(DOWN)) {
                pthread_mutex_lock(&joystickMutex);
                {
                    // printf("DOWN\n");

                    if(isMenu){
                        if(isInfo){
                            printf("Next Info\n");
                            ledMatrix_display_next_info();
                        }
                        else{
                            printf("Next Song\n");
                            ledMatrix_display_next_song();
                        }
                    }
                    else{
                        printf("Shuffle\n");
                        Song_data_toggleShuffle();
                    }
                    
                    nanosleep((const struct timespec[]){{0, POLL_SPEED_NS}}, NULL);
                }
                pthread_mutex_unlock(&joystickMutex);
            }
            // RIGHT
            else if (Joystick_isPressed(RIGHT)) {
                pthread_mutex_lock(&joystickMutex);
                {
                    // printf("RIGHT\n");

                    if(isMenu){
                        if(isInfo){

                        }
                        else{
                            printf("Enter Info\n");
                            Joystick_enterInfo();
                        }
                    }
                    else{
                        printf("Play Next\n");
                        Audio_setJoystickInput(JOYSTICK_RIGHT);
                    }
                    
                    nanosleep((const struct timespec[]){{0, POLL_SPEED_NS}}, NULL);
                }
                pthread_mutex_unlock(&joystickMutex);
            }
            // LEFT 
            else if (Joystick_isPressed(LEFT)) {
                pthread_mutex_lock(&joystickMutex);
                {
                    // printf("LEFT\n");

                    if(isMenu){
                        if(isInfo){
                            printf("Exit Info\n");
                            Joystick_exitInfo();
                        }
                        else{
                            printf("Exit Menu\n");
                            Joystick_exitMenu();
                        }
                    }
                    else{
                        printf("Prev Song\n");
                        Audio_setJoystickInput(JOYSTICK_LEFT);
                    }

                    nanosleep((const struct timespec[]){{0, POLL_SPEED_NS}}, NULL);
                }
                pthread_mutex_unlock(&joystickMutex);
            }
            // IN
            else if (Joystick_isPressed(IN)) {
                pthread_mutex_lock(&joystickMutex);
                {
                    // printf("IN\n");

                    if(isMenu){
                        if(isInfo){

                        }
                        else{
                            printf("Playing %s\n", Song_data_getSongName(ledMatrix_getCurrentSong()));
                            Audio_setJoystickInput(JOYSTICK_IN);
                            Joystick_exitMenu();
                        }
                    }
                    else{
                        printf("Play / Pause\n");
                       	if (Audio_getPause()) {
                       		Audio_setPause(false);
                            Song_data_unpauseTimer();
                       	} else {
                       		Audio_setPause(true);
                            Song_data_pauseTimer();     
                       	}
                    }
                }
                pthread_mutex_unlock(&joystickMutex);
            }

            else
                triggered = false;

            count = 0;
        }

        else if (++count == debounce)
            triggered = false;

        nanosleep((const struct timespec[]){{0, POLL_SPEED_NS}}, NULL);
    }
    return NULL;
}

static void Joystick_export(char *filePath, int value){
    FILE *file = fopen(filePath, "w");
    if (file == NULL) {
        printf("Error opening file (%s) for write\n", filePath);
        exit(1);
    }

    int dataWritten = fprintf(file, "%d", value);
    if (dataWritten <= 0) {
        printf("Error writing data\n");
        exit(1);
    }
    
    fclose(file);
}

static _Bool Joystick_isPressed(int direction){
    char *filePath;
    if (asprintf(&filePath, "/sys/class/gpio/gpio%d/value", direction) == -1)
        printf("ERROR Joystick_isPressed: cannot allocate a temporary string");

	FILE *file = fopen(filePath, "r");
	if (file == NULL) {
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

