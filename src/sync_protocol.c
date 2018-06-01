#include <stdlib.h>
#include <unistd.h>
#include <server_global.h>

int8_t send_ask_parent(int region, size_t data_size, void *buffer){
    request req = {ASK_PARENT, region, data_size};
    int8_t malloc_status = -1;  // 0 = success; defaults to -1 = error, to avoid verifying the read (if the read fails, use -1)

    pthread_mutex_lock(&sync_lock);

    // send request
    write(connected_fd, (void *) &req, sizeof(req));

    // check if peer has successfully allocated a buffer
    read(connected_fd, (void *) &malloc_status, sizeof(malloc_status));

    // send data
    if (malloc_status == 0) {
        write(connected_fd, buffer, data_size);
    }   // else do nothing; peer won't do anything either

    pthread_mutex_unlock(&sync_lock);

    return malloc_status;
}

/* receive and store data from connected_fd
 *
 * uses an external buffer before replacing the old data,
 * to prevent losing the old data on a malloc error
 * because it had already been freed
 *
 * buffer must be allocated outside this function
 */
int store_buffered(int fd, int region, size_t data_size, void *buffer){
    // receive data
    if (fd == connected_fd) pthread_mutex_lock(&sync_lock);
    size_t bytes = read(fd, buffer, data_size);
    if (fd == connected_fd) pthread_mutex_unlock(&sync_lock);

    // lock for writing
    pthread_rwlock_wrlock(&clipboard[region].rwlock);

    // free older data in region, if there is any
    if (clipboard[region].data != NULL) free(clipboard[region].data);

    // store new data
    clipboard[region].data = buffer;
    clipboard[region].data_size = bytes;
    clipboard[region].waiting = 0;

    // unlock
    pthread_rwlock_unlock(&clipboard[region].rwlock);

    return data_size;
}

void send_sync_children(int region, size_t data_size){
    // broadcast to children
    request req = {SYNC_CHILDREN, region, data_size};
    for (client *aux = remote_client_list; aux != NULL; aux = aux->next) {
        write(aux->fd, &req, sizeof(req));
        write(aux->fd, clipboard[region].data, clipboard[region].data_size);
    }

    return;
}

// send DESYNC request to parent
void send_desync_parent(int region){
    request req = {DESYNC_PARENT, region, clipboard[region].data_size};

    pthread_mutex_lock(&sync_lock);

    write(connected_fd, (void *) &req, sizeof(req));
    write(connected_fd, clipboard[region].data, clipboard[region].data_size);

    pthread_mutex_unlock(&sync_lock);

    return;
}

// equivalent to store_buffered, but doesn't use a buffer
// used in desyncs
void store_not_buffered(int fd, int region, size_t data_size){
    // lock for writing
    pthread_rwlock_wrlock(&clipboard[region].rwlock);

    // allocate memory for the pending data
    ff_realloc(clipboard[region].data, data_size);  // there should always be memory allocated when a desync happens

    // store new data
    if (fd == connected_fd) pthread_mutex_lock(&sync_lock);
    clipboard[region].data_size = read(fd, &clipboard[region].data, data_size);
    if (fd == connected_fd) pthread_mutex_unlock(&sync_lock);

    clipboard[region].waiting = 0;

    // unlock
    pthread_rwlock_unlock(&clipboard[region].rwlock);

    return;
}

// broadcast DESYNC request to children
void send_desync_children(int region){
    request req = {DESYNC_CHILDREN, region, clipboard[region].data_size};

    for (client *aux = remote_client_list; aux != NULL; aux = aux->next) {
        write(aux->fd, (void *) &req, sizeof(req));
        write(aux->fd, clipboard[region].data, clipboard[region].data_size);
    }

    return;
}
