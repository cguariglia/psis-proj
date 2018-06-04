#ifndef SERVER_GLOBAL
#define SERVER_GLOBAL

#include <sys/types.h>
#include <pthread.h>

#include <server_request.h>
void mutex_init(pthread_mutex_t *mutex);
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

#include <string.h>

#define cb_perror(err) do { \
	char __buf__[1024]; \
	strerror_r(err, __buf__, sizeof(__buf__)); \
	fprintf(stderr, "\x1B[31m%s:%d %s(), errno = %d %s\n\x1B[0m", __FILE__, __LINE__-1, __func__, err, __buf__); \
} while(0);

#define cb_eperror(err) do { \
	cb_perror(err); \
	exit(1); \
} while(0);

#define MUTEX_LOCK(mutex) do { \
    fprintf(stderr, "%s:%d %s(), try lock mutex\n", __FILE__, __LINE__, __func__); \
    int __test__ = pthread_mutex_lock(mutex); \
    if (__test__ != 0) cb_eperror(__test__); \
    fprintf(stderr, "%s:%d %s(), got lock mutex\n", __FILE__, __LINE__, __func__); \
} while(0);
#define MUTEX_UNLOCK(mutex) do { \
    fprintf(stderr, "%s:%d %s(), try unlock mutex\n", __FILE__, __LINE__, __func__); \
    int __test__ = pthread_mutex_unlock(mutex); \
    if (__test__ != 0) cb_eperror(__test__); \
    fprintf(stderr, "%s:%d %s(), got unlock mutex\n", __FILE__, __LINE__, __func__); \
} while(0);

#define RWLOCK_RDLOCK(rwlock) do { \
    fprintf(stderr, "%s:%d %s(), try read-lock rwlock\n", __FILE__, __LINE__, __func__); \
    int __test__ = pthread_rwlock_rdlock(rwlock); \
    if (__test__ != 0) cb_eperror(__test__); \
    fprintf(stderr, "%s:%d %s(), got read-lock rwlock\n", __FILE__, __LINE__, __func__); \
} while(0);
#define RWLOCK_WRLOCK(rwlock) do { \
    fprintf(stderr, "%s:%d %s(), try write-lock rwlock\n", __FILE__, __LINE__, __func__); \
    int __test__ = pthread_rwlock_wrlock(rwlock); \
    if (__test__ != 0) cb_eperror(__test__); \
    fprintf(stderr, "%s:%d %s(), got write-lock rwlock\n", __FILE__, __LINE__, __func__); \
} while(0);
#define RWLOCK_UNLOCK(rwlock) do { \
    fprintf(stderr, "%s:%d %s(), try unlock rwlock\n", __FILE__, __LINE__, __func__); \
    int __test__ = pthread_rwlock_unlock(rwlock); \
    if (__test__ != 0) cb_eperror(__test__); \
    fprintf(stderr, "%s:%d %s(), got unlock rwlock\n", __FILE__, __LINE__, __func__); \
} while(0);

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
