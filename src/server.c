#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <clipboard.h>
#include <server_threads.h>

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

int is_ipv4(char *str){
    uint8_t buf;  // buffer required for scanf to count successful reads
    return (sscanf(str, "%hhu.%hhu.%hhu.%hhu", &buf, &buf, &buf, &buf) == 4);
}

int is_port(char *str){
    uint16_t buf;   // buffer
    return sscanf(str, "%hu", &buf);
}

int main(int argc, char **argv){

    if ((argc != 1 && argc != 4) ||
        (argc == 4 && (strcmp(argv[1], "-c") != 0 || !is_ipv4(argv[2]) || !is_port(argv[3]))) {
        printf("Usage (single mode):\t%s\nUsage (connected mode):\t%s -c [IPv4 address] [port #]\n");
        exit(-1);
    }

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
