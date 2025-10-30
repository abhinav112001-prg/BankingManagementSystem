#ifndef TYPES_H
#define TYPES_H

#include <time.h>

#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 64  // For hashed password
#define MAX_FEEDBACK_LEN 100
#define MAX_DESCRIPTION_LEN 100

enum Role {
    ROLE_CUSTOMER,
    ROLE_EMPLOYEE,
    ROLE_MANAGER,
    ROLE_ADMIN
};

enum LoanStatus {
    LOAN_NEW,
    LOAN_APPROVED,
    LOAN_REJECTED
};

// TRANSACTION
typedef struct {
    time_t timestamp;
    char description[100];  // e.g., "Deposit 1000"
    double amount;
    double new_balance;
} Transaction;

// USER
typedef struct {
    int id;                  // Unique auto-incrementing ID (Primary key)
    char username[MAX_USERNAME_LEN];
    char password_hash[MAX_PASSWORD_LEN];
    enum Role role;
    int active;              // 1 = active, 0 = deactivated (for managers)
    time_t last_login;       // Timestamp for session tracking
    char reserved[100];      // Padding for future use
} User;

// USER File Header (written at start of file)
typedef struct {
    int next_id;             // For auto-increment
    int record_count;
} UserHeader;

// ACCOUNTS
typedef struct {
    int accountID;          // Unique ID (auto-increment)
    int userID;             // Foreign key
    double balance;
    int transaction_count;
    char reserved[100];      // Padding
} Account;

// ACCOUNT File Header
typedef struct {
    int next_id;
    int record_count;
} AccountHeader;

// TRANSACTIONS RECORD
typedef struct {
    int transactionID;      // Unique global ID
    int accountID;          // Foreign key
    time_t timestamp;
    char description[MAX_DESCRIPTION_LEN];  // e.g., "Deposit 1000"
    double amount;
    double new_balance;
    char reserved[100];      // Padding
} TransactionRecord;

// TRANSACTIONS File Header
typedef struct {
    int next_id;
    int record_count;
} TransactionHeader;

// LOANS
typedef struct {
    int loanID;             // Unique ID
    int custID;    // Foreign key to Users (customer)
    double amount;
    enum LoanStatus status;
    int assigned_employeeID; // Foreign key to Users (employee), 0 if unassigned
    time_t application_date;
    time_t decision_date;
    char reserved[100];
} Loan;

// LOANS File Header
typedef struct {
    int next_id;
    int record_count;
} LoanHeader;

// FEEDBACK
typedef struct {
    int feedbackID;         // Unique ID
    int custID;
    time_t timestamp;
    char message[MAX_FEEDBACK_LEN];
    char reserved[100];      // Padding
} Feedback;

// File Header
typedef struct {
    int next_id;
    int record_count;
} FeedbackHeader;

// SESSION
typedef struct {
    int user_id;
    time_t login_time;
    int session_active;      // 1 = active
    char reserved[100];      // Padding
} Session;

#endif
