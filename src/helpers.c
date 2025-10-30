#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <stdio.h>
#include "helpers.h"

void send_response(int socket_fd, const char *message) {
    write(socket_fd, message, strlen(message));
}

int read_username_from_socket(int socket_fd, char *username, size_t max_len) {
    char buffer[MAX_USERNAME_LEN];
    ssize_t bytes_read = read(socket_fd, buffer, max_len - 1);
    if (bytes_read <= 0) return -1;
    buffer[bytes_read] = '\0';
    strncpy(username, buffer, max_len);
    return 0;
}

int read_string_from_socket(int socket_fd, char *buffer, size_t max_len) {
    ssize_t bytes_read = read(socket_fd, buffer, max_len - 1);
    if (bytes_read <= 0) return -1;
    buffer[bytes_read] = '\0';
    return 0;
}
int read_line_from_socket(int fd, char *buf, size_t max) {
    ssize_t n = read(fd, buf, max - 1);
    if (n <= 0) return -1;
    buf[n] = '\0';
    /* strip trailing newline if present */
    char *nl = strchr(buf, '\n');
    if (nl) *nl = '\0';
    return 0;
}
int read_double_from_socket(int socket_fd, double *dbl, size_t max_len) {
    ssize_t bytes_read = read(socket_fd, dbl, max_len - 1);
    if (bytes_read <= 0) return -1;
    return 0;
}

double read_amount_from_socket(int socket_fd) {
    char buffer[32];
    ssize_t bytes_read = read(socket_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) return -1.0;
    buffer[bytes_read] = '\0';
    double amount;
    sscanf(buffer, "%lf", &amount);
    return amount;
}