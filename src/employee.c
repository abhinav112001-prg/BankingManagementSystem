#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "types.h"
#include "helpers.h"
#include "database.h"
#include "transactions.h"
#include "customer.h"
#include "employee.h"

// addNewCustomer()
// editCustomerDetails()
// approveRejectLoans()
// viewAssignedLoanApplications()
// viewCustomerTransactions()


// 1. Add New Customer
int addNewCustomer(int socket_fd) {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    double initial_balance;

    /* Read username */
    if (read_line_from_socket(socket_fd, username, sizeof(username)) != 0) {
        send_response(socket_fd, "Error reading username\n");
        return -1;
    }
    username[strcspn(username, "\n")] = '\0';

    /* Read password */
    if (read_line_from_socket(socket_fd, password, sizeof(password)) != 0) {
        send_response(socket_fd, "Error reading password\n");
        return -1;
    }
    password[strcspn(password, "\n")] = '\0';

    /* Validate */
    if (strlen(username) == 0 || strlen(password) == 0) {
        send_response(socket_fd, "Username and password required\n");
        return -1;
    }

    initial_balance = read_amount_from_socket(socket_fd);
    if (initial_balance < 0) {
        send_response(socket_fd, "Invalid initial balance\n");
        return -1;
    }

    int users_fd = lock_file("data/users.dat", F_WRLCK);
    if (users_fd == -1) { send_response(socket_fd, "Lock users.dat failed\n"); return -1; }

    if (find_user_by_username(username) != NULL) {
        unlock_file(users_fd);
        send_response(socket_fd, "Username already exists\n");
        return -1;
    }

    UserHeader uhdr;
    read(users_fd, &uhdr, sizeof(UserHeader));
    User new_user = {0};
    new_user.id = uhdr.next_id++;
    strncpy(new_user.username, username, MAX_USERNAME_LEN-1);
    strncpy(new_user.password_hash, password, MAX_PASSWORD_LEN-1); 
    new_user.role = ROLE_CUSTOMER;
    new_user.active = 1;
    new_user.last_login = 0;
    memset(new_user.reserved, 0, sizeof(new_user.reserved));

    lseek(users_fd, 0, SEEK_SET);
    write(users_fd, &uhdr, sizeof(UserHeader));
    lseek(users_fd, 0, SEEK_END);
    write(users_fd, &new_user, sizeof(User));
    fsync(users_fd);
    unlock_file(users_fd);

    /* ---- accounts.dat ---- */
    int accounts_fd = lock_file("data/accounts.dat", F_WRLCK);
    if (accounts_fd == -1) { send_response(socket_fd, "Lock accounts.dat failed\n"); return -1; }

    AccountHeader ahdr;
    read(accounts_fd, &ahdr, sizeof(AccountHeader));
    Account new_acc = {0};
    new_acc.accountID = ahdr.next_id++;
    new_acc.userID   = new_user.id;
    new_acc.balance  = initial_balance;
    new_acc.transaction_count = 0;
    memset(new_acc.reserved, 0, sizeof(new_acc.reserved));

    lseek(accounts_fd, 0, SEEK_SET);
    write(accounts_fd, &ahdr, sizeof(AccountHeader));
    lseek(accounts_fd, 0, SEEK_END);
    write(accounts_fd, &new_acc, sizeof(Account));
    fsync(accounts_fd);
    unlock_file(accounts_fd);

    send_response(socket_fd, "Customer added successfully\n");
    return 0;
}


