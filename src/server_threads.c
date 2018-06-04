#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>

#include <server_threads.h>
#include <sync_protocol.h>

void close_local_connection(void *fd) {
    // only run if the list isn't empty (shouldn't ever happen)
    if (local_client_list == NULL) return;

    int client_fd = *(int *) fd;

printf("current client list:"); client *auxi;
for (auxi = local_client_list; auxi->next != NULL; auxi = auxi->next) printf(" %d",auxi->fd);
printf(" %d\n- closing connection from client with fd = %d\tvoid *fd == %d\n\n", auxi->fd, client_fd, *(int *) fd);

    // find the client, remove it from the list and store it in aux
    client *aux = local_client_list;

    // client is first on the list
    if (local_client_list->fd == client_fd) {
        local_client_list = local_client_list->next;
        free(aux);
    } else {
        aux = local_client_list->next;          // second element in the list
        client *prev_aux = local_client_list;    // element before aux

//printf("\nNot the first element\n\naux->fd = %d\naux->next == NULL? %s\n\n", aux->fd, (aux->next==NULL)?"\t=== YES ===\t":"no");
int i=0;

        // find the client's element
        while (aux->fd != client_fd && aux->next != NULL) { // client_fd will always exist in the list
printf("\n# iterations: i=%d\naux->fd = %d\naux->next == NULL? %s\n\n", ++i, aux->fd, (aux->next==NULL)?"yes":"no");
            prev_aux = aux;
            aux = aux->next;
        }

        // reconnect the list and remove the element
        prev_aux->next = aux->next;
        free(aux);
    }

    close(client_fd);
}

void close_remote_connection(void *fd) {
    // only run if the list isn't empty (shouldn't ever happen)
    if (remote_client_list == NULL) return;

    int client_fd = *(int *) fd;

    // find the client, remove it from the list and store it in aux
    client *aux = remote_client_list;

    // client is first on the list
    if (remote_client_list->fd == client_fd) {
        remote_client_list = remote_client_list->next;
        free(aux);
    } else {
        aux = remote_client_list->next;         // second element in the list
        client *prev_aux = remote_client_list;  // element before aux

        // find the client's element
        while (aux->fd != client_fd && aux->next != NULL) { // client_fd will always exist in the list
            prev_aux = aux;
            aux = aux->next;
        }

        // reconnect the list and remove the element
        prev_aux->next = aux->next;
        free(aux);
    }

    close(client_fd);
}

/* add a client to the list of connections
 * and place it at the end of the list
 *
 * returns 0 on success or -1 on error
 */
int add_client(int client_fd, client_type type){
    // allocate and initialize
    int *new_fd = malloc(sizeof(int)); // so the stack won't eat this before it gets used
    memcpy(new_fd, &client_fd, sizeof(int));
    
    client *new_client = malloc(sizeof(client));
    if (new_client == NULL) {
        close(client_fd);
        return -1;
    }
    new_client->fd = client_fd;
    new_client->next = NULL;

    // list is empty before adding the new client
    if ((type ? remote_client_list : local_client_list) == NULL) {
        type ? (remote_client_list = new_client) : (local_client_list = new_client);
    } else {
        // find the last client on the list
        client *aux = type ? remote_client_list : local_client_list;
        while (aux->next != NULL) aux = aux->next;
        aux->next = new_client;
    }

printf("add_client client_fd == %d!\n", client_fd);

    // setup a thread to manage communication with the new client
    if (pthread_create(&new_client->thread_id, NULL, type ? &remote_peer_handler : &local_client_handler, (void *) new_fd) != 0) {
        type ? close_remote_connection((void *) &client_fd) : close_local_connection((void *) new_fd);
        return -1;
    }

printf("added client with fd = %d\ncurrent client list:\t", new_client->fd);
for (client *aux = local_client_list; aux != NULL; aux = aux->next) printf("%d-", aux->fd);
putchar('\n');

    return 0;
}

// void *type = client_type *type
void *accept_clients(void *type){
    int client_fd;
    char *type_str[] = {"UNIX", "TCP"};
    client_type ct = *(client_type *)type;

    if (ct != LOCAL && ct != REMOTE) pthread_exit(NULL);

    while(1) {
        // accept pending client connection requests
        if ((client_fd = accept(ct ? tcp_server_fd : local_server_fd, NULL, NULL)) == -1) {
            printf("[%s] Unable to accept client: ", type_str[ct]);
            fflush(stdout); // previous printf doesn't end with '\n' so it may not be printed
            perror(NULL);
        } else {
            // add connected client to list
            printf("who?\n");
            if (add_client(client_fd, ct) == -1) {
                printf("[%s] Unable to add client: ", type_str[ct]);
                fflush(stdout); // previous printf doesn't end with '\n' so it may not be printed
                perror(NULL);
            }
        }
    }

    pthread_exit(NULL);
}

