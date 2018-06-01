#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>

#include <server_threads.h>
#include <server_global.h>
#include <sync_protocol.h>

// argument structure for the
// close_connection cleanup function
typedef struct arg_struct {
    int fd;
    client *list;
} arg_struct;

/* add a client to the list of connections
 * and place it at the end of the list
 *
 * returns 0 on success or -1 on error
 */
int add_client(int client_fd, client_type type){
    // find the last client on the list
    client *new_client = type ? remote_client_list : local_client_list;
    while (new_client != NULL) new_client = new_client->next;

    // allocate and initialize
    if ((new_client = malloc(sizeof(client))) == NULL) {
        close(client_fd);
        return -1;
    }
    new_client->fd = client_fd;
    new_client->next = NULL;

    // setup a thread to manage communication with the new client
    if (pthread_create(&new_client->thread_id, NULL, type ? &remote_peer_handler : &local_client_handler, (void *) &client_fd) != 0) {
        free(new_client);
        close(client_fd);
        return -1;
    }

    return 0;
}

// void *type = client_type *type
void *accept_clients(void *type){
    int client_fd;
    char *type_str[] = {"UNIX", "TCP"};
    client_type ct = *(client_type *)type;

    if (ct != LOCAL || ct != REMOTE) pthread_exit(NULL);

    while(1) {
        // accept pending client connection requests
        if ((client_fd = accept(ct ? tcp_server_fd : local_server_fd, NULL, NULL)) == -1) {
            printf("[%s] Unable to accept client: ", type_str[ct]);
            fflush(stdout); // previous printf doesn't end with '\n' so it may not be printed
            perror(NULL);
        } else {
            // add connected client to list
            if (add_client(client_fd, ct) == -1) {
                printf("[%s] Unable to add client: ", type_str[ct]);
                fflush(stdout); // previous printf doesn't end with '\n' so it may not be printed
                perror(NULL);
            }
        }
    }

    pthread_exit(NULL);
}

void close_connection(void *args) {
    int client_fd = ((arg_struct *) args)->fd;
    client *client_list = ((arg_struct *) args)->list;

    // find the client, remove it from the list and store it in aux
    client *aux = client_list;

    if (client_list->fd == client_fd) {   // client is 1st on the list (already stored in aux)
        client_list = client_list->next;
        free(aux);
    } else {    // client is in the middle or the end of the list
        while (aux->next->fd != client_fd) aux = aux->next;
        client *aux_free = aux->next;
        aux->next = aux->next->next;
        free(aux_free);
    }

    close(client_fd);
}