// 2. Edit Customer Details
int editCustomerDetails(int socket_fd) {
    char username[MAX_USERNAME_LEN];
    if (read_string_from_socket(socket_fd, username, MAX_USERNAME_LEN) != 0) {
        send_response(socket_fd, "Error reading username\n");
        return -1;
    }

    int users_fd = lock_file("data/users.dat", F_WRLCK);
    if (users_fd == -1) { send_response(socket_fd, "Lock users.dat failed\n"); return -1; }

    User *u = find_user_by_username(username);
    if (u == NULL || u->role != ROLE_CUSTOMER) {
        unlock_file(users_fd);
        send_response(socket_fd, "Customer not found\n");
        return -1;
    }

    /* Send current details */
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Current: Username=%s  Active=%d\nEnter new username (or . to keep): ",
             u->username, u->active);
    send_response(socket_fd, buf);

    char new_user[MAX_USERNAME_LEN];
    if (read_string_from_socket(socket_fd, new_user, MAX_USERNAME_LEN) != 0 ||
        new_user[0] != '.') {
        /* change username */
        if (find_user_by_username(new_user) != NULL) {
            unlock_file(users_fd);
            send_response(socket_fd, "New username already taken\n");
            return -1;
        }
        strncpy(u->username, new_user, MAX_USERNAME_LEN-1);
    }

    send_response(socket_fd, "Enter new password (or . to keep): ");
    char new_pass[MAX_PASSWORD_LEN];
    if (read_string_from_socket(socket_fd, new_pass, MAX_PASSWORD_LEN) != 0 ||
        new_pass[0] != '.') {
        strncpy(u->password_hash, new_pass, MAX_PASSWORD_LEN-1);
    }

    send_response(socket_fd, "Set active (1/0): ");
    int active;
    char act_buf[8];
    if (read_string_from_socket(socket_fd, act_buf, sizeof(act_buf)) == 0)
        active = atoi(act_buf);
    else
        active = u->active;
    u->active = (active == 0) ? 0 : 1;

    /* rewrite the record */
    lseek(users_fd, sizeof(UserHeader) + (u->id-1)*sizeof(User), SEEK_SET);
    write(users_fd, u, sizeof(User));
    fsync(users_fd);
    unlock_file(users_fd);

    send_response(socket_fd, "Customer details updated\n");
    return 0;
}

/* --------------------------------------------------------------------- */
/* 3. Approve / Reject Loans                                             */
/* --------------------------------------------------------------------- */
int approveRejectLoans(int employee_id, int socket_fd) {
    /* First send list of assigned pending loans */
    viewAssignedLoanApplications(employee_id, socket_fd);

    /* Employee picks loanID */
    int loan_id;
    char buf[32];
    if (read_string_from_socket(socket_fd, buf, sizeof(buf)) != 0) {
        send_response(socket_fd, "Error reading loan ID\n");
        return -1;
    }
    loan_id = atoi(buf);

    int loans_fd = lock_file("data/loans.dat", F_WRLCK);
    if (loans_fd == -1) { send_response(socket_fd, "Lock loans.dat failed\n"); return -1; }

    LoanHeader lhdr;
    read(loans_fd, &lhdr, sizeof(LoanHeader));
    Loan loan;
    int found = 0;
    off_t pos = sizeof(LoanHeader);
    while (read(loans_fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.loanID == loan_id && loan.assigned_employeeID == employee_id &&
            loan.status == LOAN_NEW) {
            found = 1;
            break;
        }
        pos += sizeof(Loan);
    }
    if (!found) {
        unlock_file(loans_fd);
        send_response(socket_fd, "Loan not found or not assigned to you\n");
        return -1;
    }

    send_response(socket_fd, "Enter A (approve) or R (reject): ");
    char choice[8];
    if (read_string_from_socket(socket_fd, choice, sizeof(choice)) != 0) {
        unlock_file(loans_fd);
        send_response(socket_fd, "Error reading choice\n");
        return -1;
    }

    if (choice[0] == 'A' || choice[0] == 'a') {
        loan.status = LOAN_APPROVED;
        /* OPTIONAL: credit the amount to the customer's account */
        Account *acc = find_account_by_user_id(loan.custID);
        if (acc) {
            deposit(loan.custID, loan.amount, socket_fd);   // reuse deposit logic
        }
    } else if (choice[0] == 'R' || choice[0] == 'r') {
        loan.status = LOAN_REJECTED;
        // send_response(socket_fd, "Enter rejection reason: ");
        // read_string_from_socket(socket_fd, loan.reason, sizeof(loan.reason));
    } else {
        unlock_file(loans_fd);
        send_response(socket_fd, "Invalid choice\n");
        return -1;
    }

    loan.decision_date = time(NULL);
    lseek(loans_fd, pos, SEEK_SET);
    write(loans_fd, &loan, sizeof(Loan));
    fsync(loans_fd);
    unlock_file(loans_fd);

    send_response(socket_fd, "Loan decision recorded\n");
    return 0;
}

