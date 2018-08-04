#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "led_matrix.h"
#include <ctype.h>
#include <song_data.h>
#include <pthread.h>

#include "song_data.h"
#include "joystick.h"
#include "menu.h"

/*** GLOBAL VARIABLE ***/
/* GPIO PATH */
#define GPIO_PATH "/sys/class/gpio/"

/* GPIO Pins Definition */
#define RED1_PIN 8 // UPPER
#define GREEN1_PIN 80
#define BLUE1_PIN 78
#define RED2_PIN 76 // LOWER
#define GREEN2_PIN 79
#define BLUE2_PIN 74
#define CLK_PIN 73   // Arrival of each data
#define LATCH_PIN 75 // End of a row of data
#define OE_PIN 71    // Transition from one row to another
#define A_PIN 72     // Row select
#define B_PIN 77
#define C_PIN 70

#define S_IWRITE "S_IWUSR"

/* TIMING */
#define SLIDE_TIME_IN_US 200000000
#define DELAY_IN_US 5
#define DELAY_IN_SEC 0

/* LED Screen Values */
static int screen[32][16];
#define SCREEN_WIDTH 32
#define SCREEN_HEIGHT 16

/* FILES HANDLER */
static int fileDesc_red1;
static int fileDesc_blue1;
static int fileDesc_green1;
static int fileDesc_red2;
static int fileDesc_blue2;
static int fileDesc_green2;
static int fileDesc_clk;
static int fileDesc_latch;
static int fileDesc_oe;
static int fileDesc_a;
static int fileDesc_b;
static int fileDesc_c;

