/* paste.c
 * pastes something to stdout from a certain region (argv[2]) */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <clipboard.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "%s <clipboard_dir> <region>", argv[0]);
        exit(1);
    }

    int clip = clipboard_connect(argv[1]);
    if (clip == -1) perror("clipboard_connect failed\n");
    
    errno = 0;
    int region = strtol(argv[2], NULL, 0);
    if (errno != 0) perror("strtol failed\n");

    char buffer[100];
    int bytes = clipboard_paste(clip, region, buffer, sizeof(buffer));
    if (bytes == 0) perror("clipboard_paste failed\n");

    printf("Pasted %s from region %d (%d bytes)\n", buffer, region, bytes);

    clipboard_close(clip);
}
