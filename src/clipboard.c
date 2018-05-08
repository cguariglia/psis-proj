// ?? //
#include <stdio.h>
#include <stdlib.h>
// ?? //

#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <clipboard.h>

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
    strcpy(server_addr.sun_path, clipboard_dir);
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
    c_msg.data = buf;
    c_msg.data_size = count;

    // send request: data to copy + region
    write(clipboard_id, (void *) &c_msg, sizeof(c_msg));

    // process reply: bytes written
    /* unnecessary??? now the client only sends a pointer,
     * not an actual copy of the data */
    int c_re;
    if (read(clipboard_id, (void *) &c_re, sizeof(c_re)) < (ssize_t) sizeof(c_re)) {  //dúvida de contexto: devíamos pôr sizeof(int) no último sizeof?
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
    p_msg.data_size = count;

    // send request: region
    write(clipboard_id, (void *) &p_msg, sizeof(p_msg));

    // process reply: data + bytes of data received
    void *p_re = NULL;
    ssize_t num_bytes = read(clipboard_id, p_re, sizeof(p_re));

    if (num_bytes < (ssize_t) sizeof(p_re)) return 0; // read error

    // limit pasted data to requested size
    if (num_bytes > count) {
        num_bytes = count;
    }

    if ((buf = malloc(num_bytes)) == NULL) {
        //perror("Memory allocation error in paste operation");
        // could also be that num_bytes == 0, which is still valid -- NOT!
        // num_bytes == 0 is considered an error i guess
        return 0;
    }
    memcpy(buf, p_re, num_bytes);

    return num_bytes;
}
