#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include <clipboard.h>

void interrupt_f(int signum){
	printf("Terminating...\n");
	unlink(SERVER_ADDRESS);
	exit(0);
}

int main(){
	struct sockaddr_un server_addr;
    int server_fd, client_fd;
    void *cb[10] = {NULL};

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
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("Error accepting client");
            exit(-1);
            // leave exit(-1)? why not just skip an iteration?
        }
        request req;
        if (read(client_fd, (void *) &req, sizeof(req)) == sizeof(req)) {
            // TODO: what if the message was not read?
            ssize_t bytes;  // bytes of data stored (COPY) or to send (PASTE/WAIT)
            switch (req.type) {
                case COPY:
                    if (cb[req.region] != NULL) free(cb[req.region]);
                    cb[req.region] = malloc(req.count);
                    bytes = read(client_fd, cb[req.region], req.count

                    ;/ AINDA NA' 'CABEI

                    break;
                case PASTE:
                    // to do

                    //write(client_fd, cb[req.region], sizeof(cb[req.region]));

                    break;
                case WAIT:
                    // to do
                    break;
                default:
                    break;
            }
        }
    }
	exit(0);
}
