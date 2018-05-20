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
    c_msg.data_size = count;

    // send request
    if (write(clipboard_id, (void *) &c_msg, sizeof(c_msg)) != sizeof(c_msg)) return 0;

    // send data
    /* verifying this write is pointless as it will not cause errors server-side
     * and the server will reply with how many bytes were read */
    write(clipboard_id, buf, count);

    // receive reply (bytes written)
    ssize_t bytes_written;
    if (read(clipboard_id, (void *) &bytes_written, sizeof(bytes_written)) != sizeof(bytes_written)) return 0;
    if (bytes_written == -1) return 0;

    return (int) bytes_written;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count){
    // check if region is valid
    if (region < 0 || region > 9) return 0;

    // compose request
    request p_msg;
    p_msg.type = PASTE;
    p_msg.region = region;
    p_msg.data_size = count;

    // send request
    if (write(clipboard_id, (void *) &p_msg, sizeof(p_msg)) != sizeof(p_msg)) return 0;

    // receive 1st reply (data size)
    ssize_t expected_bytes;
    if (read(clipboard_id, (void *) &expected_bytes, sizeof(expected_bytes)) != sizeof(expected_bytes)) return 0;
    if (expected_bytes <= 0) {printf("\tfailed to paste from reg. %d\n", region); return 0;}

    // receive 2nd reply (data)
    ssize_t bytes_read;
    if ((bytes_read = read(clipboard_id, buf, expected_bytes)) == -1) return 0;

    return (int) bytes_read;
}

void clipboard_close(int clipboard_id){
    request c_msg;
    c_msg.type = CLOSE;
    c_msg.region = -1;      // unused
    c_msg.data_size = 0;    // unused

    // send message
    write(clipboard_id, (void *) &c_msg, sizeof(c_msg));    // error checking is pointless...

    return;
}
