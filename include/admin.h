#ifndef ADMIN_H
#define ADMIN_H
#include "types.h"

int addEmployee(int socket_fd);
int addManager(int socket_fd);
int viewAllUsers(int socket_fd);
int deactivateUser(int admin_id, int socket_fd);
int reactivateUser(int admin_id, int socket_fd);
int viewSystemLogs(int socket_fd);

#endif