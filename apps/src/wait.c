/* wait.c
 * waits to paste something to std output from a certain region (argv[1]) */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("./wait region");
        exit(1);
    }
    
    char string[100] = {0};
    int region = strtol(argv[1], NULL, 0);

    int clip = clipboard_connect("../../bin");

    printf("Waited to paste %s from region %d (%d bytes)\n", string, region, clipboard_wait(clip, region, string, strlen(string) + 1));

    clipboard_close(clip);

    exit(0);
}