void *local_client_handler(void *fd){
    int client_fd = *(int *) fd;
    request req;
    ssize_t bytes;  // bytes of data stored (COPY) or to send (PASTE/WAIT)
    ssize_t read_status;

    printf("[DEBUG] accepted client with fd %d\n", client_fd);

    arg_struct args = {client_fd, local_client_list};
    pthread_cleanup_push(close_connection, &args);

    do {
        read_status = read(client_fd, (void *) &req, sizeof(req));
        if (read_status == sizeof(req)) {
            switch (req.type) {
                case COPY:
                {
                    void *buffer = malloc(req.data_size);
                    int8_t malloc_status = 0;   // -1 = error; defaults to 0 = success (most likely to happen)
                    if (buffer == NULL) {   // unable to allocate buffer
                        // report unsuccessful malloc
                        malloc_status = -1;
                        write(client_fd, (void *) &malloc_status, sizeof(malloc_status));   // receiver checks message integrity
                    } else {                // buffer successfuly allocated
                        // store requested region
                        int region = req.region;

                        // report successful malloc (status initialized to 'success')
                        write(client_fd, (void *) &malloc_status, sizeof(malloc_status));   // receiver checks message integrity

                        // synchronize with clipboard network
                        if (mode) { // if in CONNECTED mode
                            // get data
                            bytes = read(client_fd, buffer, req.data_size);
                            if (send_ask_parent(req.region, bytes, buffer) == -1) {
                                bytes = 0;  // set bytes to 0 to report to the API that an error happened
                            } else {
                                // send data
                                pthread_mutex_lock(&sync_lock);
                                write(connected_fd, buffer, bytes);
                                pthread_mutex_unlock(&sync_lock);

                                // wait for parent's response

                                /* Error checking here is not possible because it would require
                                 * warning every child clipboard that a memory allocation error happened,
                                 * which would require "desync'ing", i.e. undoing the synchronization
                                 * by sending the original contents of the region to every clipboard.
                                 * The issue is that they would need to allocate memory to receive the response,
                                 * which would sprout more potential memory allocation errors that would have to
                                 * be resolved in the same way.
                                 * Therefor, considering that the odds of the memory being full are very low,
                                 * no error checking is done.
                                 */

                                /* message received may be for a different region than expected
                                 * but it still has to be processed, because it's removed
                                 * from the socket after it's read
                                 */
                                do {
                                    pthread_mutex_lock(&sync_lock);
                                    // reutilize req variable
                                    read(connected_fd, (void *) &req, sizeof(req)); // should work all the time, unless you're abusing the clipboard
                                    pthread_mutex_unlock(&sync_lock);

                                    switch (req.type) {
                                        case DESYNC_CHILDREN:   // malloc error in the network; store directly in the clipboard, without buffering
                                            store_not_buffered(connected_fd, req.region, req.data_size);

                                            // set bytes to 0 to report to the API that an error happened
                                            bytes = 0;

                                            send_desync_children(req.region);

                                            break;
                                        case SYNC_CHILDREN:
                                            if ((ssize_t) req.data_size != bytes && ff_realloc(buffer, req.data_size) != NULL) {
                                                // if the realloc succeeds, update the buffer size variable
                                                bytes = req.data_size;
                                            }
                                            // if the realloc fails, store only up to the old buffer size (buffer is left untouched)

                                            bytes = store_buffered(connected_fd, req.region, bytes, buffer);
                                            send_sync_children(req.region, bytes);

                                            break;
                                        default:
                                            break;
                                    }
                                } while (req.region != region);
                            }
                        } else {    // SINGLE mode
                            bytes = store_buffered(client_fd, req.region, bytes, buffer);
                            send_sync_children(req.region, bytes);
                        }

                        // reply with stored data size in requested region
                        write(client_fd, (void *) &bytes, sizeof(bytes));
                    }

        malloc_status ? printf("oops\n") : printf("copy region %d: %s\n", req.region, (char *) clipboard[req.region].data);

                    break;
                }
                case WAIT:
                    pthread_mutex_lock(&clipboard[req.region].cond_mut);
                    clipboard[req.region].waiting = 1;

                    while(clipboard[req.region].waiting)
                        pthread_cond_wait(&clipboard[req.region].cond, &clipboard[req.region].cond_mut);

                    pthread_mutex_unlock(&clipboard[req.region].cond_mut);
                case PASTE:
                    // if no data is stored in the given region reply with 0
                    if (clipboard[req.region].data_size == 0 || clipboard[req.region].data == NULL) {
                        bytes = 0;
                        write(client_fd, (void *) &bytes, sizeof(bytes));
                        break;
                    }

                    // use stored data size if it is lower than the requested data size
                    bytes = (clipboard[req.region].data_size < req.data_size) ? clipboard[req.region].data_size : req.data_size;

                    // lock for reading
                    pthread_rwlock_rdlock(&clipboard[req.region].rwlock);

                    // send clipboard data
                    write(client_fd, clipboard[req.region].data, bytes);

                    // unlock
                    pthread_rwlock_unlock(&clipboard[req.region].rwlock);

        printf("paste region %d:\tbytes: %d\t clipboard[req.region].data_size: %d\tdata_size: %d\n", req.region, (int) bytes, (int) clipboard[req.region].data_size, (int) req.data_size);

                    break;
                case CLOSE:
                    read_status = 0;    // só uma macaquice pa sair do loop
                    break;
                default:
                    break;
            }
        }
    } while (read_status > 0);

    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}

