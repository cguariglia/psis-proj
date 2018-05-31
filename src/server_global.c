#include <server_global.h>

cb clipboard[10];
client *local_client_list = NULL;
client *remote_client_list = NULL;
enum connection_mode mode;
int connected_fd;   // socket used to communicate with parent in CONNECTED mode
pthread_mutex_t sync_lock = PTHREAD_MUTEX_INITIALIZER;
int local_server_fd;
int tcp_server_fd;
