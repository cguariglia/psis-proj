#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <clipboard.h>
#include <server_global.h>
#include <server_threads.h>

#define ERROR(A) {perror(A); exit(-1);}

void cleanup(int sig){
	printf("Terminating...\n");

    // terminate peer accept threads
    pthread_cancel(local_accept_thread);
    pthread_cancel(remote_accept_thread);

    // each thread closes the associated peer fd and removes itself from its peer list
    // terminate connection with local clients
    for (client *aux = local_client_list; aux != NULL; aux = aux->next) {
        pthread_cancel(aux->thread_id);
    }

    // terminate connection with remote clients
    for (client *aux = remote_client_list; aux != NULL; aux = aux->next) {
        pthread_cancel(aux->thread_id);
    }

    // terminate connection with parent clipboard
    if (mode) pthread_cancel(parent_handler_thread);

    // free resources
    for (int i = 0; i < 10; i++) {
        if (clipboard[i].data != NULL) free(clipboard[i].data);
        pthread_rwlock_destroy(&clipboard[i].rwlock);
        pthread_cond_destroy(&clipboard[i].cond);
        pthread_mutex_destroy(&clipboard[i].cond_mut);
    }

    pthread_mutex_destroy(&sync_lock);
	unlink("./CLIPBOARD_SOCKET");

	exit(0);
}

int is_ipv4(char *str){
    u_int8_t buf;  // buffer required for scanf to count successful reads
    return (sscanf(str, "%hhu.%hhu.%hhu.%hhu", &buf, &buf, &buf, &buf) == 4);
}

int is_port(char *str){
    u_int16_t buf;   // buffer
    return sscanf(str, "%hu", &buf);
}

u_int16_t atous(const char *str) {
    u_int16_t num;
    if (sscanf(str, "%hu", &num)) {
        return num;
    } else {
        return 0;
    }
}

int main(int argc, char **argv){
    // argument syntax check
    if ((argc != 1 && argc != 4) ||
        (argc == 4 && (strcmp(argv[1], "-c") != 0 || !is_ipv4(argv[2]) || !is_port(argv[3])))) {
        printf("Usage (single mode):\t%s\nUsage (connected mode):\t%s -c [IPv4 address] [port #]\n", argv[0], argv[0]);
        exit(-1);
    }

    // arg syntax checking already done before, no need to repeat
    if (argc == 1) {
        mode = SINGLE;
        printf("Starting local clipboard server in SINGLE mode.\n");
    } else {
        mode = CONNECTED;
        printf("Starting local clipboard server in CONNECTED mode with parent clipboard @ %s:%s\n", argv[2], argv[3]);
    }

    // init clipboard
    for (int i = 0; i < 10; i++) {
        clipboard[i].data = NULL;
        clipboard[i].data_size = 0;
        clipboard[i].rwlock = (pthread_rwlock_t) PTHREAD_RWLOCK_INITIALIZER;
        clipboard[i].cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
        clipboard[i].cond_mut = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    }

    //signal(SIGINT, interrupt_f);

    // create local socket
    struct sockaddr_un local_server_addr;

    // socket creation
    if ((local_server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) ERROR("[UNIX] Socket creation error")

    // address definition
    local_server_addr.sun_family = AF_UNIX;
    strcpy(local_server_addr.sun_path, "./CLIPBOARD_SOCKET");

    // binding
    if (bind(local_server_fd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)) == -1) {
        unlink("./CLIPBOARD_SOCKET");
        ERROR("[UNIX] Socket address binding error");
    }

    // setup listen
    if (listen(local_server_fd, SERVER_BACKLOG) == -1) {
        unlink("./CLIPBOARD_SOCKET");
        ERROR("[UNIX] Listen setup error");
    }

    // create TCP socket
    struct sockaddr_in tcp_server_addr;
    socklen_t tcp_server_addr_len = sizeof(tcp_server_addr);

    // socket creation
    if ((tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        unlink("./CLIPBOARD_SOCKET");
        ERROR("[TCP] Socket creation error");
    }

    // address definition
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_server_addr.sin_port = 0;

    // binding
    if (bind(tcp_server_fd, (struct sockaddr *) &tcp_server_addr, tcp_server_addr_len) == -1) {
        unlink("./CLIPBOARD_SOCKET");
        close(tcp_server_fd);
        ERROR("[TCP] Socket address binding error");
    }

    // setup listen
    if (listen(tcp_server_fd, SERVER_BACKLOG) == -1) {
        unlink("./CLIPBOARD_SOCKET");
        close(tcp_server_fd);
        ERROR("[TCP] Listen setup error");
    }

    // print used port
    if (getsockname(tcp_server_fd, (struct sockaddr *) &tcp_server_addr, &tcp_server_addr_len) != 0) {
        unlink("./CLIPBOARD_SOCKET");
        close(tcp_server_fd);
        ERROR("[TCP] Unable to retrieve socket port");
    }
    printf("Server is using port %hu\n", ntohs(tcp_server_addr.sin_port));

    // connect to parent if running in CONNECTED mode
    if (mode) {
        struct sockaddr_in parent_addr;

        // fd creation (stored globally)
        if ((connected_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("[TCP] [CONNECTED mode] Socket creation error");
            printf("Clipboard will run in SINGLE mode.\n");
            mode = SINGLE;
        }

        // address definition
        parent_addr.sin_family = AF_INET;
        parent_addr.sin_port = htons(atous(argv[3]));
        inet_aton(argv[2], &parent_addr.sin_addr);

        // connection to server
        if (connect(connected_fd, (struct sockaddr *) &parent_addr, sizeof(parent_addr)) == -1) {
            close(connected_fd);
            perror("[TCP] [CONNECTED mode] Unable to connect to parent");
            printf("Clipboard will run in SINGLE mode.\n");
            mode = SINGLE;
        }
    }

    // block all signals for threads; let the main thread handle them
    sigset_t sig_mask;
    sigfillset(&sig_mask);
    pthread_sigmask(SIG_BLOCK, &sig_mask, NULL);

    // start threads
    client_type ct = LOCAL;
    pthread_create(&local_accept_thread, NULL, &accept_clients, (void *) &ct);
    if (mode) pthread_create(&parent_handler_thread, NULL, &remote_peer_handler, (void *) &connected_fd);
    ct = REMOTE;
    pthread_create(&remote_accept_thread, NULL, &accept_clients, (void *) &ct);

    // unblock signals
    pthread_sigmask(SIG_UNBLOCK, &sig_mask, NULL);

    // handle signals in main thread
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    // SIGKILL and SIGSTOP can't be caught

    printf("Type 'q' to stop.\n");
    char c;
    do {
        c = getchar();
    } while (c != 'q' && c != 'Q');

    cleanup(0);

	exit(0);
}
