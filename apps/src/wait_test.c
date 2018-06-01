/* wait_test.c
 * tests if wait is working as intended */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

    clipboard_wait(clip, region1, (void *) str, sizeof(data3));

    printf("Copying \"%s\" to clipboard region %d\n", data2, region1);
    clipboard_copy(clip, region1, (void *) data2, sizeof(data2));
    
    clipboard_close(clip);
    
    exit(0);
}
