/* clipboard_test.c
 * tests that the basic functions of the clipboard (copy, paste) work as intended */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <clipboard.h>

int main(){
    char data1[] = "string 1";
    char data2[] = "string 2";
    char data3[] = "string 3";
    int region1 = 2;
    int region2 = 5;
    char str[100];

    int clip = clipboard_connect(SERVER_ADDRESS);
    
    printf("Copying \"%s\" to clipboard region %d\n", data1, region1);
    clipboard_copy(clip, region1, (void *) data1, sizeof(data1));

    printf("Copying \"%s\" to clipboard region %d\n", data2, region2);
    clipboard_copy(clip, region2, (void *) data2, sizeof(data2));

    printf("Copying \"%s\" to clipboard region %d\n", data3, region2);
    clipboard_copy(clip, region2, (void *) data3, sizeof(data3));
    
    printf("Pasting data from clipboard region %d\n", region2);
    clipboard_paste(clip, region2, (void *) str, sizeof(data3));
    
    printf("Pasting data3 from clipboard region %d\n", 3);
    clipboard_paste(clip, 3, (void *) str, sizeof(data3));

    clipboard_close(clip);

    exit(0);
}
