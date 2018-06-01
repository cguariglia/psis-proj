#ifndef SYNC_PROTOCOL_H
#define SYNC_PROTOCOL_H

int8_t send_ask_parent(int region, size_t data_size, void *buffer);
int store_buffered(int fd, int region, size_t data_size, void *buffer);
void send_sync_children(int region, size_t data_size);
void send_desync_parent(int region);
void store_not_buffered(int fd, int region, size_t data_size);
void send_desync_children(int region);

#endif
