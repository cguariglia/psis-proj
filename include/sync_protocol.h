#ifndef SYNC_PROTOCOL_H
#define SYNC_PROTOCOL_H

void send_ask_parent(int region, size_t data_size, void *buffer);
int recv_sync_children(int region, size_t data_size, void *buffer);
void send_sync_children(int region, size_t data_size, void *buffer);
void send_desync_parent(int region);
void recv_desync_children(int region, size_t data_size);
void send_desync_children(int region);

#endif
