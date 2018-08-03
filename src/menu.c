#include <stdbool.h>
#include <pthread.h>

#include "led_matrix.h"
#include "song_data.h"

/******************************************************************************
 **              INTERNAL VARIABLES
 ******************************************************************************/
static _Bool isMenu = true;         // Menu mode
static _Bool isInfo = false;        // Info Mode  

/******************************************************************************
 **              FUNCTION DEFINITIONS
 ******************************************************************************/
_Bool Menu_isMenu(){
    return isMenu;
}

_Bool Menu_isInfo(){
    return isInfo;
}

void Menu_enterInfo(){
    isInfo = true;
    ledMatrix_display_info(0);
}

void Menu_exitInfo(){
    isInfo = false;
    ledMatrix_clear_infoIndex();
    ledMatrix_clear();
    ledMatrix_display_back();
}

void Menu_enterMenu(){
    isMenu = true;
    ledMatrix_display_song_list();
}

void Menu_exitMenu(){
    isMenu = false;   
    ledMatrix_clear();
    Song_data_exitMenuDisplay();
}