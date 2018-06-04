/* pastes.c
 * pastes something to std output from a certain region (argv[1]) */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("./paste region");
        exit(1);
    }
    
    char string[100];
    int region = strtol(argv[1], NULL, 0);

    int clip = clipboard_connect("../../bin");

    printf("Pasted %s from region %d (%d bytes)\n", string, region, clipboard_paste(clip, region, string, sizeof(string)));

    clipboard_close(clip);

    exit(0);
}
