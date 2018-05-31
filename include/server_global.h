#ifndef SERVER_GLOBAL
#define SERVER_GLOBAL

#include <sys/types.h>
#include <pthread.h>

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
extern pthread_mutex_t sync_lock;
extern int local_server_fd;
extern int tcp_server_fd;
extern int connected_fd;

#endif
