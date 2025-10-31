// src/client.c
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
        if (scanf("%d", &choice) != 1) { 
            while(getchar() != '\n');
            printf("Invalid choice\n");
            continue; 
        }
        getchar(); // Consume newline
        
        char buffer[BUFFER_SIZE];
        
        switch (choice) {
            case 1: // BALANCE
                snprintf(buffer, sizeof(buffer), "BALANCE");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                break;

            case 2: { // DEPOSIT
                double amt;
                printf("Amount to deposit: ");
                scanf("%lf", &amt); getchar();
                
                snprintf(buffer, sizeof(buffer), "DEPOSIT");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                snprintf(buffer, sizeof(buffer), "%.2f", amt);
                write(sock, buffer, strlen(buffer)); // 2. Send amount
                break;
            }
            case 3: { // WITHDRAW
                double amt;
                printf("Amount to withdraw: ");
                scanf("%lf", &amt); getchar();
                
                snprintf(buffer, sizeof(buffer), "WITHDRAW");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                snprintf(buffer, sizeof(buffer), "%.2f", amt);
                write(sock, buffer, strlen(buffer)); // 2. Send amount
                break;
            }
            case 4: { // TRANSFER
                char recip[MAX_USERNAME_LEN];
                double amt;
                printf("Recipient username: ");
                fgets(recip, sizeof(recip), stdin);
                // recip[strcspn(recip, "\n")] = '\0'; // Keep newline, read_line_from_socket handles it
                
                printf("Amount to transfer: ");
                scanf("%lf", &amt); getchar();
                
                snprintf(buffer, sizeof(buffer), "TRANSFER");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                write(sock, recip, strlen(recip)); // 2. Send recipient
                
                snprintf(buffer, sizeof(buffer), "%.2f", amt);
                write(sock, buffer, strlen(buffer)); // 3. Send amount
                break;
            }
            case 5: { // LOAN
                double amt;
                printf("Loan amount: ");
                scanf("%lf", &amt); getchar();
                
                snprintf(buffer, sizeof(buffer), "LOAN");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                snprintf(buffer, sizeof(buffer), "%.2f", amt);
                write(sock, buffer, strlen(buffer)); // 2. Send amount
                break;
            }
            case 6: { // FEEDBACK
                char fb[MAX_FEEDBACK_LEN];
                printf("Feedback: ");
                fgets(fb, sizeof(fb), stdin);
                // fb[strcspn(fb, "\n")] = '\0'; // Keep newline
                
                snprintf(buffer, sizeof(buffer), "FEEDBACK");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                write(sock, fb, strlen(fb)); // 2. Send feedback string
                break;
            }
            case 7: // HISTORY
                snprintf(buffer, sizeof(buffer), "HISTORY");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                break;

            case 8: // EXIT
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return;

            default:
                printf("Invalid choice\n");
                continue; // Skip the read
        }
        
        // General response read for cases 1-7
        if (choice >= 1 && choice <= 7) {
            if (read_line(sock, buffer, sizeof(buffer)) == 0)
                printf("%s", buffer);
            
            // Handle TRANSFER's extra responses
            // deposit() -> "Deposit successful"
            // withdraw() -> "Withdrawal successful"
            // transferFunds() -> "Transfer successful"
            // All 3 functions are called
            if (choice == 4) {
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Read 2nd response
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Read 3rd response
            }
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
        if (scanf("%d", &choice) != 1) { 
            while(getchar() != '\n'); // Clear invalid input buffer
            printf("Invalid choice\n");
            continue; 
        }
        getchar(); // Consume the newline after scanf

        char buffer[BUFFER_SIZE];
        
        switch (choice) {
            case 1: // ADD_EMP
                snprintf(buffer, sizeof(buffer), "ADD_EMP");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                printf("Username: ");  fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 2. Send username
                
                printf("Password: ");  fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send password
                break; // Break to read response

            case 2: // ADD_MGR
                snprintf(buffer, sizeof(buffer), "ADD_MGR");
                write(sock, buffer, strlen(buffer)); // 1. Send command
                
                printf("Username: ");  fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 2. Send username
                
                printf("Password: ");  fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send password
                break; // Break to read response
            
            case 3: // VIEW_USERS
                snprintf(buffer, sizeof(buffer), "VIEW_USERS");
                write(sock, buffer, strlen(buffer));
                break;

            case 4: // DEACTIVATE
                printf("Username to deactivate: ");
                fgets(buffer, sizeof(buffer), stdin);
                
                write(sock, "DEACTIVATE", strlen("DEACTIVATE")); // 1. Send command
                write(sock, buffer, strlen(buffer)); // 2. Send username
                break;
                
            case 5: // REACTIVATE
                printf("Username to reactivate: ");
                fgets(buffer, sizeof(buffer), stdin);

                write(sock, "REACTIVATE", strlen("REACTIVATE")); // 1. Send command
                write(sock, buffer, strlen(buffer)); // 2. Send username
                break;

            case 6: // VIEW_LOGS
                snprintf(buffer, sizeof(buffer), "VIEW_LOGS");
                write(sock, buffer, strlen(buffer));
                break;

            case 7: // EXIT
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return; // Exit function

            default:
                printf("Invalid choice\n");
                continue; // Skip the write/read block
        }

        // Common read block for cases 1-6
        if (choice >= 1 && choice <= 6) {
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
        if (scanf("%d", &choice) != 1) { 
            while(getchar() != '\n'); // Clear invalid input
            printf("Invalid choice\n");
            continue; 
        }
        getchar(); // Consume the newline after scanf

        char buffer[BUFFER_SIZE];

        switch (choice) {
            case 1: { // ADD_CUST
                snprintf(buffer, sizeof(buffer), "ADD_CUST");
                write(sock, buffer, strlen(buffer)); // 1. Send command

                printf("Enter new customer username: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 2. Send username

                printf("Enter new customer password: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send password

                printf("Enter initial balance: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 4. Send balance

                // Now, get the final response
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break; // Done with this case
            }
            case 2: { // EDIT_CUST
                printf("Enter customer username to edit: ");
                fgets(buffer, sizeof(buffer), stdin);
                
                write(sock, "EDIT_CUST", strlen("EDIT_CUST")); // 1. Send command
                write(sock, buffer, strlen(buffer)); // 2. Send username

                // Server sends "Current: ... Enter new username:"
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); 
                
                fgets(buffer, sizeof(buffer), stdin); // Get new username
                write(sock, buffer, strlen(buffer)); // 3. Send new username

                // Server sends "Enter new password:"
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);

                fgets(buffer, sizeof(buffer), stdin); // Get new password
                write(sock, buffer, strlen(buffer)); // 4. Send new password

                // Server sends "Set active (1/0):"
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);

                fgets(buffer, sizeof(buffer), stdin); // Get active status
                write(sock, buffer, strlen(buffer)); // 5. Send active status

                // Server sends final "Customer details updated"
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                
                break; // Done with this case
            }
            case 3: // MY_LOANS
                snprintf(buffer, sizeof(buffer), "MY_LOANS");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break; 

            case 4: { // LOAN_DECIDE
                snprintf(buffer, sizeof(buffer), "LOAN_DECIDE");
                write(sock, buffer, strlen(buffer)); // 1. Send command

                // Server sends the list of loans
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Print the loan list

                printf("Enter Loan ID to decide on: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 2. Send loan ID

                // Server sends "Enter A (approve) or R (reject):"
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);

                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send choice (A/R)

                // Server sends final "Loan decision recorded"
                // (It might also send "Deposit successful" first if approved)
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                if (strstr(buffer, "Deposit successful") != NULL) {
                     if (read_line(sock, buffer, sizeof(buffer)) == 0)
                        printf("%s", buffer);
                }
                
                break; // Done with this case
            }
            case 5: { // CUST_TRANS
                printf("Enter customer username to view transactions: ");
                fgets(buffer, sizeof(buffer), stdin);
                
                write(sock, "CUST_TRANS", strlen("CUST_TRANS")); // 1. Send command
                write(sock, buffer, strlen(buffer)); // 2. Send username
                
                // Server sends back the transaction list (multiple lines)
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                    
                break; // Done with this case
            }
            case 6: // EXIT
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return; // Exit function
                
            default:
                printf("Invalid choice\n");
                continue; // Skip I/O
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
        printf("--- Employee Actions ---\n");
        printf("1. Add Customer\n2. Edit Customer\n");
        printf("3. View My Assigned Loans\n4. Approve/Reject Loan\n");
        printf("5. View Customer Transactions\n");
        printf("--- Manager Actions ---\n");
        printf("6. Assign Loan to Employee\n");
        printf("7. View All Feedback\n");
        printf("8. View All Users\n");
        printf("9. Exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) { 
            while(getchar() != '\n'); // Clear invalid input
            printf("Invalid choice\n");
            continue; 
        }
        getchar(); // Consume the newline after scanf

        char buffer[BUFFER_SIZE];

        switch (choice) {
            case 1: { // ADD_CUST (Interactive)
                snprintf(buffer, sizeof(buffer), "ADD_CUST");
                write(sock, buffer, strlen(buffer)); // 1. Send command

                printf("Enter new customer username: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 2. Send username

                printf("Enter new customer password: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send password

                printf("Enter initial balance: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 4. Send balance

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break;
            }
            case 2: { // EDIT_CUST (Interactive)
                printf("Enter customer username to edit: ");
                fgets(buffer, sizeof(buffer), stdin);
                
                write(sock, "EDIT_CUST", strlen("EDIT_CUST")); // 1. Send command
                write(sock, buffer, strlen(buffer)); // 2. Send username

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Server: "Current: ... Enter new username:"
                
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send new username

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Server: "Enter new password:"

                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 4. Send new password

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Server: "Set active (1/0):"

                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 5. Send active status

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Server: "Customer details updated"
                
                break;
            }
            case 3: // MY_LOANS (Simple)
                snprintf(buffer, sizeof(buffer), "MY_LOANS");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break;

            case 4: { // LOAN_DECIDE (Interactive)
                snprintf(buffer, sizeof(buffer), "LOAN_DECIDE");
                write(sock, buffer, strlen(buffer)); // 1. Send command

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Print the loan list

                printf("Enter Loan ID to decide on: ");
                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 2. Send loan ID

                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Server: "Enter A (approve) or R (reject):"

                fgets(buffer, sizeof(buffer), stdin);
                write(sock, buffer, strlen(buffer)); // 3. Send choice (A/R)

                // Read responses
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                if (strstr(buffer, "Deposit successful") != NULL) {
                     if (read_line(sock, buffer, sizeof(buffer)) == 0)
                        printf("%s", buffer);
                }
                break;
            }
            case 5: { // CUST_TRANS (Interactive)
                printf("Enter customer username to view transactions: ");
                fgets(buffer, sizeof(buffer), stdin);
                
                write(sock, "CUST_TRANS", strlen("CUST_TRANS")); // 1. Send command
                write(sock, buffer, strlen(buffer)); // 2. Send username
                
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break;
            }
            case 6: { // ASSIGN_LOAN (Non-Interactive)
                int loan_id, emp_id;
                printf("Loan ID: ");  scanf("%d", &loan_id); getchar();
                printf("Employee ID: "); scanf("%d", &emp_id); getchar();
                
                // This is the *one* command that sends args at once
                snprintf(buffer, sizeof(buffer), "ASSIGN_LOAN %d %d", loan_id, emp_id);
                
                write(sock, buffer, strlen(buffer)); // Send command + args
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer); // Read response
                break;
            }
            case 7: // VIEW_FEEDBACK (Simple)
                snprintf(buffer, sizeof(buffer), "VIEW_FEEDBACK");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break;

            case 8: // VIEW_USERS (Simple)
                snprintf(buffer, sizeof(buffer), "VIEW_USERS");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                break;

            case 9: // EXIT
                snprintf(buffer, sizeof(buffer), "EXIT");
                write(sock, buffer, strlen(buffer));
                if (read_line(sock, buffer, sizeof(buffer)) == 0)
                    printf("%s", buffer);
                close(sock);
                return; // Exit function
                
            default:
                printf("Invalid choice\n");
                // No write, no read, just loop again
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
    char role_str[32];
    int role_choice = 0;

    printf("Select your role:\n");
    printf("1. Customer\n2. Employee\n3. Manager\n4. Admin\n");
    printf("Choice: ");
    
    while (role_choice == 0) {
        if (scanf("%d", &role_choice) != 1) {
            getchar(); // clear invalid input
            printf("Invalid choice. Try again: ");
            continue;
        }
        getchar(); // consume newline
        switch (role_choice) {
            case 1: strncpy(role_str, "CUSTOMER", sizeof(role_str)); break;
            case 2: strncpy(role_str, "EMPLOYEE", sizeof(role_str)); break;
            case 3: strncpy(role_str, "MANAGER", sizeof(role_str)); break;
            case 4: strncpy(role_str, "ADMIN", sizeof(role_str)); break;
            default:
                printf("Invalid choice. Try again: ");
                role_choice = 0;
        }
    }

    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    snprintf(buffer, sizeof(buffer), "LOGIN %s %s %s", role_str, username, password);
    write(sock, buffer, strlen(buffer));

    if (read_line(sock, buffer, sizeof(buffer)) < 0) {
        printf("Server disconnected\n");
        close(sock);
        return 1;
    }
    
    printf("%s", buffer); // Print server response (e.g., "Login successful" or error)
    
    if (strstr(buffer, "successful") == NULL) {
        close(sock);
        return 1;
    }

    /* ---------- role dispatch ---------- */
    if (strcmp(role_str, "CUSTOMER") == 0)   customer_menu(sock);
    else if (strcmp(role_str, "EMPLOYEE") == 0) employee_menu(sock);
    else if (strcmp(role_str, "MANAGER") == 0)  manager_menu(sock);
    else if (strcmp(role_str, "ADMIN") == 0)    admin_menu(sock);
    else {
        printf("Unknown role – exiting.\n");
        close(sock);
    }

    return 0;
}