void disconnect_parent(void *noarg){
    mode = SINGLE;
    close(connected_fd);
}

void *remote_peer_handler(void *fd){
    int peer_fd = *(int *) fd;
    request recvd_req;
    ssize_t bytes, read_status;
    void *buffer = NULL;
    int8_t malloc_status;  // 0 = success, -1 = error

    void (*cleanup_routine)(void *) = (peer_fd == connected_fd) ? disconnect_parent : close_connection;
    arg_struct args = {peer_fd, remote_client_list};

    pthread_cleanup_push(cleanup_routine, (peer_fd == connected_fd) ? NULL : &args);

    do {
        read_status = read(peer_fd, (void *) &recvd_req, sizeof(recvd_req));
        if (read_status == sizeof(recvd_req)) {
            switch (recvd_req.type) {
                case ASK_PARENT:
                    if ((buffer = malloc(recvd_req.data_size)) == NULL) {
                        /* In case of memory allocation error, the clipboard network will
                         * attempt to revert the synchronization process by synchronizing the
                         * data that was previously in the requested region through a different
                         * request that doesn't require buffer memory to be allocated.
                         */
                        // send malloc failure message
                        // peer won't reply with the data if it detects an error
                        malloc_status = -1;
                        if (peer_fd == connected_fd) pthread_mutex_lock(&sync_lock);
                        write(peer_fd, &malloc_status, sizeof(malloc_status));
                        if (peer_fd == connected_fd) pthread_mutex_unlock(&sync_lock);

                        // attempt to undo synchronization
                        if (mode) { // CONNECTED
                            // ask parent for a DESYNC
                            send_desync_parent(recvd_req.region);
                        } else {    // SINGLE
                            // broadcast DESYNC to children
                            send_desync_children(recvd_req.region);
                        }
                    } else {
                        if (mode) { // CONNECTED mode
                            // receive incoming data
                            if (peer_fd == connected_fd) pthread_mutex_lock(&sync_lock);
                            bytes = read(peer_fd, buffer, recvd_req.data_size);
                            if (peer_fd == connected_fd) pthread_mutex_lock(&sync_lock);

                            if (send_ask_parent(recvd_req.region, bytes, buffer) == -1)
                                send_desync_parent(recvd_req.region);
                        } else {    // SINGLE mode
                            bytes = store_buffered(peer_fd, recvd_req.region, recvd_req.data_size, buffer);
                            send_sync_children(recvd_req.region, bytes);
                        }
                    }
                    break;
                case SYNC_CHILDREN:
                    if ((buffer = malloc(recvd_req.data_size)) == NULL) {
                        /* In case of memory allocation error, the clipboard network will
                         * attempt to revert the synchronization process by synchronizing the
                         * data that was previously in the requested region through a different
                         * request that doesn't require buffer memory to be allocated.
                         */

                        // SYNC_CHILDREN will always be received by children, i.e. servers in CONNECTED mode
                        send_desync_parent(recvd_req.region);
                    } else {
                        bytes = store_buffered(peer_fd, recvd_req.region, bytes, buffer);
                        send_sync_children(recvd_req.region, bytes);
                    }
                    break;
                case DESYNC_PARENT:
                    if (mode) { // CONNECTED
                        store_not_buffered(peer_fd, recvd_req.region, recvd_req.data_size);
                        send_desync_parent(recvd_req.region);
                    } else {    // SINGLE
                        store_not_buffered(peer_fd, recvd_req.region, recvd_req.data_size);
                        send_desync_children(recvd_req.region);
                    }
                    break;
                case DESYNC_CHILDREN:
                    store_not_buffered(peer_fd, recvd_req.region, recvd_req.data_size);
                    send_desync_children(recvd_req.region);
                    break;
                default:
                    break;
            }
        }
    } while (read_status > 0);

    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}
