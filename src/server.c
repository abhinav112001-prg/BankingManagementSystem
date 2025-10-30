// src/server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "types.h"
#include "database.h"
#include "helpers.h"
#include "customer.h"

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

// Initialize database files
void init_database() {
    const char *files[] = {
        "users.dat", 
        "accounts.dat", 
        "transactions.dat", 
        "loans.dat", 
        "feedback.dat", 
        "sessions.dat"
    };
    for (int i=0; i<6; i++) {
        int fd = open(files[i], O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            perror("Failed to create database file");
            exit(1);
        }
        // Write header if file is empty
        off_t size = lseek(fd, 0, SEEK_END);
        if (size == 0) {
            UserHeader header = { .next_id = 1, .record_count = 0 }; // Works for all headers
            write(fd, &header, sizeof(UserHeader));
            fsync(fd);
        }
        close(fd);
    }
}

void create_initial_admin() {
    if (find_user_by_username("admin") != NULL) return;

    int fd = open("users.dat", O_RDWR);
    if (fd == -1) return;

    UserHeader hdr = { .next_id = 1, .record_count = 0 };
    lseek(fd, 0, SEEK_SET);
    write(fd, &hdr, sizeof(UserHeader));

    User admin = {0};
    admin.id = 1;
    strcpy(admin.username, "admin");
    strcpy(admin.password_hash, "admin123");  // Change later!
    admin.role = ROLE_ADMIN;
    admin.active = 1;

    lseek(fd, 0, SEEK_END);
    write(fd, &admin, sizeof(User));
    fsync(fd);
    close(fd);
}


// Authenticate user
int authenticate_user(const char *username, const char *password, int *user_id, enum Role *role) {
    User *u = find_user_by_username(username);
    if (!u || !u->active) return -1;
    if (strcmp(u->password_hash, password) != 0) return -1;
    *user_id = u->id;
    *role    = u->role;
    return 0;
}

// Add session
int add_session(int user_id) {
    int fd = lock_file("sessions.dat", F_WRLCK);
    if (fd == -1) return -1;
    Session session;
    while (read(fd, &session, sizeof(Session)) == sizeof(Session)) {
        if (session.user_id == user_id && session.session_active) {
            unlock_file(fd);
            return -1; // Session already active
        }
    }
    Session new_session = {
        .user_id = user_id,
        .login_time = time(NULL),
        .session_active = 1,
        .reserved = {0}
    };
    lseek(fd, 0, SEEK_END);
    write(fd, &new_session, sizeof(Session));
    fsync(fd);
    unlock_file(fd);
    return 0;
}

// Helper: read full line from client
static int read_full_line(int fd, char *buf, size_t max) {
    ssize_t n = read(fd, buf, max - 1);
    if (n <= 0) return -1;
    buf[n] = '\0';
    return 0;
}