/* LED ALPHABET */
static int A[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int B[7][3] = {{1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}};
static int C[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 1, 0}};
static int D[7][3] = {{1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}};
static int E[7][3] = {{1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 1, 1}};
static int F[7][3] = {{1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}};
static int G[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int H[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int I[7][3] = {{1, 1, 1}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {1, 1, 1}};
static int J[7][3] = {{0, 1, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int K[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int L[7][3] = {{1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 1, 1}};
static int M[7][3] = {{1, 0, 1}, {1, 1, 1}, {1, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int N[7][3] = {{1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int O[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int P[7][3] = {{1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}};
static int Q[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
static int R[7][3] = {{1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}, {1, 0, 0}, {1, 1, 0}, {1, 0, 1}};
static int S[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int T[7][3] = {{1, 1, 1}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}};
static int U[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}};
static int V[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int W[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 1, 1}, {1, 0, 1}};
static int X[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int Y[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}};
static int Z[7][3] = {{1, 1, 1}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 1, 1}};

static int a[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 1}, {1, 1, 1}, {1, 0, 1}, {1, 1, 1}};
static int b[7][3] = {{1, 0, 0}, {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}};
static int c[7][3] = {{0, 0, 0}, {0, 0, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {0, 1, 1}};
static int d[7][3] = {{0, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 1}};
static int e[7][3] = {{0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 0, 1}, {1, 1, 1}, {1, 0, 0}, {0, 1, 1}};
static int f[7][3] = {{0, 0, 1}, {0, 1, 0}, {1, 1, 1}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}};
static int g[7][3] = {{0, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 1}, {0, 0, 1}, {1, 1, 0}};
static int h[7][3] = {{1, 0, 0}, {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int i[7][3] = {{0, 1, 0}, {0, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {1, 1, 1}};
static int j[7][3] = {{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {1, 1, 0}};
static int k[7][3] = {{1, 0, 0}, {1, 0, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}};
static int l[7][3] = {{1, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {1, 1, 1}};
static int m[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 1}, {1, 0, 1}, {1, 0, 1}};
static int n[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}};
static int o[7][3] = {{0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int p[7][3] = {{1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}, {1, 0, 0}, {1, 0, 0}};
static int q[7][3] = {{0, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 1}, {0, 0, 1}, {0, 0, 1}};
static int r[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}};
static int s[7][3] = {{0, 0, 0}, {0, 0, 0}, {0, 1, 1}, {1, 0, 0}, {1, 1, 1}, {0, 0, 1}, {1, 1, 0}};
static int t[7][3] = {{0, 1, 0}, {0, 1, 0}, {1, 1, 1}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, 1}};
static int u[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 1}};
static int v[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int w[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 1, 1}, {0, 1, 1}};
static int x[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}, {1, 0, 1}, {1, 0, 1}};
static int y[7][3] = {{1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {0, 1, 1}, {0, 0, 1}, {1, 1, 0}};
static int z[7][3] = {{0, 0, 0}, {0, 0, 0}, {1, 1, 1}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {1, 1, 1}};

/* LED INTRO CHARACTERS */
static int SFU_S[8][5] = {{0, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 0, 0, 0}, {1, 1, 1, 0, 0}, {0, 1, 1, 1, 1}, {0, 0, 0, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 0}};
static int SFU_F[8][5] = {{1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 0, 0, 0}, {1, 1, 1, 1, 1}, {1, 1, 1, 1, 1}, {1, 1, 0, 0, 0}, {1, 1, 0, 0, 0}, {1, 1, 0, 0, 0}};
static int SFU_U[8][5] = {{1, 1, 0, 1, 1}, {1, 1, 0, 1, 1}, {1, 1, 0, 1, 1}, {1, 1, 0, 1, 1}, {1, 1, 0, 1, 1}, {1, 1, 0, 1, 1}, {1, 1, 0, 1, 1}, {0, 1, 1, 1, 0}};

/* LED SPECIAL CHARACTERS */
static int tab[7][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 1, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
static int colon[7][3] = {{0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 1, 0}, {0, 0, 0}};

/* LED NUMBERS 0-9 */
static int zero[7][3] = {{0, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 0}};
static int one[7][3] = {{0, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {1, 1, 1}};
static int two[7][3] = {{0, 1, 0}, {1, 0, 1}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 1, 1}};
static int three[7][3] = {{0, 1, 0}, {1, 0, 1}, {0, 0, 1}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int four[7][3] = {{0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 0, 1}, {0, 0, 1}};
static int five[7][3] = {{1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 0, 1}, {0, 0, 1}, {1, 1, 0}};
static int six[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int seven[7][3] = {{1, 1, 1}, {0, 0, 1}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}};
static int eight[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {0, 1, 0}};
static int nine[7][3] = {{0, 1, 0}, {1, 0, 1}, {1, 0, 1}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {0, 1, 0}};

/* Arrays containing the characters */
static int *uppercaseAlphabet[] = {
    &A[0][0], &B[0][0], &C[0][0], &D[0][0], &E[0][0], &F[0][0], &G[0][0], &H[0][0], &I[0][0],
    &J[0][0], &K[0][0], &L[0][0], &M[0][0], &N[0][0], &O[0][0], &P[0][0], &Q[0][0], &R[0][0],
    &S[0][0], &T[0][0], &U[0][0], &V[0][0], &W[0][0], &X[0][0], &Y[0][0], &Z[0][0]};

static int *lowercaseAlphabet[] = {
    &a[0][0], &b[0][0], &c[0][0], &d[0][0], &e[0][0], &f[0][0], &g[0][0], &h[0][0], &i[0][0],
    &j[0][0], &k[0][0], &l[0][0], &m[0][0], &n[0][0], &o[0][0], &p[0][0], &q[0][0], &r[0][0],
    &s[0][0], &t[0][0], &u[0][0], &v[0][0], &w[0][0], &x[0][0], &y[0][0], &z[0][0]};

static int *numbers[] = {
    &zero[0][0], &one[0][0], &two[0][0], &three[0][0], &four[0][0], &five[0][0], &six[0][0],
    &seven[0][0], &eight[0][0], &nine[0][0]};

static int *SFU[] = {&SFU_S[0][0], &SFU_F[0][0], &SFU_U[0][0]};

/* Miscellaneous declared static variables */

static int alphabetRows = 7;
static int alphabetCols = 3;
static int titleLength = 0;
static int **ledTrack;
static int **ledAlbum;
static int **ledartist;
static int **ledTrackTime;

#define BACKGROUND_COLOUR RED
#define MENU_COLOR WHITE
#define INFO_COLOR LIGHT_BLUE

static int currentSong = 0;
static int currentSongIndex = 0;
static const int songIndexSize = 3; // Artist, Album, Year

static _Bool stop = false;
static pthread_t timerThreadId;

static void *timerThread(void *arg)
{
    while (!stop)
    {
        ledMatrix_music_timer(Song_data_getTimer(), 16764159, 0);
    }
    return NULL;
}

//static int reallocLength;
/**
 * exportAndOut
 * Export a pin and set the direction to output
 * @params
 *  int pinNum: the pin number to be exported and set for output
 */
static void exportAndOut(int pinNum)
{
    // Export the gpio pins
    FILE *gpioExP = fopen(GPIO_PATH "export", "w");
    if (gpioExP == NULL)
    {
        printf("ERROR: Unable to open export file.\n");
        exit(-1);
    }
    fprintf(gpioExP, "%d", pinNum);
    fclose(gpioExP);

    // Change the direction into out
    char fileNameBuffer[1024];
    sprintf(fileNameBuffer, GPIO_PATH "gpio%d/direction", pinNum);

    FILE *gpioDirP = fopen(fileNameBuffer, "w");
    fprintf(gpioDirP, "out");
    fclose(gpioDirP);

    return;
}

/**
 * ledMatrix_setupPins
 * Setup the pins used by the led matrix, by exporting and set the direction to out
 */
static void ledMatrix_setupPins(void)
{
    // Upper led
    exportAndOut(RED1_PIN);
    fileDesc_red1 = open("/sys/class/gpio/gpio8/value", O_WRONLY, S_IWRITE);
    exportAndOut(GREEN1_PIN);
    fileDesc_green1 = open("/sys/class/gpio/gpio80/value", O_WRONLY, S_IWRITE);
    exportAndOut(BLUE1_PIN);
    fileDesc_blue1 = open("/sys/class/gpio/gpio78/value", O_WRONLY, S_IWRITE);

    // Lower led
    exportAndOut(RED2_PIN);
    fileDesc_red2 = open("/sys/class/gpio/gpio76/value", O_WRONLY, S_IWRITE);
    exportAndOut(GREEN2_PIN);
    fileDesc_green2 = open("/sys/class/gpio/gpio79/value", O_WRONLY, S_IWRITE);
    exportAndOut(BLUE2_PIN);
    fileDesc_blue2 = open("/sys/class/gpio/gpio74/value", O_WRONLY, S_IWRITE);

    // Timing
    exportAndOut(CLK_PIN);
    fileDesc_clk = open("/sys/class/gpio/gpio73/value", O_WRONLY, S_IWRITE);
    exportAndOut(LATCH_PIN);
    fileDesc_latch = open("/sys/class/gpio/gpio75/value", O_WRONLY, S_IWRITE);
    exportAndOut(OE_PIN);
    fileDesc_oe = open("/sys/class/gpio/gpio71/value", O_WRONLY, S_IWRITE);

    // Row Select
    exportAndOut(A_PIN);
    fileDesc_a = open("/sys/class/gpio/gpio72/value", O_WRONLY, S_IWRITE);
    exportAndOut(B_PIN);
    fileDesc_b = open("/sys/class/gpio/gpio77/value", O_WRONLY, S_IWRITE);
    exportAndOut(C_PIN);
    fileDesc_c = open("/sys/class/gpio/gpio70/value", O_WRONLY, S_IWRITE);

    return;
}

/** 
 *  ledMatrix_clock
 *  Generate the clock pins
 */
static void ledMatrix_clock(void)
{
    // Bit-bang the clock gpio
    // Notes: Before program writes, must make sure it's on the 0 index
    lseek(fileDesc_clk, 0, SEEK_SET);
    write(fileDesc_clk, "1", 1);
    lseek(fileDesc_clk, 0, SEEK_SET);
    write(fileDesc_clk, "0", 1);

    return;
}

/**
 *  ledMatrix_latch
 *  Generate the latch pins
 */
static void ledMatrix_latch(void)
{
    lseek(fileDesc_latch, 0, SEEK_SET);
    write(fileDesc_latch, "1", 1);
    lseek(fileDesc_latch, 0, SEEK_SET);
    write(fileDesc_latch, "0", 1);

    return;
}

/**
 *  ledMatrix_bitsFromInt
 *  Convert integer passed on into bits and put in array
 *  @params:
 *      int * arr: pointer to array to be filled with bits
 *      int input: integer to be converted to bits
 */
static void ledMatrix_bitsFromInt(int *arr, int input)
{
    arr[0] = input & 1;

    arr[1] = input & 2;
    arr[1] = arr[1] >> 1;

    arr[2] = input & 4;
    arr[2] = arr[2] >> 2;

    return;
}

/**
 *  ledMatrix_setRow
 *  Set LED Matrix row
 *  @params:
 *      int rowNum: the rowNumber to be inputted to row pins
 */
static void ledMatrix_setRow(int rowNum)
{
    // Convert rowNum single bits from int
    int arr[3] = {0, 0, 0};
    ledMatrix_bitsFromInt(arr, rowNum);

    // Write on the row pins
    char a_val[2];
    sprintf(a_val, "%d", arr[0]);
    lseek(fileDesc_a, 0, SEEK_SET);
    write(fileDesc_a, a_val, 1);

    char b_val[2];
    sprintf(b_val, "%d", arr[1]);
    lseek(fileDesc_b, 0, SEEK_SET);
    write(fileDesc_b, b_val, 1);

    char c_val[2];
    sprintf(c_val, "%d", arr[2]);
    lseek(fileDesc_c, 0, SEEK_SET);
    write(fileDesc_c, c_val, 1);

    return;
}

/**
 *  ledMatrix_setColourTop
 *  Set the colour of the top part of the LED
 *  @params:
 *      int colour: colour to be set
 */
static void ledMatrix_setColourTop(int colour)
{;
    int arr[3] = {0, 0, 0};
    ledMatrix_bitsFromInt(arr, colour);

    // Write on the colour pins
    char red1_val[2];
    sprintf(red1_val, "%d", arr[0]);
    lseek(fileDesc_red1, 0, SEEK_SET);
    write(fileDesc_red1, red1_val, 1);

    char green1_val[2];
    sprintf(green1_val, "%d", arr[1]);
    lseek(fileDesc_green1, 0, SEEK_SET);
    write(fileDesc_green1, green1_val, 1);

    char blue1_val[2];
    sprintf(blue1_val, "%d", arr[2]);
    lseek(fileDesc_blue1, 0, SEEK_SET);
    write(fileDesc_blue1, blue1_val, 1);

    return;
}

/**
 *  ledMatrix_setColourBottom
 *  Set the colour of the bottom part of the LED
 *  @params:
 *      int colour: colour to be set
 */
static void ledMatrix_setColourBottom(int colour)
{
    int arr[3] = {0, 0, 0};
    ledMatrix_bitsFromInt(arr, colour);

    // Write on the colour pins
    char red2_val[2];
    sprintf(red2_val, "%d", arr[0]);
    lseek(fileDesc_red2, 0, SEEK_SET);
    write(fileDesc_red2, red2_val, 1);

    char green2_val[2];
    sprintf(green2_val, "%d", arr[1]);
    lseek(fileDesc_green2, 0, SEEK_SET);
    write(fileDesc_green2, green2_val, 1);

    char blue2_val[2];
    sprintf(blue2_val, "%d", arr[2]);
    lseek(fileDesc_blue2, 0, SEEK_SET);
    write(fileDesc_blue2, blue2_val, 1);

    return;
}
/**
 *  ledMatrix_refresh
 *  Fill the LED Matrix with the respective pixel colour
 */
void ledMatrix_refresh(void)
{
    for (int rowNum = 0; rowNum < 8; rowNum++)
    {
        lseek(fileDesc_oe, 0, SEEK_SET);
        write(fileDesc_oe, "1", 1);
        ledMatrix_setRow(rowNum);
        for (int colNum = 0; colNum < 32; colNum++)
        {
            ledMatrix_setColourTop(screen[colNum][rowNum]);
            ledMatrix_setColourBottom(screen[colNum][rowNum + 8]);
            ledMatrix_clock();
        }
        ledMatrix_latch();
        lseek(fileDesc_oe, 0, SEEK_SET);
        write(fileDesc_oe, "0", 1);
        struct timespec reqDelay = {0, 1000000};
        nanosleep(&reqDelay, (struct timespec *)NULL);
    }
    return;
}

/**
 *  ledMatrix_setPixel
 *  Set the pixel selected on LED MAtrix with the colour selected
 *  @params:
 *      int x: x-axis
 *      int y: y-axis
 *      int colour: colour selected
 */

static void ledMatrix_setPixel(int x, int y, int colour)
{
    screen[y][x] = colour;

    return;
}

/**
 *  Initialize the LED Matrix GPIO pins and also set the entire screen array to have values of 0.
 */

_Bool* ledMatrix_init(pthread_t* pSplashScreenThread)
{
    memset(screen, 0, sizeof(screen));
    ledMatrix_setupPins();

    _Bool* splashScreenPlaying = malloc(sizeof(_Bool));
    pthread_create(pSplashScreenThread, NULL, ledMatrix_splash_screen, splashScreenPlaying);
    return splashScreenPlaying;
}

/**
 *  The splash screen that appears after the call to init.
 *  Currently splash screen lasts for 3 seconds, however this can be changed anytime.
 */

void ledMatrix_start_music_timer(_Bool start)
{   
    if(start) {
        pthread_create(&timerThreadId, NULL, timerThread, NULL);
    }
    else{
        stop = true;
        pthread_join(timerThreadId, NULL);
    }
}

/**
 * Method is used to set specific sections of the LED screen, by specificing which row and columns to begin and end. 
 */

void ledMatrix_fill_screen(int start_row, int end_row, int start_col, int end_col, int colour) {

    for (int row = start_row; row < end_row; row += 2)
    {
        for (int col = start_col; col < end_col; col++)
        {
            if (col % 2 == 0)
            {
                ledMatrix_setPixel(row, col, colour);
            }
            else
            {
                ledMatrix_setPixel(row + 1, col, colour);
            }
        }
    }
}

/**
 * Besides being obviously used as a splash screen it is also used to show something at the 
 * beginning while the system is still loading the metadata of all the songs in the song directory.
 */

void* ledMatrix_splash_screen(void* ptr)
{
    _Bool* playing = (_Bool *)ptr;

    time_t endwait;
    time_t start = time(NULL);
    time_t second = 3; // end loop after this time has elapsed
    memset(screen, 0, sizeof(screen));
    for (int row = 0; row < SCREEN_HEIGHT; row += 2)
    {
        for (int col = 0; col < SCREEN_WIDTH; col++)
        {
            if (col % 2 == 0)
            {
                ledMatrix_setPixel(row, col, 1);
            }
            else
            {
                ledMatrix_setPixel(row + 1, col, 1);
            }
        }
    }
    int increment = 15;
    for (int k = 0; k < 3; k++)
    {
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                if (SFU[k][i * 5 + j] == 1)
                {
                    ledMatrix_setPixel(i + 8, j + increment, WHITE);
                }
            }
        }
        increment += 6;
    }

    endwait = start + second;

    while (*playing || (start < endwait))
    {
        ledMatrix_refresh();
        start = time(NULL);
    }
    memset(screen, 0, sizeof(screen));
    ledMatrix_refresh();
    return NULL;
}

/**
 *  Takes a string and extracts each character from it, however the supported extractable characters at the moment is only:
 * 
 *      1. Entire alphabet (upper and lower case) 
 *      2. Numbers
 *      3. Colons
 *      4. Dashes
 *      5. Spaces
 * 
 *  Anything not listed above will be skipped and in place will be something similar to a space.
 *  
 *  @params:
 *      char *string: Track, artist, album, song duration, etc..                
 */

int **ledMatrix_extract_string(char *string)
{
    int length = titleLength = strlen(string);
    int **ptr = (int **)malloc(length * sizeof(int *));
    memset(ptr, 0, length * sizeof(int *));

    for (int i = 0; i < length; i++)
    {
        if (string[i] >= 'A' && string[i] <= 'Z')
        {
            ptr[i] = uppercaseAlphabet[string[i] - 'A'];
        }
        else if (string[i] >= 'a' && string[i] <= 'z')
        {
            ptr[i] = lowercaseAlphabet[string[i] - 'a'];
        }
        else if (string[i] == ' ')
        {
            ptr[i] = NULL; // space
        }
        else if (string[i] == '-')
        {
            ptr[i] = &tab[0][0];
        }
        else if (string[i] == ':')
        {
            ptr[i] = &colon[0][0];
        }
        else if (isdigit(string[i]))
        {
            ptr[i] = numbers[string[i] - '0'];
        }
        else
        {
            continue;
        }
    }

    return ptr;
}

/**
 *  Grabs the track name and displays to be seen on the LED matrix and if the name is too long 
 *  it returns a none-zero value indicating that we should call the ledMatrix_slideTrack() method. 
 */

int ledMatrix_music_details(char *track, int colour, int rowOffSet)
{
    ledMatrix_clear_top();
    ledTrack = ledMatrix_extract_string(track);

    int increment = 0;
    int increment2 = 0;
    int isrealloc = 0;
    int temp = 0;
    int counter = strlen(track);

    if ((strlen(track) * 3 + strlen(track) - 1) > SCREEN_WIDTH)
    {
        for (int j = 0; j < strlen(track); j++)
        {
            if (ledTrack[increment] == NULL)
            {
                increment2 -=2;
                isrealloc = 1;
                counter --;
            }
            if (isrealloc != 1)
            {
                for (int rows = 0; rows < alphabetRows; rows++)
                {
                    for (int cols = 0; cols < alphabetCols; cols++)
                    {
                        if (ledTrack[increment][rows * 3 + cols] == 1 && cols + increment2 < SCREEN_WIDTH && rows + rowOffSet >= 0 && rows + rowOffSet < 16)
                        {
                            ledMatrix_setPixel(rows + rowOffSet, cols + increment2, colour);
                            isrealloc = 1;
                        }
                    }
                }
            }

            if (isrealloc == 1)
            {
                ledTrack = (int **)realloc(ledTrack, (strlen(track) + temp + 1) * sizeof(int *));
                ledTrack[strlen(track) + temp] = &ledTrack[temp][0];
                if (&ledTrack[temp][0] != NULL)
                {
                    counter += 1;
                }
                isrealloc = 0;
                temp += 1;
            }
            increment++;
            increment2 += 4;
        }
        return strlen(track) + temp;
    }
    else
    {
        int offset = 0;//(abs(SCREEN_WIDTH - (strlen(track) + (strlen(track) * 3))) / 2);
        for (int j = 0; j < strlen(track); j++)
        {
            if (ledTrack[increment] == NULL)
            {
                increment2 += 1;
                increment++;
                continue;
            }
            for (int rows = 0; rows < alphabetRows; rows++)
            {
                for (int cols = 0; cols < alphabetCols; cols++)
                {
                    if (ledTrack[increment][rows * 3 + cols] == 1 && cols + increment2 < SCREEN_WIDTH && rows + rowOffSet >= 0 && rows + rowOffSet < 16)
                    {
                        ledMatrix_setPixel(rows + rowOffSet, offset + cols + increment2, colour);
                    }
                }
            }
            increment++;
            increment2 += 4;
        }
    }
    return 0;
}

/**
 *  Method used to slide the track across the display from left to right it wraps around the display in an animated fashion.
 */

static void ledMatrix_slideTrack(int **ledT, int track, int colour, int rowOffSet)
{
    int breakout = -1;
    for (int i = 0; i < titleLength; i++)
    {
        if (&ledT[i][0] == NULL)
        {
            breakout += 1;
        }
        else
        {
            breakout += 4;
        }
    }
    int remainder = breakout % 32;
    int multiplier = breakout / 32;
    int increment = 0;
    int increment2 = 0;

    for (int test = 0; test >= -1 * ((remainder + 32 * multiplier) + 1); test--)
    {
        ledMatrix_clear_top();
        increment = 0;
        increment2 = 0;

        for (int j = 0; j < (track); j++)
        {
            if (ledT[increment] == NULL)
            {
                increment2 += 1;
                increment++;
                continue;
            }
            for (int rows = 0; rows < alphabetRows; rows++)
            {
                for (int cols = 0; cols < alphabetCols; cols++)
                {
                    if (ledT[increment][rows * 3 + cols] == 1 && cols + increment2 + test < SCREEN_WIDTH && test + cols + increment2 >= 0)
                    {
                        ledMatrix_setPixel(rows + rowOffSet, test + cols + increment2, colour);
                    }
                }
            }
            increment++;
            increment2 += 4;
        }

        struct timespec reqDelay = {DELAY_IN_SEC, 40000000};
        nanosleep(&reqDelay, (struct timespec *)NULL);
    }
    return;
}

/**
 * A method to encapsulate the necessary methods to display the track name and also slide the track name if it's too long.
 */
void ledMatrix_music_track_display(char *track, int colour, int rowOffSet)
{
    int isOverflow = ledMatrix_music_details(track, colour, rowOffSet);
    if (isOverflow != 0)
    {
            ledMatrix_slideTrack(ledTrack, isOverflow, colour, rowOffSet);
    }
}

/**
 * Handles the update of the total time and remaining time of a song on the display when playing a song. 
 */

void ledMatrix_music_timer(int duration, int colour, int horizontalOffset)
{
    if (duration >= 600 || duration < 0)
        duration = 0;

    char bufferMinutes[128];
    char bufferSeconds[128];
    int seconds = duration % 60;
    int minutes = (duration - seconds) / 60;

    sprintf(bufferMinutes, "%d", minutes);
    sprintf(bufferSeconds, "%d", seconds);

    strcat(bufferMinutes, ":");
    if (seconds < 10)
    {
        strcat(bufferMinutes, "0");
    }
    strcat(bufferMinutes, bufferSeconds);
    strcat(bufferMinutes, "\0");

    int increment = 0;
    int increment2 = 0;

    ledTrackTime = ledMatrix_extract_string(bufferMinutes);
    int verticalOffset = 9;

    for (int j = 0; j < strlen(bufferMinutes); j++)
    {
        if (ledTrackTime[increment] == NULL)
        {
            increment2 += 1;
            increment++;
            continue;
        }
        for (int rows = 0; rows < alphabetRows; rows++)
        {
            for (int cols = 0; cols < alphabetCols; cols++)
            {
                if (ledTrackTime[increment][rows * 3 + cols] == 1 && cols + increment2 < SCREEN_WIDTH)
                {
                    ledMatrix_setPixel(verticalOffset + rows, cols + increment2, colour);
                    ledMatrix_setPixel(verticalOffset + rows, horizontalOffset + cols + increment2, colour);
                }
            }
        }
        increment++;
        // This is a hack and assumes the song we are getting is below 10 minutes.
        if(increment == 1 || increment ==2) {
            increment2 += 3;
        }
        else{
        increment2 += 4;
        }
    }
    
    if (horizontalOffset == 0) {
        // for (int rows = 7; rows < SCREEN_HEIGHT; rows++){
        //     for (int cols = 0; cols < 13; cols++){
        //         ledMatrix_setPixel(rows, cols, 0);
        //     }
        // }
    }
    else {
        ledMatrix_setPixel(9, 18, 5);
        ledMatrix_setPixel(10, 17, 5);
        ledMatrix_setPixel(11, 16, 5);
        ledMatrix_setPixel(12, 15, 5);
        ledMatrix_setPixel(13, 14, 5);
        ledMatrix_setPixel(14, 13, 5);
    }
    free(ledTrackTime);
    ledTrackTime = NULL;
}

/**
 * Method used to exclusively clear only the bottom of the LED display from row 8 to 15
 */

void ledMatrix_clear_bottom()
{
    for (int rows = 8; rows < SCREEN_HEIGHT; rows++)
    {
        for (int cols = 0; cols < SCREEN_WIDTH; cols++)
        {

            ledMatrix_setPixel(rows, cols, 0);
        }
    }
    int background_colour = BACKGROUND_COLOUR;
    if (!Menu_isMenu()) background_colour = 0;
    ledMatrix_fill_screen(8, SCREEN_HEIGHT, 0, SCREEN_WIDTH, background_colour);
}

/**
 * Method used to exclusively clear only the top of the LED display from row 0 to 6
 */

void ledMatrix_clear_top()
{
    for (int rows = 0; rows < 7; rows++)
    {
        for (int cols = 0; cols < SCREEN_WIDTH; cols++)
        {

            ledMatrix_setPixel(rows, cols, 0);
        }
    }
    int background_colour = BACKGROUND_COLOUR;
    if (!Menu_isMenu()) background_colour = 0;
    ledMatrix_fill_screen(0, 7, 0, SCREEN_WIDTH, background_colour);
}
/**
 *  Thread cleanup and when program ends.
 */
void ledMatrix_clean()
{
    stop = true;
    pthread_join(timerThreadId, NULL);
    free(ledTrack);
    free(ledAlbum);
    free(ledartist);
    if(ledTrackTime != NULL) {
        free(ledTrackTime);
    }
    ledTrack = ledAlbum = ledartist = NULL;
}

/**
 * Used to clear display and set its background to a specific color value dpeneding on which page you are on when using the LED display.
 */

void ledMatrix_clear()
{
    memset(screen, 0, sizeof(screen));
    int background_colour = BACKGROUND_COLOUR;
    if (!Menu_isMenu()) background_colour = 0;
    ledMatrix_fill_screen(0, SCREEN_HEIGHT, 0, SCREEN_WIDTH, background_colour);
    ledMatrix_refresh();
}

void ledMatrix_timer_clear(){
    for (int rows = 8; rows < SCREEN_HEIGHT; rows++){
        for (int cols = 0; cols < 13; cols++){
            ledMatrix_setPixel(rows, cols, 0);
        }
    }
    int background_colour = RED;
    if (!Menu_isMenu()) background_colour = 0;
    for (int row = 8; row < SCREEN_HEIGHT; row += 2)
    {
        for (int col = 0; col < 13; col++)
        {
            if (col % 2 == 0)
            {
                ledMatrix_setPixel(row, col, background_colour);
            }
            else
            {
                ledMatrix_setPixel(row + 1, col, background_colour);
            }
        }
    }
}

/**
 * Display the songs in a directory.
 */
void ledMatrix_display_song_list(){
    ledMatrix_clear();
    
    currentSong = 0; 

    int offset = 4;
    int colour = MENU_COLOR;
    ledMatrix_music_details(Song_data_getSongName(currentSong), colour, offset);
}

void ledMatrix_display_next_song(){
    ledMatrix_clear();
    int songBufferSize = Song_data_getSongBufferSize();
    if (currentSong >= (songBufferSize - 1))
    {
        currentSong = 0;
    }
    else{
        currentSong++;
    }

    int offset = 4;
    int colour = MENU_COLOR;
    ledMatrix_music_details(Song_data_getSongName(currentSong), colour, offset);
}

void ledMatrix_display_prev_song(){
    ledMatrix_clear();
    int songBufferSize = Song_data_getSongBufferSize();
    if (currentSong <= 0)
    {
        currentSong = songBufferSize - 1;
    }
    else{
        currentSong--;
    }

    int offset = 4;
    int colour = MENU_COLOR;
    ledMatrix_music_details(Song_data_getSongName(currentSong), colour, offset);
}

int ledMatrix_display_info(int index){ 
    ledMatrix_clear();
    int offset = 4;
    int colour = INFO_COLOR;

    // Artist
    if(index == 0){
        char* artist = Song_data_getArtist(currentSong);

        if (artist == NULL){
            return -1;
        }

        ledMatrix_music_details(artist, colour, offset);
    }
    // Album
    else if (index == 1){
        char* album = Song_data_getAlbum(currentSong);

        if (album == NULL){
            return -1;
        }

        ledMatrix_music_details(album, colour, offset);
    }
    // Year
    else if (index == 2){
        char* year = Song_data_getYear(currentSong);

        if (year == NULL){
            return -1;
        }

        ledMatrix_music_details(year, colour, offset);
    }

    return 0;
}


void ledMatrix_display_next_info(){
    if(currentSongIndex >= (songIndexSize - 1)){
        currentSongIndex = 0;
    }
    else{
        currentSongIndex++;
    }

    printf("Next Index: %d\n", currentSongIndex);

    if (ledMatrix_display_info(currentSongIndex) == -1){
        ledMatrix_display_next_info();
    }
}

void ledMatrix_display_prev_info(){
    if(currentSongIndex <= 0){
        currentSongIndex = songIndexSize - 1;
    }
    else{
        currentSongIndex--;
    }
    printf("Prev Index: %d\n", currentSongIndex);

    if (ledMatrix_display_info(currentSongIndex) == -1){
        ledMatrix_display_prev_info();
    }
}

void ledMatrix_display_back(){
    int offset = 4;
    int colour = MENU_COLOR;
    ledMatrix_music_details(Song_data_getSongName(currentSong), colour, offset);
}

void ledMatrix_clear_infoIndex(){
    currentSongIndex = 0;
}

void ledMatrix_clear_menu(){
    currentSong = 0;
}

int ledMatrix_getCurrentSong(){
    return currentSong;
}