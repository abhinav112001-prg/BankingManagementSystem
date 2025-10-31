#ifndef CUSTOMER_H
#define CUSTOMER_H
#include "types.h"

int getBalance(int customer_id, int socket_fd);

int addFeedback(int customer_id, int socket_fd);
int applyLoan(int customer_id, int socket_fd);
int viewTransactionHistory(int user_id, int socket_fd);
int transferFunds(int customer_id, int socket_fd);
int exitCustomer(int customer_id, int socket_fd);
#endif