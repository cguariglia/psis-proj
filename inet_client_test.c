#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_BACKLOG 10

u_short atous(const char *str) {
    u_short num;
    if (sscanf(str, "%hu", &num)) {
        return num;
    } else {
        return 0;
    }
}

int main(int argc, char **argv){

    if (argc != 3) exit(-1);

    struct sockaddr_in server_addr;
    int client_fd;

    // --- Socket setup ---
    // fd creation
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation error");
        exit(-1);
    }

    // address definition
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atous(argv[2]));
    inet_aton(argv[1], &server_addr.sin_addr);

    // connection to server
    if (connect(client_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("client socket connection");
        return(-1);
    }

    char str1[] = "asdf";
    char str2[] = "bit conneeeeeeeect";
    char str3[] = "qwerty";
    write(client_fd, str1, strlen(str1)+1);
    //sleep(1);
    write(client_fd, str2, strlen(str2)+1);
    //sleep(1);
    write(client_fd, str3, strlen(str3)+1);

    close(client_fd);

	exit(0);
}
