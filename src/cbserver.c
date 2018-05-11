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
    if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
        perror("Error accepting client");
        exit(-1);
        // leave exit(-1)? why not just skip an iteration?
    }
    printf("accepted client with fd %d\n", client_fd);
    while(1) {
        request req;
        if (read(client_fd, (void *) &req, sizeof(req)) == sizeof(req)) {
            // TODO: what if the message was not read?
            ssize_t bytes;  // bytes of data stored (COPY) or to send (PASTE/WAIT)
            switch (req.type) {
                case COPY:
                    if (cb[req.region] != NULL) free(cb[req.region]);
                    cb[req.region] = malloc(req.data_size);
                    bytes = read(client_fd, cb[req.region], req.data_size);
                    write(client_fd, (void *) &bytes, sizeof(bytes));

        printf("copy region %d: %s\n", req.region, (char *) cb[req.region]);

                    break;
                case PASTE:
                    bytes = (sizeof(cb[req.region]) < req.data_size) ? sizeof(cb[req.region]) : req.data_size;
                    if (write(client_fd, (void *) &bytes, sizeof(bytes)) != sizeof(bytes)) break;
                    write(client_fd, cb[req.region], bytes);

        printf("paste region %d: %s\n", req.region, (char *) cb[req.region]);

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
