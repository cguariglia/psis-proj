/* copy.c
 * copies something (argv[1]) to a certain region (argv[2]) */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(int argc, char **argv){
    if (argc != 3) {
        printf("./copy string region");
        exit(1);
    }
    
    int region = strtol(argv[2], NULL, 0);
    char string[strlen(argv[1]) + 1];
    
    strcpy(string, argv[1]);
    
    int clip = clipboard_connect("../../bin");

    printf("Copied %s to region %d (%d bytes)\n", string, region, clipboard_copy(clip, region, string, strlen(string) + 1));

    clipboard_close(clip);

    exit(0);
}
