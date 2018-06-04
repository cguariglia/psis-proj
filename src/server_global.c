#include <stdio.h>
#include <stdlib.h>
#include <server_global.h>


void mutex_init(pthread_mutex_t *mutex)
{
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);
    pthread_mutex_init(mutex, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
}

cb clipboard[10];
client *local_client_list = NULL;
client *remote_client_list = NULL;
enum connection_mode mode;
int connected_fd;   // socket used to communicate with parent in CONNECTED mode
pthread_t parent_handler_thread, local_accept_thread, remote_accept_thread;
pthread_mutex_t sync_lock;
int local_server_fd;
int tcp_server_fd;

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
void *ff_realloc(void *ptr, size_t size){
    if (ptr != NULL || size == 0) free(ptr);
    if (size == 0) return NULL;
    ptr = malloc(size);
    return ptr;
}
