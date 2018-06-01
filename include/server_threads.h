#ifndef SERVER_THREADS
#define SERVER_THREADS

#include <server_global.h>

void *accept_clients(void *type);
void close_connection(void *args);
void *local_client_handler(void *fd);
void *remote_peer_handler(void *fd);

#endif
