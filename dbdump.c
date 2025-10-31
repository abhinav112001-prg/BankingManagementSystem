/* src/dbdump.c - Database Dump Utility */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include "./include/types.h"

#define DATA_DIR "data"

void print_user(User *u) {
    const char *role = u->role == ROLE_ADMIN   ? "ADMIN" :
                       u->role == ROLE_MANAGER ? "MANAGER" :
                       u->role == ROLE_EMPLOYEE? "EMPLOYEE" : "CUSTOMER";
    char time_str[30] = "Never";
    if (u->last_login != 0) {
        struct tm *tm = localtime(&u->last_login);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);
    }
    printf("  [ID:%-3d] %-15s | Role: %-8s | Active: %s | Last Login: %s\n",
           u->id, u->username, role, u->active ? "YES" : "NO", time_str);
}

void print_account(Account *a) {
    printf("  [AccID:%-3d] UserID:%-3d | Balance: $%.2f | Tx Count: %d\n",
           a->accountID, a->userID, a->balance, a->transaction_count);
}

void print_transaction(TransactionRecord *t) {
    char time_str[30];
    struct tm *tm = localtime(&t->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);
    printf("  [TxID:%-3d] AccID:%-3d | %.2f | %s | %s\n",
           t->transactionID, t->accountID, t->amount, t->description, time_str);
}

void print_loan(Loan *l) {
    const char *status = l->status == LOAN_NEW       ? "NEW" :
                         l->status == LOAN_APPROVED  ? "APPROVED" :
                         l->status == LOAN_REJECTED  ? "REJECTED" : "UNKNOWN";
    char app_date[30], dec_date[30] = "N/A";
    struct tm *tm1 = localtime(&l->application_date);
    strftime(app_date, sizeof(app_date), "%Y-%m-%d", tm1);
    if (l->decision_date != 0) {
        struct tm *tm2 = localtime(&l->decision_date);
        strftime(dec_date, sizeof(dec_date), "%Y-%m-%d", tm2);
    }
    printf("  [LoanID:%-3d] Cust:%-3d | $%.2f | Status: %-8s | Applied: %s | Decided: %s | Emp: %d \n",
           l->loanID, l->custID, l->amount, status, app_date, dec_date,
           l->assigned_employeeID);
}

void print_feedback(Feedback *f) {
    char time_str[30];
    struct tm *tm = localtime(&f->timestamp);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);
    printf("  [FBID:%-3d] Cust:%-3d | %s | %s\n",
           f->feedbackID, f->custID, time_str, f->message);
}

void print_session(Session *s) {
    char time_str[30];
    struct tm *tm = localtime(&s->login_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm);
    printf("  [UserID:%-3d] Login: %s | Active: %s\n",
           s->user_id, time_str, s->session_active ? "YES" : "NO");
}

void dump_file(const char *filename, const char *title,
               void (*printer)(void *), size_t record_size, size_t header_size) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", DATA_DIR, filename);
    // snprintf(path, sizeof(path), "%s", filename);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Warning: Could not open %s (missing or empty)\n\n", path);
        return;
    }

    printf("=== %s ===\n", title);

    // Skip header
    lseek(fd, header_size, SEEK_SET);

    void *record = malloc(record_size);
    int count = 0;
    while (read(fd, record, record_size) == (ssize_t)record_size) {
        printer(record);
        count++;
    }
    free(record);
    close(fd);

    if (count == 0) printf("  (no records)\n");
    printf("\n");
}



int main() {
    
    printf("\n");
    printf("========================================\n");
    printf("     BANKING SYSTEM DATABASE DUMP\n");
    printf("========================================\n\n");

    dump_file("users.dat",       "USERS",          (void(*)(void*))print_user,       sizeof(User),       sizeof(UserHeader));
    dump_file("accounts.dat",    "ACCOUNTS",       (void(*)(void*))print_account,    sizeof(Account),    sizeof(AccountHeader));
    dump_file("transactions.dat","TRANSACTIONS",   (void(*)(void*))print_transaction,sizeof(Transaction),sizeof(TransactionHeader));
    dump_file("loans.dat",       "LOANS",          (void(*)(void*))print_loan,       sizeof(Loan),       sizeof(LoanHeader));
    dump_file("feedback.dat",    "FEEDBACK",       (void(*)(void*))print_feedback,   sizeof(Feedback),   sizeof(FeedbackHeader));
    dump_file("sessions.dat",    "SESSIONS",       (void(*)(void*))print_session,    sizeof(Session),    sizeof(LoanHeader));

    printf("Dump complete.\n\n");
    return 0;
    
}