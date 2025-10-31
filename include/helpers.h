#ifndef HELPERS_H
#define HELPERS_H
#include "types.h"

void send_response(int socket_fd, const char *message);
int read_username_from_socket(int socket_fd, char *username, size_t max_len);
int read_string_from_socket(int socket_fd, char *buffer, size_t max_len);
double read_amount_from_socket(int socket_fd);
int read_line_from_socket(int fd, char *buf, size_t max);
int read_double_from_socket(int socket_fd, double *dbl, size_t max_len);

#endif