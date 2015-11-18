#ifndef NON_BLOCK_IO_H
#define NON_BLOCK_IO_H

#include <sys/types.h>
#include <sys/socket.h>

extern int lc_sendto_non_block(int fd, void* data, size_t size, int flags, struct sockaddr* addr, socklen_t addr_len);
extern int lc_send_non_block(int fd, void* data, size_t size, int flags);

extern int lc_recvfrom_non_block(int fd, void* buffer, size_t size, int flags, struct sockaddr* addr, socklen_t* addr_len);
extern int lc_recv_non_block(int fd, void* buffer, size_t size, int flags);
#endif // NON_BLOCK_IO_H
