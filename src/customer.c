/*
getBalance(), 
deposit(),                  //transactions
withdraw(),                 //transactions
transferFunds(),            //transactions
applyLoan()
addFeedback(),
getTransactionHistory().    //transactions
*/
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "types.h"
#include "helpers.c"
#include "database.c"

int getBalance(int customer_id, int socket_fd) {
    int accounts_fd = lock_file("accounts.dat", F_WRLCK);
    if (accounts_fd == -1) {
        send_response(socket_fd, "Failed to lock accounts file\n");
        return -1;
    }
    Account *account=find_account_by_user_id(customer_id);
    if (account == NULL) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Account not found\n");
        return -1;
    }
    double balance = account->balance;
    char message[50];
    snprintf(message,sizeof(message), "Your balance is %.2f\n", balance);
    send_response(socket_fd, message);
    unlock_file(accounts_fd);
    return 0;
}

int addFeedback(int customer_id, int socket_fd){
    char msg[MAX_FEEDBACK_LEN];
    if (read_string_from_socket(socket_fd, msg, MAX_FEEDBACK_LEN) != 0) {
        send_response(socket_fd, "Error reading feedback\n");
        return -1;
    }
    msg[strcspn(msg, "\n")] = '\0';
    
    Feedback fdbk = {
        .custID = customer_id,
        .timestamp = time(NULL),
        .message = {0},
        .reserved = {0}
    };
    strncpy(fdbk.message, msg, MAX_FEEDBACK_LEN - 1);
    //open file
    int fd = open("feedback.dat", O_RDWR|O_CREAT, 0644);
    if (fd == -1) {
        send_response(socket_fd, "Failed to open feedback file\n");
        return -1;
    }
    // Lock
    struct flock lock = { 
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        send_response(socket_fd, "Failed to lock feedback file\n");
        close(fd);
        return -1;
    }
    // Fetch ID and update header
    FeedbackHeader header;
    read(fd, &header, sizeof(FeedbackHeader));
    fdbk.feedbackID = header.next_id++;
    header.record_count++;
    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(FeedbackHeader));
    // add in feedback table
    lseek(fd, 0, SEEK_END);
    write(fd, &fdbk, sizeof(Feedback));
    fsync(fd);
    // Unlock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    send_response(socket_fd, "Feedback submitted successfully\n");
    return 0;
}

int applyLoan(int customer_id, int socket_fd) {
    if (customer_id <= 0) {
        char *err_msg = "Invalid customer ID\n";
        write(socket_fd, err_msg, strlen(err_msg));
        return -1;
    }
    double loanAmt;
    printf("Enter loan amount: \n");
    scanf("%lf", loanAmt);
    if (read_double_from_socket(socket_fd, &loanAmt, sizeof(loanAmt)) != 0) {
        send_response(socket_fd, "Error reading loanAmt username\n");
        return -1;
    }
    if (loanAmt <= 0) {
        char *err_msg = "Invalid loan amount\n";
        write(socket_fd, err_msg, strlen(err_msg));
        return -1;
    }
    Loan new_loan = {
        .custID = customer_id,
        .amount = loanAmt,
        .status = LOAN_NEW,
        .assigned_employeeID = 0,
        .application_date = time(NULL),
        .decision_date = 0,
        .reserved = {0}
    };

    // Open file
    int fd;
    fd = open("loans.dat", O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        send_response(socket_fd, "Failed to open loans file\n");
        return -1;
    }
    // LOCK
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        send_response(socket_fd, "Failed to lock loans file\n");
        close(fd);
        return -1;
    }
    new_loan.loanID = fetch_and_increment_id(fd);
    // Append loan
    lseek(fd, 0, SEEK_END);
    write(fd, &new_loan, sizeof(Loan));
    fsync(fd);
    // Unlock file
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    send_response(socket_fd, "Loan application submitted successfully\n");
    return 0;
}

int viewTransactionHistory(int user_id, int socket_fd) {
    int accounts_fd, transactions_fd;
    Account *account;
    TransactionRecord transaction;
    char buffer[1024]; // Buffer for formatting output
    int bytes_written;

    // Lock accounts.dat to get account_id
    accounts_fd = lock_file("accounts.dat", F_RDLCK);
    if (accounts_fd == -1) {
        send_response(socket_fd, "Failed to lock accounts file\n");
        return -1;
    }

    // Find account
    account = find_account_by_user_id(user_id);
    if (account == NULL) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Account not found\n");
        return -1;
    }
    int account_id = account->accountID;
    unlock_file(accounts_fd);

    // Lock transactions.dat for reading
    transactions_fd = lock_file("transactions.dat", F_RDLCK);
    if (transactions_fd == -1) {
        send_response(socket_fd, "Failed to lock transactions file\n");
        return -1;
    }

    // Send header (display)
    snprintf(buffer, sizeof(buffer), "Transaction History for Account ID %d:\n", account_id);
    send_response(socket_fd, buffer);
    snprintf(buffer, sizeof(buffer), "%-20s %-30s %-10s %-10s\n", "Date", "Description", "Amount", "Balance");
    send_response(socket_fd, buffer);
    snprintf(buffer, sizeof(buffer), "------------------------------------------------------------\n");
    send_response(socket_fd, buffer);

    // Read transactions
    TransactionHeader header;
    if (read(transactions_fd, &header, sizeof(TransactionHeader)) != sizeof(TransactionHeader)) {
        unlock_file(transactions_fd);
        send_response(socket_fd, "Error reading transactions header\n");
        return -1;
    }

    lseek(transactions_fd, sizeof(TransactionHeader), SEEK_SET);
    while (read(transactions_fd, &transaction, sizeof(TransactionRecord)) == sizeof(TransactionRecord)) {
        if (transaction.accountID == account_id) {
            // Format transaction
            char time_str[26];
            ctime_r(&transaction.timestamp, time_str);
            time_str[strlen(time_str) - 1] = '\0'; // Remove newline
            snprintf(buffer, sizeof(buffer), "%-20s %-30s $%-9.2f $%-9.2f\n",
                     time_str, transaction.description, transaction.amount, transaction.new_balance);
            send_response(socket_fd, buffer);
        }
    }

    // Unlock transactions.dat
    unlock_file(transactions_fd);

    // Send footer
    snprintf(buffer, sizeof(buffer), "End of Transaction History\n");
    send_response(socket_fd, buffer);
    return 0;
}

int exitCustomer(int customer_id, int socket_fd) {
    int fd = lock_file("sessions.dat", F_WRLCK);
    if (fd == -1) {
        send_response(socket_fd, "Failed to lock sessions file\n");
        return -1;
    }
    Session session;
    while (read(fd, &session, sizeof(Session)) == sizeof(Session)) {
        if (session.user_id == customer_id && session.session_active) {
            session.session_active = 0;
            lseek(fd, -sizeof(Session), SEEK_CUR);
            write(fd, &session, sizeof(Session));
            fsync(fd);
            break;
        }
    }
    unlock_file(fd);
    send_response(socket_fd, "Session terminated\n");
    return 0;
}