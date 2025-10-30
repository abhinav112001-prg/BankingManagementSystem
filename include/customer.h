#ifndef CUSTOMER_H
#define CUSTOMER_H
#include "types.h"

int getBalance(int customer_id, int socket_fd);
int update_account(int fd, Account *account, double new_balance);
int deposit(int customer_id, double amount, int socket_fd);
int withdraw(int customer_id, double amount, int socket_fd);
int log_transaction(int account_id, const char *description, double amount, double new_balance);
int addFeedback(int customer_id, int socket_fd);
int applyLoan(int customer_id, int socket_fd);
int viewTransactionHistory(int user_id, int socket_fd);
int transferFunds(int customer_id, int socket_fd);

#endif