#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

int withdraw(int user_id, double amount, int socket_fd);
int deposit(int user_id, double amount, int socket_fd);
int log_transaction(int account_id, const char *description, double amount, double new_balance);
int transferFunds(int customer_id, int socket_fd);

#endif