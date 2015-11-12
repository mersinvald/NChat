#include "non_block_io.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <libexplain/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "error.h"
#include "log.h"

#define MAX_IDLE_SECS 0
#define MAX_IDLE_USECS 1000

#define EWOULDBLOCK EAGAIN

int lc_sendto_non_block(int fd, void *data, size_t size, int flags, struct sockaddr *addr, socklen_t addr_len){
    fd_set wset;
    struct timeval timeout;
    int n, i, err;

    n = sendto(fd, data, size, flags, addr, addr_len);
    err = errno;
    if(n < 0){
        switch(err){
        case EINTR: return 0;
        case EAGAIN:
            FD_ZERO(&wset);
            FD_SET(fd, &wset);
            timeout.tv_sec = MAX_IDLE_SECS;
            timeout.tv_usec = MAX_IDLE_USECS;
            i = select(fd, NULL, &wset, NULL, &timeout);
            if(i < 0){
                lc_error("ERROR in send_non_block() - %s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
                return 0;
            } else
            if(i == 0) return 0;
            break;
        default:
            lc_error("ERROR in send_non_block() - %s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
            return -1;
        }
    }
    return n;
}

int lc_send_non_block(int fd, void* data, size_t size, int flags){
    fd_set wset;
    struct timeval timeout;
    int n, i, err;

    n = send(fd, data, size, flags);
    err = errno;
    if(n < 0){
        switch(err){
        case EINTR: return 0;
        case EAGAIN:
            FD_ZERO(&wset);
            FD_SET(fd, &wset);
            timeout.tv_sec = MAX_IDLE_SECS;
            timeout.tv_usec = MAX_IDLE_USECS;
            i = select(fd, NULL, &wset, NULL, &timeout);
            if(i < 0){
                lc_error("ERROR in send_non_block() - %s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
                return 0;
            } else
            if(i == 0) return 0;
            break;
        default:
            lc_error("ERROR in send_non_block() - %s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
            return -1;
        }
    }
    return n;
}

int lc_recvfrom_non_block(int fd, void *buffer, size_t size, int flags, struct sockaddr *addr, socklen_t *addr_len){
    fd_set rset;
    struct timeval timeout;
    int n, i;

    n = recvfrom(fd, buffer, size, flags, addr, addr_len);
    int err = errno;
    if(n < 0){
        switch(err){
        case EINTR: return 0;
        case EAGAIN:
            FD_ZERO(&rset);
            FD_SET(fd, &rset);
            timeout.tv_sec = MAX_IDLE_SECS;
            timeout.tv_usec = MAX_IDLE_USECS;
            i = select(fd, &rset, NULL, NULL, &timeout);
            if(i < 0){
                lc_error("ERROR in recvfrom_non_block() - %s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
                return 0;
            } else
            if(i == 0) return 0;
            break;
        default:
            lc_error("ERROR in recvfrom_non_block() - %s", explain_socket(AF_INET, SOCK_STREAM, 0));
            return -1;
        }
    }
    return n;
}

int lc_recv_non_block(int fd, void* buffer, size_t size, int flags){
    fd_set rset;
    struct timeval timeout;
    int n, i;

    n = recv(fd, buffer, size, flags);
    int err = errno;
    if(n < 0){
        switch(err){
        case EINTR: return 0;
        case EAGAIN:
            FD_ZERO(&rset);
            FD_SET(fd, &rset);
            timeout.tv_sec = MAX_IDLE_SECS;
            timeout.tv_usec = MAX_IDLE_USECS;
            i = select(fd, &rset, NULL, NULL, &timeout);
            if(i < 0){
                lc_error("ERROR in recv_non_block() - %s", explain_socket(AF_UNIX, SOCK_DGRAM, 0));
                return 0;
            } else
            if(i == 0) return 0;
            break;
        default:
            lc_error("ERROR in recv_non_block() - %s", explain_socket(AF_INET, SOCK_STREAM, 0));
            return -1;
        }
    }
    return n;
}
