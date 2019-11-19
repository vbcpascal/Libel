#include "apiwrap.h"

int __wrap_socket(int domain, int type, int protocol) {
  return api::socket::socket(domain, type, protocol);
}

int __wrap_bind(int socket, const struct sockaddr* address,
                socklen_t address_len) {
  return api::socket::bind(socket, address, address_len);
}

int __wrap_listen(int socket, int backlog) {
  return api::socket::listen(socket, backlog);
}

int __wrap_connect(int socket, const struct sockaddr* address,
                   socklen_t address_len) {
  return api::socket::connect(socket, address, address_len);
}

int __wrap_accept(int socket, struct sockaddr* address,
                  socklen_t* address_len) {
  return api::socket::accept(socket, address, address_len);
}

ssize_t __wrap_read(int fildes, void* buf, size_t nbyte) {
  return api::socket::read(fildes, buf, nbyte);
}

ssize_t __wrap_write(int fildes, const void* buf, size_t nbyte) {
  return api::socket::write(fildes, buf, nbyte);
}

ssize_t __wrap_close(int fildes) { return api::socket::close(fildes); }

int __wrap_getaddrinfo(const char* node, const char* service,
                       const struct addrinfo* hints, struct addrinfo** res) {
  return api::socket::getaddrinfo(node, service, hints, res);
}

void __wrap_freeaddrinfo(struct addrinfo* res) {
  return api::socket::freeaddrinfo(res);
}