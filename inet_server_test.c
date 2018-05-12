#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 3000
#define SERVER_BACKLOG 10

int server_fd;

void interrupt_f(int signum){
	printf("Terminating...\n");
	close(server_fd);
	exit(0);
}

int main(int argc, char **argv){
    struct sockaddr_in server_addr;
    int client_fd;

    signal(SIGINT, interrupt_f);

    // --- Socket setup ---
    // fd creation
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation error");
        exit(-1);
    }

    // address definition
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

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

    printf("Ready.\n");

    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
        perror("Error accepting client");
        exit(-1);
    }

    char buffer[100] = {0};
    int b = read(client_fd, (void *) buffer, 5);
    printf("%d bytes, msg: %s\n", b, buffer);

    b = read(client_fd, (void *) buffer, 19);
    printf("%d bytes, msg: %s\n", b, buffer);

    b = read(client_fd, (void *) buffer, 7);
    printf("%d bytes, msg: %s\n", b, buffer);

    close(server_fd);

	exit(0);
}
