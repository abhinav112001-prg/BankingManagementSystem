#ifndef HELPERS_H
#define HELPERS_H
#include "types.h"

void send_response(int socket_fd, const char *message);
int read_string_from_socket(int socket_fd, char *buffer, size_t max_len);
double read_amount_from_socket(int socket_fd);

#endif