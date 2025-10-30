#ifndef EMPLOYEE_H
#define EMPLOYEE_H
#include "types.h"

int addNewCustomer(int socket_fd);
int editCustomerDetails(int socket_fd);
int approveRejectLoans(int employee_id, int socket_fd);
int viewAssignedLoanApplications(int employee_id, int socket_fd);
int viewCustomerTransactions(int socket_fd);

int assignLoanToEmployee(int manager_id, int loan_id, int employee_id, int socket_fd);
int viewAllFeedback(int socket_fd);

#endif