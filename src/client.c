/* src/client.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "types.h"
#include "helpers.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

static int read_line(int sock, char *buf, size_t max)
{
    ssize_t n = read(sock, buf, max - 1);
    if (n <= 0) return -1;
    buf[n] = '\0';
    return 0;
}

/* --------------------------------------------------------------- */
/* Customer menu                                                   */
/* --------------------------------------------------------------- */
static void customer_menu(int sock)
{
    while (1) {
        printf("\n=== CUSTOMER MENU ===\n");
        printf("1. View Balance\n2. Deposit\n3. Withdraw\n4. Transfer Funds\n");
        printf("5. Apply for Loan\n6. Add Feedback\n7. View Transaction History\n8. Exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();
        char buffer[BUFFER_SIZE];
        int send_ok = 1;
        switch (choice) {
            case 1:
                snprintf(buffer, sizeof(buffer), "BALANCE");
                break;

            case 2: {
                double amt;
                printf("Amount to deposit: ");
                scanf("%lf", &amt); getchar();
                snprintf(buffer, sizeof(buffer), "DEPOSIT %.2f", amt);
                break;
            }
            case 3: {
                double amt;
                printf("Amount to withdraw: ");
                scanf("%lf", &amt); getchar();
                snprintf(buffer, sizeof(buffer), "WITHDRAW %.2f", amt);
                break;
            }
            case 4: {
                char recip[MAX_USERNAME_LEN];
                double amt;
                printf("Recipient username: ");
                fgets(recip, sizeof(recip), stdin);
                recip[strcspn(recip, "\n")] = '\0';
                printf("Amount to transfer: ");
                scanf("%lf", &amt); getchar();
                snprintf(buffer, sizeof(buffer), "TRANSFER %s", recip);
                write(sock, buffer, strlen(buffer));
                snprintf(buffer, sizeof(buffer), "%.2f", amt);
                break;
            }
            case 5: {
                double amt;
                printf("Loan amount: ");
                scanf("%lf", &amt); getchar();
                snprintf(buffer, sizeof(buffer), "LOAN %.2f", amt);
                break;
            }
            case 6: {
                char fb[MAX_FEEDBACK_LEN];
                printf("Feedback: ");
                fgets(fb, sizeof(fb), stdin);
                fb[strcspn(fb, "\n")] = '\0';
                snprintf(buffer, sizeof(buffer), "FEEDBACK %s", fb);
                break;
            }
            case 7:
                snprintf(buffer, sizeof(buffer), "HISTORY");
                break;

            case 8:
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return;

            default:
                printf("Invalid choice\n");
                send_ok = 0;
        }

        if (send_ok) {
            write(sock, buffer, strlen(buffer));
            if (read_line(sock, buffer, sizeof(buffer)) == 0)
                printf("%s", buffer);
        }
    }
}

/* --------------------------------------------------------------- */
/* Employee menu                                                   */
/* --------------------------------------------------------------- */
static void employee_menu(int sock)
{
    while (1) {
        printf("\n=== EMPLOYEE MENU ===\n");
        printf("1. Add Customer\n2. Edit Customer\n");
        printf("3. View My Assigned Loans\n4. Approve/Reject Loan\n");
        printf("5. View Customer Transactions\n6. Exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        char buffer[BUFFER_SIZE];
        int  send_ok = 1;

        switch (choice) {
            case 1:  snprintf(buffer, sizeof(buffer), "ADD_CUST"); break;
            case 2:  snprintf(buffer, sizeof(buffer), "EDIT_CUST"); break;
            case 3:  snprintf(buffer, sizeof(buffer), "MY_LOANS"); break;
            case 4:  snprintf(buffer, sizeof(buffer), "LOAN_DECIDE"); break;
            case 5:  snprintf(buffer, sizeof(buffer), "CUST_TRANS"); break;
            case 6:
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return;
            default:
                printf("Invalid choice\n");
                send_ok = 0;
        }

        if (send_ok) {
            write(sock, buffer, strlen(buffer));
            if (read_line(sock, buffer, sizeof(buffer)) == 0)
                printf("%s", buffer);
        }
    }
}

/* --------------------------------------------------------------- */
/* Manager menu                                                    */
/* --------------------------------------------------------------- */
static void manager_menu(int sock)
{
    while (1) {
        printf("\n=== MANAGER MENU ===\n");
        printf("1. Add Customer\n2. Edit Customer\n");
        printf("3. View My Assigned Loans\n4. Approve/Reject Loan\n");
        printf("5. View Customer Transactions\n");
        printf("6. Assign Loan to Employee (LOAN_ID EMP_ID)\n");
        printf("7. View All Feedback\n8. View All Users\n9. Exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        char buffer[BUFFER_SIZE];
        int  send_ok = 1;

        switch (choice) {
            case 1:  snprintf(buffer, sizeof(buffer), "ADD_CUST"); break;
            case 2:  snprintf(buffer, sizeof(buffer), "EDIT_CUST"); break;
            case 3:  snprintf(buffer, sizeof(buffer), "MY_LOANS"); break;
            case 4:  snprintf(buffer, sizeof(buffer), "LOAN_DECIDE"); break;
            case 5:  snprintf(buffer, sizeof(buffer), "CUST_TRANS"); break;
            case 6: {
                int loan_id, emp_id;
                printf("Loan ID: ");  scanf("%d", &loan_id); getchar();
                printf("Employee ID: "); scanf("%d", &emp_id); getchar();
                snprintf(buffer, sizeof(buffer), "ASSIGN_LOAN %d %d", loan_id, emp_id);
                break;
            }
            case 7:  snprintf(buffer, sizeof(buffer), "VIEW_FEEDBACK"); break;
            case 8:  snprintf(buffer, sizeof(buffer), "VIEW_USERS"); break;
            case 9:
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return;
            default:
                printf("Invalid choice\n");
                send_ok = 0;
        }

        if (send_ok) {
            write(sock, buffer, strlen(buffer));
            if (read_line(sock, buffer, sizeof(buffer)) == 0)
                printf("%s", buffer);
        }
    }
}

/* --------------------------------------------------------------- */
/* Admin menu                                                      */
/* --------------------------------------------------------------- */
static void admin_menu(int sock)
{
    while (1) {
        printf("\n=== ADMIN MENU ===\n");
        printf("1. Add Employee\n2. Add Manager\n");
        printf("3. View All Users\n4. Deactivate User\n");
        printf("5. Reactivate User\n6. View Logs\n7. Exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        getchar();

        char buffer[BUFFER_SIZE];
        int  send_ok = 1;

        switch (choice) {
            case 1:  snprintf(buffer, sizeof(buffer), "ADD_EMP"); break;
            case 2:  snprintf(buffer, sizeof(buffer), "ADD_MGR"); break;
            case 3:  snprintf(buffer, sizeof(buffer), "VIEW_USERS"); break;
            case 4:  snprintf(buffer, sizeof(buffer), "DEACTIVATE"); break;
            case 5:  snprintf(buffer, sizeof(buffer), "REACTIVATE"); break;
            case 6:  snprintf(buffer, sizeof(buffer), "VIEW_LOGS"); break;
            case 7:
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return;
            default:
                printf("Invalid choice\n");
                send_ok = 0;
        }

        if (send_ok) {
            write(sock, buffer, strlen(buffer));
            if (read_line(sock, buffer, sizeof(buffer)) == 0)
                printf("%s", buffer);
        }
    }
}

/* --------------------------------------------------------------- */
/* MAIN                                                            */
/* --------------------------------------------------------------- */
int main(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    struct sockaddr_in srv = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    if (inet_pton(AF_INET, SERVER_IP, &srv.sin_addr) <= 0) {
        perror("inet_pton"); exit(1);
    }
    if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("connect"); exit(1);
    }

    /* ---------- login ---------- */
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN];
    char buffer[BUFFER_SIZE];

    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    snprintf(buffer, sizeof(buffer), "LOGIN %s %s", username, password);
    write(sock, buffer, strlen(buffer));

    if (read_line(sock, buffer, sizeof(buffer)) < 0) {
        printf("Server disconnected\n");
        close(sock);
        return 1;
    }
    printf("%s", buffer);
    if (strstr(buffer, "successful") == NULL) {
        close(sock);
        return 1;
    }

    /* ---------- role dispatch ---------- */
    if (strstr(buffer, "ROLE_CUSTOMER"))   customer_menu(sock);
    else if (strstr(buffer, "ROLE_EMPLOYEE")) employee_menu(sock);
    else if (strstr(buffer, "ROLE_MANAGER"))  manager_menu(sock);
    else if (strstr(buffer, "ROLE_ADMIN"))    admin_menu(sock);
    else {
        printf("Unknown role – exiting.\n");
        close(sock);
    }

    return 0;
}