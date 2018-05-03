#include <stdio.h>
#include <stdlib.h>

#include <clipboard.h>

int main(){
    int fd = clipboard_connect(SERVER_ADDRESS);

    if(fd == -1){
        exit(-1);
    }

    char *data1 = "asdf";
    char *data2 = "qwerty";
    int region1 = 2;
    int region2 = 5;
    char str[5];
    int bytes;

    printf("Copying \"%s\" to clipboard region %d\n", data1, region1);
    bytes = clipboard_copy(fd, region1, (void *) data1, sizeof(data1));
    printf("Copied %d bytes to clipboard\n", bytes);
    printf("Copying \"%s\" to clipboard region %d\n", data2, region2);
    bytes = clipboard_copy(fd, region2, (void *) data2, sizeof(data2));
    printf("Copied %d bytes to clipboard\n", bytes);
    printf("Pasting data from clipboard region %d\n", region1);
    bytes = clipboard_paste(fd, region2, (void *) str, sizeof(str));
    printf("Pasted \"%s\", %d bytes\n", str, bytes);

    exit(0);
}
