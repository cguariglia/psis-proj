#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>

#define SERVER_ADDRESS "./cbserver"    // MUDAR
#define SERVER_BACKLOG 5

// Messages exchanged between server and clients
typedef struct request {
    enum {COPY, PASTE, WAIT} type;  // request type
    int region;                     // region to copy to/paste from
    void *data;                     // data copied to clipboard
} request; // OY M8 confirmar com stores se se mantém aqui, pq cbserver.c também precisa
// .h extra só com isto? ^

/*
 * clipboard_connect:   Connects to a clipboard
 *
 * char *clipboard_dir: directory where the clipboard is launched
 *
 * Returns:             clipboard id on success
 *                      -1 on error (local clipboard unaccessable)
 */
int clipboard_connect(char *clipboard_dir);

/*
 * clipboard_copy:      Copies data pointed to by buf
 *                      to a region in the local clipboard
 *
 * int clipboard_id:    clipboard id
 * int region:          region [0-9]
 * void *buf:           pointer to data to be copied to the clipboard
 * size_t count:        length of data pointed by buf
 *
 * Returns:             positive integer = nr. of bytes copied
 *                      0 on error
 */
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);

/*
 * clipboard_paste:     Copies data from a region in the local clipboard and
 *                      stores it in the memory pointed to by buf, up to a length of count
 *
 * int clipboard_id:    clipboard id
 * int region:          region [0-9]
 * void *buf:           pointer to data to be copied to the clipboard
 * size_t count:        length of data pointed by buf
 *
 * Returns:             positive integer = nr. of bytes copied
 *                      0 on error
 */
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);

/*
 * clipboard_wait:      Wait until new data is copied to a region and then
 *                      copy that data to the memory pointed to by buf, up to a length of count
 *
 * int clipboard_id:    clipboard id
 * int region:    		region [0-9]
 * void *buf:           pointer to data to be copied to the clipboard
 * size_t count:        length of data pointed by buf
 *
 * Returns:             positive integer = nr. of bytes copied
 *                      0 on error
 */
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);

/*
 * clipboard_close:		Close the connection between the application and the local clipboard
 *
 * int clipboard_id:    clipboard id
 *
 * Returns:             nothing
 */
void clipboard_close(int clipboard_id);
