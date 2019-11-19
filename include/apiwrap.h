#ifndef APIWRAP_H_
#define APIWRAP_H_

#include "api.h"

extern "C" {

int __wrap_socket(int domain, int type, int protocol);

int __wrap_bind(int socket, const struct sockaddr *address,
                socklen_t address_len);

int __wrap_listen(int socket, int backlog);

int __wrap_connect(int socket, const struct sockaddr *address,
                   socklen_t address_len);

int __wrap_accept(int socket, struct sockaddr *address, socklen_t *address_len);

ssize_t __wrap_read(int fildes, void *buf, size_t nbyte);

ssize_t __wrap_write(int fildes, const void *buf, size_t nbyte);

ssize_t __wrap_close(int fildes);

int __wrap_getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints, struct addrinfo **res);

void __wrap_freeaddrinfo(struct addrinfo *res);
}

#endif
