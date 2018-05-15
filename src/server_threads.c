

#include <server_threads.h>

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
                    // to do
                    break;
                default:
                    break;
            }
        }
    }

    pthread_exit(NULL);
}
