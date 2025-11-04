#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "types.h"
#include "transactions.h"
#include "helpers.h"
#include "database.h"

int deposit (int customer_id, double amount, int socket_fd) {
    int accounts_fd, transactions_fd;
    Account *account;
    TransactionRecord transaction;

    // Validate amount
    if (amount <= 0) {
        send_response(socket_fd, "Invalid deposit amount\n");
        return -1;
    }

    // Lock accounts.dat
    accounts_fd = lock_file("data/accounts.dat", F_WRLCK);
    if (accounts_fd == -1) {
        send_response(socket_fd, "Failed to lock accounts file\n");
        return -1;
    }

    // Find account
    account = find_account_by_user_id(customer_id);
    if (account == NULL) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Account not found\n");
        return -1;
    }

    // Update balance (using the 0-based ID fix)
    double new_balance = account->balance + amount;
    lseek(accounts_fd, sizeof(AccountHeader) + (account->accountID) * sizeof(Account), SEEK_SET);
    account->balance = new_balance;
    account->transaction_count++;
    if (write(accounts_fd, account, sizeof(Account)) != sizeof(Account)) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Failed to update account\n");
        return -1;
    }
    fsync(accounts_fd);

    // Unlock accounts.dat
    unlock_file(accounts_fd);

    // Log transaction
    transactions_fd = lock_file("data/transactions.dat", F_WRLCK);
    if (transactions_fd == -1) {
        // Rollback: Subtract amount back
        accounts_fd = lock_file("data/accounts.dat", F_WRLCK);
        // Re-find account to be safe
        account = find_account_by_user_id(customer_id); 
        if(account != NULL) {
            account->balance = account->balance - amount;
            account->transaction_count++; // Log rollback
            lseek(accounts_fd, sizeof(AccountHeader) + (account->accountID) * sizeof(Account), SEEK_SET);
            write(accounts_fd, account, sizeof(Account));
            fsync(accounts_fd);
        }
        unlock_file(accounts_fd);
        send_response(socket_fd, "Failed to log transaction\n");
        return -1;
    }

    TransactionHeader trans_header;
    lseek(transactions_fd, 0, SEEK_SET); // 1. Go to start
    read(transactions_fd, &trans_header, sizeof(TransactionHeader)); // 2. Read header

    // Prepare transaction
    transaction.transactionID = trans_header.next_id++; // 3. Use and increment ID
    transaction.accountID = account->accountID;
    transaction.timestamp = time(NULL);
    snprintf(transaction.description, MAX_DESCRIPTION_LEN, "Deposit %.2f", amount);
    transaction.amount = amount;
    transaction.new_balance = new_balance;
    memset(transaction.reserved, 0, sizeof(transaction.reserved));

    // Append transaction
    lseek(transactions_fd, 0, SEEK_END);
    write(transactions_fd, &transaction, sizeof(TransactionRecord));

    // 4. Increment count and write header back
    lseek(transactions_fd, 0, SEEK_SET);
    trans_header.record_count++;
    write(transactions_fd, &trans_header, sizeof(TransactionHeader));
    fsync(transactions_fd);

    // Unlock transactions.dat
    unlock_file(transactions_fd);

    send_response(socket_fd, "Deposit successful\n");
    return 0;
}

// Withdraw 
int withdraw(int customer_id, double amount, int socket_fd) {
    int accounts_fd, transactions_fd;
    Account *account;
    TransactionRecord transaction;

    // Validate amount
    if (amount <= 0) {
        send_response(socket_fd, "Invalid withdrawal amount\n");
        return -1;
    }

    // Lock accounts.dat
    accounts_fd = lock_file("data/accounts.dat", F_WRLCK);
    if (accounts_fd == -1) {
        send_response(socket_fd, "Failed to lock accounts file\n");
        return -1;
    }

    // Find account
    account = find_account_by_user_id(customer_id);
    if (account == NULL) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Account not found\n");
        return -1;
    }

    // Check sufficient funds
    if (account->balance < amount) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Insufficient funds\n");
        return -1;
    }

    // Update balance (using the 0-based ID fix)
    double new_balance = account->balance - amount;
    lseek(accounts_fd, sizeof(AccountHeader) + (account->accountID) * sizeof(Account), SEEK_SET);
    account->balance = new_balance;
    account->transaction_count++;
    if (write(accounts_fd, account, sizeof(Account)) != sizeof(Account)) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Failed to update account\n");
        return -1;
    }
    fsync(accounts_fd);

    // Unlock accounts.dat
    unlock_file(accounts_fd);

    // Log transaction
    transactions_fd = lock_file("data/transactions.dat", F_WRLCK);
    if (transactions_fd == -1) {
        // Rollback: Add amount back
        accounts_fd = lock_file("data/accounts.dat", F_WRLCK);
        // Re-find account to be safe
        account = find_account_by_user_id(customer_id);
        if(account != NULL) {
            account->balance = account->balance + amount;
            account->transaction_count++; // Log rollback
            lseek(accounts_fd, sizeof(AccountHeader) + (account->accountID) * sizeof(Account), SEEK_SET);
            write(accounts_fd, account, sizeof(Account));
            fsync(accounts_fd);
        }
        unlock_file(accounts_fd);
        send_response(socket_fd, "Failed to log transaction\n");
        return -1;
    }

    TransactionHeader trans_header;
    lseek(transactions_fd, 0, SEEK_SET); // 1. Go to start
    read(transactions_fd, &trans_header, sizeof(TransactionHeader)); // 2. Read header

    // Prepare transaction
    transaction.transactionID = trans_header.next_id++; // 3. Use and increment ID
    transaction.accountID = account->accountID;
    transaction.timestamp = time(NULL);
    snprintf(transaction.description, MAX_DESCRIPTION_LEN, "Withdrawal %.2f", amount);
    transaction.amount = -amount; // Negative for withdrawal
    transaction.new_balance = new_balance;
    memset(transaction.reserved, 0, sizeof(transaction.reserved));

    // Append transaction
    lseek(transactions_fd, 0, SEEK_END);
    write(transactions_fd, &transaction, sizeof(TransactionRecord));

    // 4. Increment count and write header back
    lseek(transactions_fd, 0, SEEK_SET);
    trans_header.record_count++;
    write(transactions_fd, &trans_header, sizeof(TransactionHeader));
    fsync(transactions_fd);

    // Unlock transactions.dat
    unlock_file(transactions_fd);

    send_response(socket_fd, "Withdrawal successful\n");
    return 0;
}

