#ifndef SERVER_THREADS
#define SERVER_THREADS

void *accept_clients(void *type);
void *local_client_handler(void *fd);
void *remote_client_handler(void *fd);

#endif
