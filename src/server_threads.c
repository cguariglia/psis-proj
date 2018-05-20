
#include <string.h>
#include <sys/socket.h>

#include <server_threads.h>

extern int running;
extern client *local_client_list;
extern client *remote_client_list;

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

void *accept_clients(void *type){
    int client_fd;
    char type_str[5] = {0};
    client_type ct;

    if (!strcmp((char *) type, "LOCAL")) {
        strcpy(type_str, "UNIX");
        ct = LOCAL;
    } else if (!strcmp((char *) type, "REMOTE")) {
        strcpy(type_str, "TCP");
        ct = REMOTE;
    } else pthread_exit(NULL);

    while(1) {
        // accept pending client connection requests
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("[%s] Unable to accept client", type_str);
        } else {
            // add connected client to list
            if (add_client(client_fd, ct) == -1) {
                perror("[%s] Unable to add client", type_str);
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
                    // lock for writing
                    pthread_rwlock_wrlock(&clipboard[req.region].rwlock);
                    // replace older data in region, if there is any
                    if (clipboard[req.region].data != NULL) free(clipboard[req.region].data);
                    if ((clipboard[req.region].data = malloc(req.data_size)) == NULL) {
                        for (int i = 0; i < 10; i++) {
                            if (clipboard[i].data != NULL) free(clipboard[i].data);
                        }
    /* MEEEEMES */      printf("Malloc error! Fix this shit!\n");
                        exit(-1);
                    }
                    // store new data
                    clipboard[req.region].data_size = read(client_fd, clipboard[req.region].data, req.data_size);
                    // reply with stored data size
                    write(client_fd, (void *) &clipboard[req.region].data_size, sizeof(clipboard[req.region].data_size));
                    // unlock
                    pthread_rwlock_unlock(&clipboard[req.region].rwlock);

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

            // free stuff from client list

                    break;
                default:
                    break;
            }
        }
    }

    pthread_exit(NULL);
}
