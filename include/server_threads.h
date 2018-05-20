
#include <server_request.h>

// client list
typedef struct client {
    pthread_t thread_id;
    int fd;
    struct client *next;
} client;

typedef enum {LOCAL, REMOTE} client_type;

void *accept_clients(void *type);
void *local_client_handler(void *fd);
void *remote_client_handler(void *fd);
