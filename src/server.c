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

void interrupt_f(int signum){
	printf("Terminating...\n");

    // free resources
    for (int i = 0; i < 10; i++) {
        if (clipboard[i].data != NULL) free(clipboard[i].data);

        // ainda podem existir locks; melhorar no final e acrescentar tipo variáveis de estado?
        pthread_rwlock_destroy(&clipboard[i].rwlock);
    }
    pthread_mutex_destroy(&sync_lock);
	unlink(SERVER_ADDRESS);

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
    strcpy(local_server_addr.sun_path, SERVER_ADDRESS);

    // binding
    if (bind(local_server_fd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)) == -1) {
        unlink(SERVER_ADDRESS);
        ERROR("[UNIX] Socket address binding error");
    }

    // setup listen
    if (listen(local_server_fd, SERVER_BACKLOG) == -1) {
        unlink(SERVER_ADDRESS);
        ERROR("[UNIX] Listen setup error");
    }


    // create TCP socket
    struct sockaddr_in tcp_server_addr;
    socklen_t tcp_server_addr_len = sizeof(tcp_server_addr);
    
    // socket creation
    if ((tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        unlink(SERVER_ADDRESS);
        ERROR("[TCP] Socket creation error");
    }

    // address definition
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_server_addr.sin_port = 0;

    // binding
    if (bind(tcp_server_fd, (struct sockaddr *) &tcp_server_addr, tcp_server_addr_len) == -1) {
        unlink(SERVER_ADDRESS);
        close(tcp_server_fd);
        ERROR("[TCP] Socket address binding error");
    }

    // setup listen
    if (listen(tcp_server_fd, SERVER_BACKLOG) == -1) {
        unlink(SERVER_ADDRESS);
        close(tcp_server_fd);
        ERROR("[TCP] Listen setup error");
    }

    // print used port
    if (getsockname(tcp_server_fd, (struct sockaddr *) &tcp_server_addr, &tcp_server_addr_len) != 0) {
        unlink(SERVER_ADDRESS);
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
    pthread_t local_accept_thread, remote_accept_thread;
    client_type ct = LOCAL;
    pthread_create(&local_accept_thread, NULL, &accept_clients, (void *) &ct);
    ct = REMOTE;
    pthread_create(&remote_accept_thread, NULL, &accept_clients, (void *) &ct);

    // unblock signals
    pthread_sigmask(SIG_UNBLOCK, &sig_mask, NULL);

    // handle signals in main thread

	exit(0);
}
