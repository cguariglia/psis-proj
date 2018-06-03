/* print_clipboard.c
 * prints the contents of the clipboard to standard output */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(){
    char str[100];

    int clip = clipboard_connect("../../bin");

    for (int i = 0; i < 10; i++) {
        memset(str, 0, sizeof(str));
        clipboard_paste(clip, i, str, sizeof(str));
        printf("Region %d -- %s\n", i, str);
    }

    clipboard_close(clip);

    exit(0);
}
