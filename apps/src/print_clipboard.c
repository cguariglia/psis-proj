/* print_clipboard.c
 * prints the contents of the clipboard to standard output */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(int argc, char *argv[]){
    char str[100];

    int clip = clipboard_connect(SERVER_ADDRESS);
    
    for (int i = 0; i < 10; i++) {
        clipboard_copy(clip, i, buf, sizeof(buf));
        printf("Region %d -- %s\n", i, buf)
    }
    
    clipboard_close(clip);

    exit(0);
}
