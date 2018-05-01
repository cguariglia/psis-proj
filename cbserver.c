#include <sys/types.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"

void interrupt_f(int signum){
	printf("Terminating...\n");
	unlink(SERVER_ADDRESS);
	exit(0);
}

int main(){
	struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;
    int server_fd, client_fd;
    socklen_t client_addr_size;
    void *cb[10] = {0};

    printf("Initiating setup...\n");

    // --- Socket setup ---
    // fd creation
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket creation");
        exit(-1);
    }
    // address definition
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_ADDRESS);
    // binding
    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("socket bind");
        exit(-1);
    }
    // setup listen
    if (listen(server_fd, SERVER_BACKLOG) == -1) {
        perror("listen");
        exit(-1);
    }

    printf("Ready. Use ^C to stop.\n");

    while(1) {
        message msg;
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {    // in case shit happens, change NULL, NULL to (struct sockaddr *) &client_addr, &client_addr_size
            perror("client accept");
            exit(-1);
        }
        read(




    }









	exit(0);

}
