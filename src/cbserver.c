#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>

#include <clipboard.h>

typedef struct cb {     // clipboard region
    void *data;
    size_t data_size;
} cb;

void interrupt_f(int signum){
	printf("Terminating...\n");
	unlink(SERVER_ADDRESS);
	exit(0);
}

int main(){
	struct sockaddr_un server_addr;
    int server_fd, client_fd;
    cb clipboard[10];

    // init clipboard
    for (int i = 0; i < 10; i++) {
        clipboard[i].data = NULL;
        clipboard[i].data_size = 0;
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

        printf("copy region %d: %s\n", req.region, (char *) clipboard[req.region].data);

                    break;
                case PASTE:
                    // compared requested data size with stored data size and choose the lowest
                    bytes = (clipboard[req.region].data_size < req.data_size) ? clipboard[req.region].data_size : req.data_size;

                    /* if no data is stored in the given region (data == NULL, data_size == 0),
                     * data_size value is sent regardless and but the client won't attempt to read data */
                    if (write(client_fd, (void *) &bytes, sizeof(bytes)) != sizeof(bytes)) break;
                    if (clipboard[req.region].data != NULL) {
                        write(client_fd, clipboard[req.region].data, bytes);
                    }
                    /* write prettier code? ^ */

        printf("paste region %d:\tbytes: %d\t clipboard[req.region].data_size: %d\tdata_size: %d\n", req.region, (int) bytes, (int) clipboard[req.region].data_size, (int) req.data_size);

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