// Client handler
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int  user_id = -1;
    enum Role role;

    // LOGIN
    while (1) {
        if (read_full_line(client_fd, buffer, sizeof(buffer)) < 0) {
            close(client_fd);
            return NULL;
        }

        char cmd[32], username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN];
        if (sscanf(buffer, "%s %s %s", cmd, username, password) != 3)
            continue;

        if (strcmp(cmd, "LOGIN") != 0) {
            send_response(client_fd, "Send LOGIN <user> <pass>\n");
            continue;
        }

        if (authenticate_user(username, password, &user_id, &role) != 0) {
            send_response(client_fd, "Login failed\n");
            continue;
        }

        if (add_session(user_id) != 0) {
            send_response(client_fd, "Session already active\n");
            continue;
        }

        const char *role_str = (role == ROLE_CUSTOMER) ? "CUSTOMER" :
                               (role == ROLE_EMPLOYEE) ? "EMPLOYEE" :
                               (role == ROLE_MANAGER)  ? "MANAGER"  : "ADMIN";
        snprintf(buffer, sizeof(buffer), "Login successful ROLE_%s\n", role_str);
        send_response(client_fd, buffer);
        break;
    }

    // COMMAND LOOP (role-based)
    while (1) {
        if (read_full_line(client_fd, buffer, sizeof(buffer)) < 0)
            break;                                   // client disconnected
        if (role == ROLE_CUSTOMER) {
            char cmd[32];
            sscanf(buffer, "%s", cmd);

            if (strcmp(cmd, "BALANCE") == 0) {
                getBalance(user_id, client_fd);
            }
            else if (strcmp(cmd, "DEPOSIT") == 0) {
                double amt = read_amount_from_socket(client_fd);
                deposit(user_id, amt, client_fd);
            }
            else if (strcmp(cmd, "WITHDRAW") == 0) {
                double amt = read_amount_from_socket(client_fd);
                withdraw(user_id, amt, client_fd);
            }
            else if (strcmp(cmd, "TRANSFER") == 0) {
                transferFunds(user_id, client_fd);
            }
            else if (strcmp(cmd, "LOAN") == 0) {
                applyLoan(user_id, client_fd);
            }
            else if (strcmp(cmd, "FEEDBACK") == 0) {
                addFeedback(user_id, client_fd);
            }
            else if (strcmp(cmd, "HISTORY") == 0) {
                viewTransactionHistory(user_id, client_fd);
            }
            else if (strcmp(cmd, "EXIT") == 0) {
                exitCustomer(user_id, client_fd);
                break;
            }
            else {
                send_response(client_fd, "Unknown command\n");
            }
        }

        else if (role == ROLE_EMPLOYEE) {
            char cmd[32];
            sscanf(buffer, "%s", cmd);

            if (strcmp(cmd, "ADD_CUST") == 0)         addNewCustomer(client_fd);
            else if (strcmp(cmd, "EDIT_CUST") == 0)   editCustomerDetails(client_fd);
            else if (strcmp(cmd, "LOAN_DECIDE") == 0) approveRejectLoans(user_id, client_fd);
            else if (strcmp(cmd, "MY_LOANS") == 0)    viewAssignedLoanApplications(user_id, client_fd);
            else if (strcmp(cmd, "CUST_TRANS") == 0)  viewCustomerTransactions(client_fd);
            else if (strcmp(cmd, "EXIT") == 0) {
                exitCustomer(user_id, client_fd);
                break;
            }
            else {
                send_response(client_fd, "Unknown employee command\n");
            }
        }

        else if (role == ROLE_MANAGER) {
            char cmd[32], a1[64] = {0}, a2[64] = {0};
            sscanf(buffer, "%s %s %s", cmd, a1, a2);

            if (strcmp(cmd, "ADD_CUST") == 0)          addNewCustomer(client_fd);
            else if (strcmp(cmd, "EDIT_CUST") == 0)    editCustomerDetails(client_fd);
            else if (strcmp(cmd, "MY_LOANS") == 0)     viewAssignedLoanApplications(user_id, client_fd);
            else if (strcmp(cmd, "LOAN_DECIDE") == 0)  approveRejectLoans(user_id, client_fd);
            else if (strcmp(cmd, "CUST_TRANS") == 0)   viewCustomerTransactions(client_fd);
            else if (strcmp(cmd, "ASSIGN_LOAN") == 0) {
                int loan_id = atoi(a1);
                int emp_id  = atoi(a2);
                assignLoanToEmployee(user_id, loan_id, emp_id, client_fd);
            }
            else if (strcmp(cmd, "VIEW_FEEDBACK") == 0) viewAllFeedback(client_fd);
            else if (strcmp(cmd, "VIEW_USERS") == 0)    viewAllUsers(client_fd);
            else if (strcmp(cmd, "EXIT") == 0) {
                exitCustomer(user_id, client_fd);
                break;
            }
            else {
                send_response(client_fd, "Unknown manager command\n");
            }
        }

        else if (role == ROLE_ADMIN) {
            char cmd[32];
            sscanf(buffer, "%s", cmd);

            if (strcmp(cmd, "ADD_EMP") == 0)         addEmployee(client_fd);
            else if (strcmp(cmd, "ADD_MGR") == 0)    addManager(client_fd);
            else if (strcmp(cmd, "VIEW_USERS") == 0) viewAllUsers(client_fd);
            else if (strcmp(cmd, "DEACTIVATE") == 0) deactivateUser(user_id, client_fd);
            else if (strcmp(cmd, "REACTIVATE") == 0) reactivateUser(user_id, client_fd);
            else if (strcmp(cmd, "VIEW_LOGS") == 0)  viewSystemLogs(client_fd);
            else if (strcmp(cmd, "EXIT") == 0) {
                exitCustomer(user_id, client_fd);
                break;
            }
            else {
                send_response(client_fd, "Unknown admin command\n");
            }
        }
    }
    close(client_fd);
    return NULL;
}


int main() {
    init_database();
    create_initial_admin();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d ...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (*client_fd < 0) {
            perror("accept");
            free(client_fd);
            continue;
        }
        pthread_t th;
        if (pthread_create(&th, NULL, handle_client, client_fd) != 0) {
            perror("pthread_create");
            close(*client_fd);
            free(client_fd);
            continue;
        }
        pthread_detach(th);
    }

    close(server_fd);
    return 0;
}