#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "pot.h"
#include "audio.h"

/******************************************************************************
 **              INTERNAL MACRO DEFINITIONS
 ******************************************************************************/
#define A2D_VIRTUAL_CAPE    "/sys/devices/platform/bone_capemgr/slots"
#define A2D_FILE_VOLTAGE0   "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define BB_ADC              "BB-ADC"
#define DATA_PTS_SIZE       (10)
#define SLEEP_SEC           (1)
#define POLL_SPEED_NS       (100000000)     // 100ms

/******************************************************************************
 **              INTERNAL VARIABLES
 ******************************************************************************/
static pthread_mutex_t potMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t potThreadId;
static _Bool stopping = false;
const int DATA_PTS[DATA_PTS_SIZE][2] = {
    {0, 10},
    {500, 20},
    {1000, 30},
    {1500, 40},
    {2000, 50},
    {2500, 60},
    {3000, 70},
    {3500, 80},
    {4000, 90},
    {4100, 100}};

/******************************************************************************
 **              INTERNAL FUNCTION PROTOTYPES
 ******************************************************************************/
static void* POT_thread(void* arg);
static int POT_getCurrVolt();
static void POT_loadCape();

/******************************************************************************
 **              FUNCTION DEFINITIONS
 ******************************************************************************/
void POT_init()
{
    printf("POT_init()\n");
    POT_loadCape();
    
    if (pthread_create(&potThreadId, NULL, POT_thread, NULL))
        printf("ERROR cannot create a new pot thread!");
}

void POT_cleanup()
{
    stopping = true;
    pthread_join(potThreadId, NULL);
}

int POT_getVolume()
{
    int s = POT_getCurrVolt();
    int a = -1, b = -1, m = -1, n = -1;

    for (int i = 0; i < DATA_PTS_SIZE; i++) {
        if (DATA_PTS[i][0] == s)
            return DATA_PTS[i][1];
        else if (i < (DATA_PTS_SIZE - 1) && s > DATA_PTS[i][0] && s < DATA_PTS[i + 1][0]) {
            a = DATA_PTS[i][0];
            b = DATA_PTS[i + 1][0];
            m = DATA_PTS[i][1];
            n = DATA_PTS[i + 1][1];
            break;
        }
    }

    if (a == -1 || b == -1 || m == -1 || n == -1) {
        printf("ERROR pot getArrSize: cannot get required data points to calculate array size\n");
        exit(-1);
    }

    return (int)((double)(s - a) / (b - a) * (n - m) + m);
}

static void* POT_thread(void* arg)
{
    while (!stopping) {
        pthread_mutex_lock(&potMutex);
        Audio_setVolume(POT_getVolume());
        pthread_mutex_unlock(&potMutex);
        nanosleep((const struct timespec[]){{0, POLL_SPEED_NS}}, NULL);
    }
    return NULL;
}

static void POT_loadCape()
{
    FILE *file = fopen(A2D_VIRTUAL_CAPE, "w");
    // Voltage input file is not immediately available after boot up, thus we wait a second and retry again.
    nanosleep((const struct timespec[]){{SLEEP_SEC, 0}}, NULL);

    if (!file) {
        printf("ERROR pot init: cannot load cape for write\n");
        exit(-1);
    }

    int dataWritten = fprintf(file, BB_ADC);
    if (dataWritten <= 0) {
        printf("ERROR pot init: cannot write data\n");
        exit(-1);
    }

    fclose(file);
}

static int POT_getCurrVolt()
{
    FILE *file = fopen(A2D_FILE_VOLTAGE0, "r");

    if (!file) {
        nanosleep((const struct timespec[]){{SLEEP_SEC, 0}}, NULL);
        file = fopen(A2D_FILE_VOLTAGE0, "r");
        if (!file) {
            printf("ERROR pot getCurrVolt: Cannot open voltage input file\n");
            exit(-1);
        }
    }

    int a2dReading = 0;
    int itemsRead = fscanf(file, "%d", &a2dReading);
    if (itemsRead <= 0) {
        printf("ERROR pot getCurrVolt: Cannot read values from voltage input file\n");
        exit(-1);
    }

    fclose(file);
    return a2dReading;
}