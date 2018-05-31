#ifndef SERVER_REQUEST
#define SERVER_REQUEST

#include <sys/types.h>

// Messages exchanged between server and clients
typedef struct request {
    enum {COPY, PASTE, WAIT, CLOSE, ASK_PARENT, SYNC_CHILDREN} type;   // request type
    int region;         // region to copy to/paste from
    size_t data_size;   // amount of data (in bytes) to copy/paste
} request;

#endif
