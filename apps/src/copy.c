/* copy.c
 * copies something (argv[3]) to a certain region (argv[2]) */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <clipboard.h>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "%s <clipboard_dir> <region> <string>", argv[0]);
        exit(1);
    }

    int clip = clipboard_connect(argv[1]);
    if (clip == -1) perror("clipboard_connect failed\n");

    errno = 0;
    int region = strtol(argv[2], NULL, 0);
    if (errno != 0) perror("strtol failed\n");

    int bytes = clipboard_copy(clip, region, argv[3], strlen(argv[3]) + 1);
    if (bytes == 0) perror("clipboard_copy failed\n");

    printf("Copied %s to region %d (%d bytes)\n", argv[3], region, bytes);

    clipboard_close(clip);
}
