#include <stdio.h>
#include <stdlib.h>

#include <clipboard.h>

int main(){
    int fd = clipboard_connect(SERVER_ADDRESS);

    if(fd == -1){
        exit(-1);
    }

    char data1[] = "asdsdsdsdf";
    char data2[] = "qwerty1234";
    char data3[] = "eheh replaced ya";
    int region1 = 2;
    int region2 = 5;
    char str[100];
    int bytes;

printf("sizeof(data1) == %d\nsizeof(data2) == %d\n", sizeof(data1), sizeof(data2));

    printf("Copying \"%s\" to clipboard region %d\n", data1, region1);
    bytes = clipboard_copy(fd, region1, (void *) data1, sizeof(data1));
    printf("Copied %d bytes to clipboard\n", bytes);

    sleep(1);

    printf("Copying \"%s\" to clipboard region %d\n", data2, region2);
    bytes = clipboard_copy(fd, region2, (void *) data2, sizeof(data2));
    printf("Copied %d bytes to clipboard\n", bytes);

    sleep(1);

    printf("Pasting data from clipboard region %d\n", region1);
    bytes = clipboard_paste(fd, region1, (void *) str, sizeof(data1));
    printf("Pasted \"%s\", %d bytes\n", str, bytes);

    sleep(1);

    printf("Pasting data from clipboard region %d\n", region2);
    bytes = clipboard_paste(fd, region2, (void *) str, sizeof(data2));
    printf("Pasted \"%s\", %d bytes\n", str, bytes);

    sleep(1);

    printf("Copying \"%s\" to clipboard region %d\n", data3, region2);
    bytes = clipboard_copy(fd, region2, (void *) data3, sizeof(data3));
    printf("Copied %d bytes to clipboard\n", bytes);

    sleep(1);

    printf("Pasting data from clipboard region %d\n", region2);
    bytes = clipboard_paste(fd, region2, (void *) str, sizeof(data3));
    printf("Pasted \"%s\", %d bytes\n", str, bytes);

    sleep(1);

    printf("Trying to paste data3 from clipboard region %d\n", 3);
    bytes = clipboard_paste(fd, 3, (void *) str, sizeof(data3));
    printf("Pasted \"%s\", %d bytes\n", str, bytes);

    exit(0);
}