/* --------------------------------------------------------------------- */
/* 4. View Assigned Loan Applications                                    */
/* --------------------------------------------------------------------- */
int viewAssignedLoanApplications(int employee_id, int socket_fd) {
    int fd = lock_file("data/loans.dat", F_RDLCK);
    if (fd == -1) { send_response(socket_fd, "Lock loans.dat failed\n"); return -1; }

    LoanHeader hdr;
    read(fd, &hdr, sizeof(LoanHeader));

    char line[256];
    snprintf(line, sizeof(line),
             "Assigned Pending Loans (Employee %d):\n"
             "%-8s %-12s %-10s %-12s\n",
             employee_id, "LoanID", "CustomerID", "Amount", "Applied");
    send_response(socket_fd, line);
    snprintf(line, sizeof(line), "-----------------------------------------\n");
    send_response(socket_fd, line);

    Loan loan;
    int count = 0;
    while (read(fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.assigned_employeeID == employee_id && loan.status == LOAN_NEW) {
            char tbuf[30];
            struct tm *tm_info = localtime(&loan.application_date);
            strftime(tbuf, sizeof(tbuf), "%Y-%m-%d", tm_info);
            snprintf(line, sizeof(line), "%-8d %-12d $%-9.2f %-12s\n",
                     loan.loanID, loan.custID, loan.amount, tbuf);
            send_response(socket_fd, line);
            count++;
        }
    }
    if (count == 0) send_response(socket_fd, "(none)\n");
    unlock_file(fd);
    return 0;
}

/* --------------------------------------------------------------------- */
/* 5. View Customer Transactions                                         */
/* --------------------------------------------------------------------- */
int viewCustomerTransactions(int socket_fd) {
    char username[MAX_USERNAME_LEN];
    if (read_string_from_socket(socket_fd, username, MAX_USERNAME_LEN) != 0) {
        send_response(socket_fd, "Error reading username\n");
        return -1;
    }

    User *u = find_user_by_username(username);
    if (u == NULL || u->role != ROLE_CUSTOMER) {
        send_response(socket_fd, "Customer not found\n");
        return -1;
    }

    /* Re-use the same function that customers use */
    return viewTransactionHistory(u->id, socket_fd);
}

/* -----------------------------------------*/
/* 6. (Manager) Assign Loan to Employee     */
/* -----------------------------------------*/
int assignLoanToEmployee(int manager_id, int loan_id, int employee_id, int socket_fd) {
    /* Verify manager */
    User *mgr = find_user_by_id(manager_id);
    if (mgr == NULL || mgr->role != ROLE_MANAGER) {
        send_response(socket_fd, "Permission denied\n");
        return -1;
    }
    /* Verify employee */
    User *emp = find_user_by_id(employee_id);
    if (emp == NULL || emp->role != ROLE_EMPLOYEE) {
        send_response(socket_fd, "Invalid employee\n");
        return -1;
    }

    int fd = lock_file("data/loans.dat", F_WRLCK);
    if (fd == -1) { send_response(socket_fd, "Lock loans.dat failed\n"); return -1; }

    LoanHeader hdr;
    read(fd, &hdr, sizeof(LoanHeader));
    Loan loan;
    int found = 0;
    off_t pos = sizeof(LoanHeader);
    while (read(fd, &loan, sizeof(Loan)) == sizeof(Loan)) {
        if (loan.loanID == loan_id && loan.status == LOAN_NEW) {
            loan.assigned_employeeID = employee_id;
            found = 1;
            break;
        }
        pos += sizeof(Loan);
    }
    if (!found) {
        unlock_file(fd);
        send_response(socket_fd, "Loan not found or not pending\n");
        return -1;
    }

    lseek(fd, pos, SEEK_SET);
    write(fd, &loan, sizeof(Loan));
    fsync(fd);
    unlock_file(fd);

    char msg[128];
    snprintf(msg, sizeof(msg), "Loan %d assigned to employee %d\n", loan_id, employee_id);
    send_response(socket_fd, msg);
    return 0;
}

/* --------------------------------------------------------------------- */
/* 7. View All Feedback                                                  */
/* --------------------------------------------------------------------- */
int viewAllFeedback(int socket_fd) {
    int fd = lock_file("data/feedback.dat", F_RDLCK);
    if (fd == -1) { send_response(socket_fd, "Lock feedback.dat failed\n"); return -1; }

    FeedbackHeader hdr;
    read(fd, &hdr, sizeof(FeedbackHeader));

    char line[256];
    snprintf(line, sizeof(line),
             "Customer Feedback (%d entries):\n"
             "%-8s %-12s %-20s %s\n",
             hdr.record_count, "ID", "CustID", "Date", "Message");
    send_response(socket_fd, line);
    snprintf(line, sizeof(line), "------------------------------------------------------------\n");
    send_response(socket_fd, line);

    Feedback fb;
    while (read(fd, &fb, sizeof(Feedback)) == sizeof(Feedback)) {
        char tbuf[30];
        struct tm *tm_info = localtime(&fb.timestamp);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M", tm_info);
        snprintf(line, sizeof(line), "%-8d %-12d %-20s %.80s\n",
                 fb.feedbackID, fb.custID, tbuf, fb.message);
        send_response(socket_fd, line);
    }
    unlock_file(fd);
    return 0;
}