
#include <string.h>
#include <sys/socket.h>

#include <server_threads.h>

extern cb clipboard[10];
extern client *local_client_list;
extern client *remote_client_list;
extern enum {SINGLE, CONNECTED} mode;
extern pthread_mutex_t sync_lock;

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
    char **type_str = {"UNIX", "TCP"};
    client_type ct = *(client_type *)type;

    if (ct != LOCAL || ct != REMOTE) pthread_exit(NULL);

    while(1) {
        // accept pending client connection requests
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("[%s] Unable to accept client", type_str[ct]);
        } else {
            // add connected client to list
            if (add_client(client_fd, ct) == -1) {
                perror("[%s] Unable to add client", type_str[ct]);
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
                    void *buffer = malloc(req.data_size);
                    if (buffer == NULL) {   // unable to allocate buffer
                        // empty socket buffer
/* DOES THIS EVEN WORK?! */read(client_fd, NULL, req.data_size);   // PLS DO NOT FUCK MY PC UP
                        // reply with an error
                        const size_t malloc_err = 0;    // pls dont fuck up
                        write(client_fd, (void *) &malloc_err, sizeof(malloc_err));
                    } else {
                        // synchronize with clipboard network
                        if (mode) { // if in CONNECTED mode




                    }

    /* ok, i gotta write some shit down
     *
     * in a network, you cant have any mutexes, so everything has to go through the main parent so it decides what to sync
     * but locally, there are rwlocks, so you can just write to other places using the locks to prevent street race conditions
     * so, if my client handler wants to synchronize, it can just lock the access to connected_fd and write to it, right?
     *                             (with its children)           (using a standard mutex of course)
     */



                    /*

                    // lock for writing
                    pthread_rwlock_wrlock(&clipboard[req.region].rwlock);
                    // replace older data in region, if there is any
                    if (clipboard[req.region].data != NULL) free(clipboard[req.region].data);
                    if ((clipboard[req.region].data = malloc(req.data_size)) == NULL) {

                    }
                    // store new data
                    clipboard[req.region].data_size = read(client_fd, clipboard[req.region].data, req.data_size);

                    // reply with stored data size
                    write(client_fd, (void *) &clipboard[req.region].data_size, sizeof(clipboard[req.region].data_size));

                    // unlock
                    pthread_rwlock_unlock(&clipboard[req.region].rwlock);

                    */

        printf("copy region %d: %s\n", req.region, (char *) clipboard[req.region].data);

                    break;
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
                case WAIT:

                    // to do

                    break;
                case CLOSE:
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
    unsigned char status;

    while(1) {
        if (read(client_fd, (void *) &recvd_req, sizeof(recvd_req)) == sizeof(recvd_req)) {
            pthread_mutex_lock(&sync_lock);
            switch (recvd_req.type) {
                case ASK_PARENT:

                    break;
                case SYNC_CHILDREN;
                    if ((buffer = malloc(recvd_req.data_size) == NULL) {
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
