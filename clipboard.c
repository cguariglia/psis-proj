// ?? //
#include <stdio.h>
#include <stdlib.h>
// ?? //

#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "clipboard.h"

int clipboard_connect(char * clipboard_dir){
	struct sockaddr_un server_addr;
    int client_fd;

    // --- Socket setup ---
    // fd creation
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        //perror("client socket creation");
        return(-1);
    }
    // server address
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_ADDRESS);
    // connect to server
    if (connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        //perror("client socket connection");
        return(-1);
    }

    return client_fd;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count){
    // check if region is valid
    if (region < 0 || region > 9) return 0;

    // compose request
    request c_msg;
    c_msg.type = COPY;
    c_msg.region = region;
    if ((c_msg.data = malloc(count)) == NULL) {
        //perror("Memory allocation error in copy operation");
        return 0;
    }
    memcpy(c_msg.data, buf, count);

    // send request: data to copy + region
    write(clipboard_id, (void *) c_msg, sizeof(c_msg));

    // process reply: bytes written
    int c_re;
    if (read(clipboard_id, (void *) &c_re, sizeof(c_re)) < sizeof(c_re)) {  //dúvida de contexto: devíamos pôr sizeof(int) no último sizeof?
        /* data expected is a single integer; if not enough
         * bytes are read, the whole message is discarded */
        return 0;
    }

    return c_re;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
    // check if region is valid
    if (region < 0 || region > 9) return 0;

    // compose request
    request p_msg;
    p_msg.type = PASTE;
    p_msg.region = region;
    p_msg.data = NULL;

    // send request: region
    write(clipboard_id, (void *) p_msg, sizeof(p_msg));

    // process reply: data + bytes of data received
    void *p_re;
    int num_bytes = read(clipboard_id, p_re, sizeof(p_re));

    if (num_bytes < sizeof(p_re)) return 0; // read error

    if ((buf = malloc(num_bytes)) == NULL) {
        //perror("Memory allocation error in paste operation");
        // could also be that num_bytes == 0, which is still valid
        return 0;
    }
    memcpy(buf, p_re, num_bytes);

    return num_bytes;
}