// Helper: Log transaction (can be reused)
int log_transaction(int account_id, const char *description, double amount, double new_balance) {
    int fd = lock_file("data/transactions.dat", F_WRLCK);
    if (fd == -1) return -1;

    TransactionHeader header;
    read(fd, &header, sizeof(TransactionHeader));
    TransactionRecord transaction;
    transaction.transactionID = header.next_id++;
    transaction.accountID = account_id;
    transaction.timestamp = time(NULL);
    strncpy(transaction.description, description, MAX_DESCRIPTION_LEN);
    transaction.amount = amount;
    transaction.new_balance = new_balance;
    memset(transaction.reserved, 0, sizeof(transaction.reserved));

    lseek(fd, 0, SEEK_END);
    write(fd, &transaction, sizeof(TransactionRecord));
    lseek(fd, 0, SEEK_SET);
    header.record_count++;
    write(fd, &header, sizeof(TransactionHeader));
    fsync(fd);

    unlock_file(fd);
    return 0;
}

// Helper: Update account balance and transaction count
int update_account(int fd, Account *account, double new_balance) {
    
    lseek(fd, sizeof(AccountHeader) + (account->accountID) * sizeof(Account), SEEK_SET);
    account->balance = new_balance;
    account->transaction_count++;
    if (write(fd, account, sizeof(Account)) != sizeof(Account)) {
        return -1;
    }
    fsync(fd); // Ensure durability
    return 0;
}

int transferFunds(int customer_id, int socket_fd) {
    char recipient_username[MAX_USERNAME_LEN];
    double amount;
    int users_fd, accounts_fd;
    User *recipient_user;
    Account *sender_account, *recipient_account;

    // Read input from socket
    if (read_line_from_socket(socket_fd, recipient_username, MAX_USERNAME_LEN) != 0) {
        send_response(socket_fd, "Error reading recipient username\n");
        return -1;
    }
    amount = read_amount_from_socket(socket_fd);
    if (amount <= 0) {
        send_response(socket_fd, "Invalid transfer amount\n");
        return -1;
    }

    // Validate recipient
    users_fd = lock_file("data/users.dat", F_RDLCK);
    if (users_fd == -1) {
        send_response(socket_fd, "Failed to lock users file\n");
        return -1;
    }
    recipient_user = find_user_by_username(recipient_username);
    if (recipient_user == NULL || recipient_user->role != ROLE_CUSTOMER || recipient_user->active == 0) {
        unlock_file(users_fd);
        send_response(socket_fd, "Invalid or inactive recipient\n");
        return -1;
    }
    if (recipient_user->id == customer_id) {
        unlock_file(users_fd);
        send_response(socket_fd, "Cannot transfer to self\n");
        return -1;
    }
    unlock_file(users_fd);
    // free(recipient_user); // If find_user_by_username uses malloc

    // Lock accounts.dat
    accounts_fd = lock_file("data/accounts.dat", F_WRLCK);
    if (accounts_fd == -1) {
        send_response(socket_fd, "Failed to lock accounts file\n");
        return -1;
    }

    // Find accounts
    sender_account = find_account_by_user_id(customer_id);
    recipient_account = find_account_by_user_id(recipient_user->id);
    if (sender_account == NULL || recipient_account == NULL) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Account not found\n");
        return -1;
    }

    // Check sufficient funds
    if (sender_account->balance < amount) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Insufficient funds\n");
        return -1;
    }

    // Perform transfer
    if (withdraw(customer_id, amount, socket_fd) != 0) {
        unlock_file(accounts_fd);
        send_response(socket_fd, "Withdrawal failed\n");
        return -1;
    }
    if (deposit(recipient_user->id, amount, socket_fd) != 0) {
        // Rollback withdrawal
        deposit(customer_id, amount, socket_fd);
        unlock_file(accounts_fd);
        send_response(socket_fd, "Deposit failed\n");
        return -1;
    }

    unlock_file(accounts_fd);
    send_response(socket_fd, "Transfer successful\n");
    return 0;
}
/*
    user enters other user's name. name will be validated so that user doesnt add his own name.
    withdraw function called for curent user, pass current user id
    deposit function called for specified user, pass specified user id (will be extracted using the 
    specified user name)
*/
////