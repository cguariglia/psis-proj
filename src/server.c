#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <clipboard.h>
#include <server_threads.h>

#define ERROR(A) {perror(A); exit(-1);}

// clipboard region
typedef struct cb {
    void *data;
    size_t data_size;
    pthread_rwlock_t rwlock;
} cb;

cb clipboard[10];
client *local_client_list = NULL;
client *remote_client_list = NULL;
enum {SINGLE, CONNECTED} mode;
int running = 1;

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

    // init clipboard
    for (int i = 0; i < 10; i++) {
        clipboard[i].data = NULL;
        clipboard[i].data_size = 0;
        clipboard[i].rwlock = (pthread_rwlock_t) PTHREAD_RWLOCK_INITIALIZER;
    }

    signal(SIGINT, interrupt_f);

    // create local socket
    struct sockaddr_un local_server_addr;
    int local_server_fd;

    // socket creation
    if ((local_server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) ERROR("[UNIX] Socket creation error")

    // address definition
    local_server_addr.sun_family = AF_UNIX;
    strcpy(local_server_addr.sun_path, SERVER_ADDRESS);

    // binding
    if (bind(local_server_fd, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)) == -1) {
        unlink(local_server_fd);
        ERROR("[UNIX] Socket address binding error");
    }

    // setup listen
    if (listen(local_server_fd, SERVER_BACKLOG) == -1) {
        unlink(local_server_fd);
        ERROR("[UNIX] Listen setup error");
    }


    // create TCP socket
    struct sockaddr_in tcp_server_addr;
    socklen_t tcp_server_addr_len = sizeof(tcp_server_addr);
    int tcp_server_fd;

    // socket creation
    if ((tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        unlink(local_server_fd);
        ERROR("[TCP] Socket creation error");
    }

    // address definition
    tcp_server_addr.sun_family = AF_INET;
    tcp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_server_addr.sin_port = 0;

    // binding
    if (bind(tcp_server_fd, (struct sockaddr *) &tcp_server_addr, tcp_server_addr_len) == -1) {
        unlink(local_server_fd);
        close(tcp_server_fd);
        ERROR("[TCP] Socket address binding error");
    }

    // setup listen
    if (listen(tcp_server_fd, SERVER_BACKLOG) == -1) {
        unlink(local_server_fd);
        close(tcp_server_fd);
        ERROR("[TCP] Listen setup error");
    }

    // print used port
    if (getsockname(tcp_server_fd, (struct sockaddr *) &tcp_server_addr, &tcp_server_addr_len) != 0) {
        unlink(local_server_fd);
        close(tcp_server_fd);
        ERROR("[TCP] Unable to retrieve socket port");
    }
    printf("Server connected to port %hu\n", ntohs(tcp_server_addr.sin_port));



	exit(0);
}
