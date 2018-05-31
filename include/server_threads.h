#ifndef SERVER_THREADS
#define SERVER_THREADS

#include <server_request.h>

void *accept_clients(void *type);
void *local_client_handler(void *fd);
void *remote_client_handler(void *fd);

#endif
