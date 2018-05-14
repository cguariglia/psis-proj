#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <clipboard.h>

// clipboard region
typedef struct cb {
    void *data;
    size_t data_size;
    pthread_rwlock_t rwlock;
} cb;

// client list
typedef struct client {
    pthread_t thread_id;
    int fd;
    struct client *next;
} client;

cb clipboard[10];
client *client_list = NULL;

/* add a client to the list of connections
 * and place it at the end of the list
 *
 * returns 0 on success or -1 on error
 */
client *add_client(int client_fd){
    // find the last client on the list
    client *new_client = client_list;
    while (new_client != NULL) new_client = new_client->next;

    // allocate and initialize
    if ((new_client = malloc(sizeof(client))) == NULL) return NULL;
    new_client->fd = client_fd;
    new_client->next = NULL;

    return new_client;
}

void interrupt_f(int signum){
	printf("Terminating...\n");

    // free resources
    for (int i = 0; i < 10; i++) {
        if (clipboard[i].data != NULL) free(clipboard[i].data);

        // ainda podem existir locks; melhorar no final e acrescentar tipo variáveis de estado?
        pthread_rwlock_destroy(&clipboard[i].rwlock);
    }
	unlink(SERVER_ADDRESS);

	exit(0);
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
                    // to do
                    break;
                default:
                    break;
            }
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv){
    struct sockaddr_un server_addr;
    int server_fd, client_fd;

    // init clipboard
    for (int i = 0; i < 10; i++) {
        clipboard[i].data = NULL;
        clipboard[i].data_size = 0;
        clipboard[i].rwlock = (pthread_rwlock_t) PTHREAD_RWLOCK_INITIALIZER;
    }

    signal(SIGINT, interrupt_f);

    printf("Initiating setup...\n");

    // --- Socket setup ---
    // fd creation
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation error");
        exit(-1);
    }
    // address definition
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_ADDRESS);
    // binding
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("Socket address binding error");
        exit(-1);
    }
    // setup listen
    if (listen(server_fd, SERVER_BACKLOG) == -1) {
        perror("Listen setup error");
        exit(-1);
    }

    printf("Ready. Use ^C to stop.\n");

    while(1) {
        // accept pending client connection requests
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("Unable to accept client");
        } else {
            // add connected client to list
            client *new_client = add_client(client_fd);
            if (new_client == NULL) {
                perror("Unable to allocate memory for new client");
            } else {
                // setup a thread to manage communication with the new client
                if (pthread_create(&new_client->thread_id, NULL, &local_client_handler, (void *) &client_fd) != 0) {
                    perror("Unable to start a thread for the new client");
                }
            }
        }

        // do we need to do anything else?
    }
	exit(0);
}
