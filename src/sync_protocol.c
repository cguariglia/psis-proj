#include <stdlib.h>
#include <unistd.h>
#include <server_global.h>

void send_ask_parent(int region, size_t data_size, void *buffer){
    // send request
    request req = {ASK_PARENT, region, data_size};
    write(connected_fd, (void *) &req, sizeof(req));

    // check if peer has successfully allocated a buffer
    int8_t malloc_status = -1;  // 0 = success; defaults to -1 = error, to avoid verifying the next read (if the read fails, use -1)
    read(connected_fd, (void *) &malloc_status, sizeof(malloc_status));

    // send data
    if (malloc_status == 0) {
        write(connected_fd, buffer, data_size);
    }   // else do nothing; peer won't do anything either

    return;
}

int recv_sync_children(int region, size_t data_size, void *buffer){
    // receive data
    data_size = read(connected_fd, buffer, data_size);

    // lock for writing
    pthread_rwlock_wrlock(&clipboard[region].rwlock);

    // free older data in region, if there is any
    if (clipboard[region].data != NULL) free(clipboard[region].data);

    // store new data
    clipboard[region].data = buffer;
    clipboard[region].data_size = data_size;
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

    write(connected_fd, (void *) &req, sizeof(req));
    write(connected_fd, clipboard[region].data, clipboard[region].data_size);

    return;
}

// equivalent to recv_sync_children, but doesn't use a buffer
void recv_desync_children(int region, size_t data_size){
    // lock for writing
    pthread_rwlock_wrlock(&clipboard[region].rwlock);

    // allocate memory for the pending data
    ff_realloc(clipboard[region].data, data_size);  // there should always be memory allocated when a desync happens

    // store new data
    clipboard[region].data_size = read(connected_fd, &clipboard[region].data, data_size);
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
