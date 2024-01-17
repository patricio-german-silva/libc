#include "stdint.h"
#include <stdio.h>
#include <unistd.h>
#include "ff.h"
#include "diskio.h"






FATFS FatFs;   /* Work area (filesystem object) for logical drive */

// Main program
int main(int argc, char *argv[]){
    FIL fil;        /* File object */
    char line[100]; /* Line buffer */
    FRESULT fr;     /* FatFs return code */


    /* Give a work area to the default drive */
    f_mount(&FatFs, "", 0);

    /* Open a text file */
    fr = f_open(&fil, "message.txt", FA_READ);
    if (fr) return (int)fr;

    /* Read every line and display it */
    while (f_gets(line, sizeof line, &fil)) {
        printf(line);
    }

    f_close(&fil);

    fr = f_open(&fil, "newe.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    if (fr) return (int)fr;
    f_puts("Specifies write access to the file. Data can be written to the file. Combine with FA_READ for read-write access.\0", &fil);
    f_puts("The input Unicode characters in multiple encoding unit, such as surrogate pair and multi-byte sequence.\n\0", &fil);
    f_close(&fil);
    /* Open a text file */
    fr = f_open(&fil, "newe.txt", FA_READ);
    if (fr) return (int)fr;

    /* Read every line and display it */
    while (f_gets(line, sizeof line, &fil)) {
        printf(line);
    }

    f_close(&fil);
    return 0;
}
