#ifndef SERVER_GLOBAL
#define SERVER_GLOBAL

#include <sys/types.h>
#include <pthread.h>

#include <server_request.h>

// clipboard region
typedef struct cb {
    void *data;
    size_t data_size;
    pthread_rwlock_t rwlock;
    int waiting;
    pthread_cond_t cond;
    pthread_mutex_t cond_mut;
} cb;

// client list
typedef struct client {
    pthread_t thread_id;
    int fd;
    struct client *next;
} client;

typedef enum {LOCAL, REMOTE} client_type;

extern cb clipboard[10];
extern client *local_client_list;
extern client *remote_client_list;
extern enum connection_mode {SINGLE, CONNECTED} mode;
extern int connected_fd;
extern pthread_t parent_handler_thread, local_accept_thread, remote_accept_thread;
extern pthread_mutex_t sync_lock;
extern int local_server_fd;
extern int tcp_server_fd;

/* Free-first realloc
 * Assures that original pointer is freed first,
 * to reduce the chances of a malloc error;
 * Doesn't preserve old contents of ptr
 *
 * ptr == NULL  <=>  malloc
 * size == 0    <=>  free, return NULL
 *
 * Returns a pointer to the newly allocated memory
 * or NULL if size == 0 or an error happened
 */
void *ff_realloc(void *ptr, size_t size);

#endif