void *local_client_handler(void *fd){
    int client_fd = *(int *) fd;
    request req;
    ssize_t bytes;  // bytes of data stored (COPY) or to send (PASTE/WAIT)
    ssize_t read_status;

    printf("[DEBUG] accepted client with fd %d\n", client_fd);
    
    pthread_cleanup_push(close_local_connection, (void *) &client_fd);
    
    do {
        read_status = read(client_fd, (void *) &req, sizeof(req));
        printf("first: req.region: %d | req.type: %d | req.data_size: %d\n", req.region, req.type, req.data_size);
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
                            printf("bytes: %d\n", bytes);
                            if (send_ask_parent(req.region, bytes, buffer) == -1) {
                                bytes = 0;  // set bytes to 0 to report to the API that an error happened
                                    printf("error\n");

                            } else {
                                // send data
                                pthread_mutex_lock(&sync_lock);
                                write(connected_fd, buffer, bytes);
                                pthread_mutex_unlock(&sync_lock);

                                                            printf("annyong2\n");

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
                                                                printf("ansdasdsanyong\n");
                                    printf("req.region: %d | req.type: %d\n", req.region, req.type);

                                    switch (req.type) {
                                        case DESYNC_CHILDREN:   // malloc error in the network; store directly in the clipboard, without buffering
                                        printf("desyn\n");
                                            store_not_buffered(connected_fd, req.region, req.data_size);
                                printf("annyongdsadsadsadsadsadsadsa\n");

                                            // set bytes to 0 to report to the API that an error happened
                                            bytes = 0;

                                            send_desync_children(req.region);

                                            break;
                                        case SYNC_CHILDREN:
                                        printf("sync\n");
                                            if ((ssize_t) req.data_size != bytes && ff_realloc(buffer, req.data_size) != NULL) {
                                                // if the realloc succeeds, update the buffer size variable
                                                bytes = req.data_size;
                                            }
                                            // if the realloc fails, store only up to the old buffer size (buffer is left untouched)
                                                                        printf("kill me\n");

                                            bytes = store_buffered(connected_fd, req.region, bytes, buffer);
                                            send_sync_children(req.region, bytes);

                                            break;
                                        default:
                                            break;
                                    }
                                } while (req.region != region);
                            }
                        } else {    // SINGLE mode
                            bytes = store_buffered(client_fd, req.region, req.data_size, buffer);
                            send_sync_children(req.region, bytes);
                        }

                        // reply with stored data size in requested region
                        write(client_fd, (void *) &bytes, sizeof(bytes));
                    }
                    break;
                }
                case WAIT:
                    pthread_mutex_lock(&clipboard[req.region].cond_mut);
                    clipboard[req.region].waiting = 1;

                    while(clipboard[req.region].waiting)
                        pthread_cond_wait(&clipboard[req.region].cond, &clipboard[req.region].cond_mut);

                    pthread_mutex_unlock(&clipboard[req.region].cond_mut);
                    // this fallthrough is on purpose because wait is basically a paste
                case PASTE:
                    // if no data is stored in the given region reply with 0
                    if (clipboard[req.region].data_size == 0 || clipboard[req.region].data == NULL) {
                        bytes = 0;
                        write(client_fd, (void *) &bytes, sizeof(bytes));
                        break;
                    }
                                        
                    // use stored data size if it is lower than the requested data size
                    bytes = (clipboard[req.region].data_size < req.data_size) ? clipboard[req.region].data_size : req.data_size;

                    write(client_fd, (void *) &bytes, sizeof(bytes));

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

    void (*cleanup_routine)(void *) = (peer_fd == connected_fd) ? disconnect_parent : close_remote_connection;

    pthread_cleanup_push(cleanup_routine, (peer_fd == connected_fd) ? NULL : (void *) &peer_fd);

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
                        malloc_status = 0;
                        if (peer_fd == connected_fd) pthread_mutex_lock(&sync_lock);
                        write(peer_fd, &malloc_status, sizeof(malloc_status));
                        if (peer_fd == connected_fd) pthread_mutex_unlock(&sync_lock);
                        
                        if (mode) { // CONNECTED mode
                            // receive incoming data
                            if (peer_fd == connected_fd) pthread_mutex_lock(&sync_lock);
                            bytes = read(peer_fd, buffer, recvd_req.data_size);
                            if (peer_fd == connected_fd) pthread_mutex_lock(&sync_lock);

                            if (send_ask_parent(recvd_req.region, bytes, buffer) == -1)
                                send_desync_parent(recvd_req.region);
                        } else {    // SINGLE mode
                            printf("args fd %d region %d data_size %d buf %s buf %p\n", peer_fd, recvd_req.region, recvd_req.data_size, (char *) buffer, buffer);
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
