/* print_clipboard.c
 * prints the contents of the clipboard to standard output */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "%s <clipboard_dir>", argv[0]);
        exit(1);
    }

    int clip = clipboard_connect(argv[1]);
    if (clip == -1) perror("clipboard_connect failed\n");

    char str[100];
    int bytes;
    for (int i = 0; i < 10; i++) {
        memset(str, 0, sizeof(str));
        bytes = clipboard_paste(clip, i, str, sizeof(str));
        if (bytes == 0) perror("clipboard_paste failed\n");
        printf("Region %d -- %s (%d bytes)\n", i, str, bytes);
    }

    clipboard_close(clip);
}
