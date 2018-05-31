#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>

#include <server_global.h>
#include <server_threads.h>

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
    if (pthread_create(&new_client->thread_id, NULL, type ? &remote_client_handler : &local_client_handler, (void *) &client_fd) != 0) {
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
            printf("[%s] Unable to accept client", type_str[ct]);
        } else {
            // add connected client to list
            if (add_client(client_fd, ct) == -1) {
                printf("[%s] Unable to add client", type_str[ct]);
            }
        }
    }

    pthread_exit(NULL);
}

void *local_client_handler(void *fd){
    int client_fd = *(int *) fd;
    request req;

    printf("[DEBUG] accepted client with fd %d\n", client_fd);

    while(1) {
        if (read(client_fd, (void *) &req, sizeof(req)) == sizeof(req)) {

            // TODO: what if the message was not read?

            ssize_t bytes;  // bytes of data stored (COPY) or to send (PASTE/WAIT)
            switch (req.type) {
                case COPY:
                {
                    void *buffer = malloc(req.data_size);
                    int8_t malloc_status = 0;   // -1 = error; defaults to 0 = success (most likely to happen)
                    if (buffer == NULL) {   // unable to allocate buffer
                        // report unsuccessful malloc
                        malloc_status = -1;
                        write(client_fd, (void *) &malloc_status, sizeof(malloc_status));   // receiver checks message integrity
                    } else {
                        // store requested region
                        int region = req.region;

                        // report successful malloc (initialized to 'success')
                        write(client_fd, (void *) &malloc_status, sizeof(malloc_status));   // receiver checks message integrity

                        // get data
                        bytes = read(client_fd, buffer, req.data_size);

                        // synchronize with clipboard network
                        if (mode) { // if in CONNECTED mode
                            req.type = ASK_PARENT;  // reutilize req variable; region and data_size are the same
                            write(connected_fd, (void *) &req, sizeof(req)); // receiver checks message integrity

                            malloc_status = -1;  // 0 = success; defaults to -1 = error, to avoid verifying the next read (if the read fails, use -1)
                            read(connected_fd, (void *) &malloc_status, sizeof(malloc_status));    // read a single byte

                            if (malloc_status == -1) {

            // IDFK PLS HALP

                            } else {    // malloc_status == 0
                                write(connected_fd, buffer, bytes);
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
                                    // reutilize req variable
                                    read(connected_fd, (void *) &req, sizeof(req)); // should work all the time, unless you're abusing the clipboard

                                    if ((ssize_t) req.data_size != bytes && realloc(buffer, req.data_size) != NULL) {
                                        // if the realloc succeeds, update the buffer size variable
                                        bytes = req.data_size;
                                    }
                                    // if the realloc fails, store only up to the old buffer size (buffer is left untouched)
                                    bytes = read(connected_fd, buffer, bytes);

                                    // lock for writing
                                    pthread_rwlock_wrlock(&clipboard[req.region].rwlock);

                                    // free older data in region, if there is any
                                    if (clipboard[req.region].data != NULL) free(clipboard[req.region].data);

                                    // store new data
                                    clipboard[req.region].data = buffer;
                                    clipboard[req.region].data_size = bytes;
                                    clipboard[req.region].waiting = 0;
                                    
                                    // unlock
                                    pthread_rwlock_unlock(&clipboard[req.region].rwlock);
                                } while (req.region != region);

                                // reply with stored data size in requested region
                                write(client_fd, (void *) &bytes, sizeof(bytes));

                            }
                        }
                        // SINGLE mode
                        // broadcast to all children (if there are any)
                        for (client *aux = remote_client_list; aux != NULL; aux = aux->next) {

                            // still got shtuffs to do

                        }

                    }
        printf("copy region %d: %s\n", req.region, (char *) clipboard[req.region].data);

                    break;
                }
                case WAIT:
                    pthread_rwlock_rdlock(&clipboard[req.region].rwlock);
                    clipboard[req.region].waiting = 1;
                    
                    while(clipboard[req.region].waiting == 1)
                        pthread_cond_wait(&clipboard[req.region].cond, &clipboard[req.region].cond_mut);
                    
                    pthread_rwlock_unlock(&clipboard[req.region].rwlock);
                case PASTE:
                    // compared requested data size with stored data size and choose the lowest
                    bytes = (clipboard[req.region].data_size < req.data_size) ? clipboard[req.region].data_size : req.data_size;

                    /* if no data is stored in the given region (data == NULL, data_size == 0),
                     * data_size value is sent regardless and but the client won't attempt to read data */
                    if (write(client_fd, (void *) &bytes, sizeof(bytes)) != sizeof(bytes)) break;
                    // lock for reading
                    pthread_rwlock_rdlock(&clipboard[req.region].rwlock);
                    if (clipboard[req.region].data != NULL) {
                        write(client_fd, clipboard[req.region].data, bytes);
                    }
                    // unlock
                    pthread_rwlock_unlock(&clipboard[req.region].rwlock);

                    /* write prettier code? ^ as in join the conditions and stuff */
        printf("paste region %d:\tbytes: %d\t clipboard[req.region].data_size: %d\tdata_size: %d\n", req.region, (int) bytes, (int) clipboard[req.region].data_size, (int) req.data_size);

                    break;
                case CLOSE:
                {
                    // find the client, remove it from the list and store it in aux
                    client *aux = local_client_list;
                    if (local_client_list->fd == client_fd) {   // client is 1st on the list (already stored in aux)
                        local_client_list = local_client_list->next;
                        free(aux);
                    } else {                                    // client is in the middle or the end of the list
                        while (aux->next->fd != client_fd) aux = aux->next;
                        client *aux_free = aux->next;
                        aux->next = aux->next->next;
                        free(aux_free);
                    }
                    close(client_fd);
                    pthread_exit(NULL);
                    break;
                }
                default:
                    break;
            }
        }
    }
}

/*void ask_parent(request req){

}

void sync_children(int client_fd, request recvd_req){
    void *buffer = NULL;
    unsigned char status;

    if ((buffer = malloc(recvd_req.data_size) == NULL) {
        status = 0; // error
        if (write(client_fd, &status, sizeof(status)) == sizeof(status)) {
            request desync_req = {ASK_PARENT
            ask_parent(
        }
    }

}*/

void *remote_client_handler(void *fd){
    int client_fd = *(int *) fd;
    request recvd_req;
    void *buffer = NULL;
    int8_t status;

    while(1) {
        if (read(client_fd, (void *) &recvd_req, sizeof(recvd_req)) == sizeof(recvd_req)) {

            pthread_mutex_lock(&sync_lock);

            switch (recvd_req.type) {
                case ASK_PARENT:

                    break;
                case SYNC_CHILDREN:
                    if ((buffer = malloc(recvd_req.data_size)) == NULL) {
                        status = 0; // error
                        if (write(client_fd, &status, sizeof(status)) == sizeof(status)) {
                            request desync_req = {ASK_PARENT, recvd_req.region, clipboard[recvd_req.region].data_size};
        /* error checking!*/write(connected_fd, &desync_req, sizeof(desync_req));

                        }
                    }
                    break;
                default:
                    break;
            }

            pthread_mutex_unlock(&sync_lock);
        }
    }
}
