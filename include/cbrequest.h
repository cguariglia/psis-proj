#include <sys/types.h>

// Messages exchanged between server and clients
typedef struct request {
    enum {COPY, PASTE, WAIT} type;  // request type
    int region;                     // region to copy to/paste from
    void *data;                     // pointer to data to copy to clipboard
    size_t data_size;               // amount of data (in bytes) to copy/paste
} request;
