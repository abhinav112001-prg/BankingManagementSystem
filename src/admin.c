/* src/admin.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "types.h"
#include "database.h"
#include "helpers.h"
#include "admin.h"

/* --------------------------------------------------------------------- */
/* 1. Add Employee                                                       */
/* --------------------------------------------------------------------- */
int addEmployee(int socket_fd) {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

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

    int users_fd = lock_file("data/users.dat", F_WRLCK);
    if (users_fd == -1) {
        send_response(socket_fd, "Failed to lock users.dat\n");
        return -1;
    }

    if (find_user_by_username(username) != NULL) {
        unlock_file(users_fd);
        send_response(socket_fd, "Username already exists\n");
        return -1;
    }

    UserHeader hdr;
    read(users_fd, &hdr, sizeof(UserHeader));

    User new_user = {0};
    new_user.id = hdr.next_id++;
    strncpy(new_user.username, username, MAX_USERNAME_LEN - 1);
    strncpy(new_user.password_hash, password, MAX_PASSWORD_LEN - 1);
    new_user.role = ROLE_EMPLOYEE;
    new_user.active = 1;
    new_user.last_login = 0;
    memset(new_user.reserved, 0, sizeof(new_user.reserved));

    lseek(users_fd, 0, SEEK_SET);
    write(users_fd, &hdr, sizeof(UserHeader));
    lseek(users_fd, 0, SEEK_END);
    write(users_fd, &new_user, sizeof(User));
    fsync(users_fd);
    unlock_file(users_fd);

    send_response(socket_fd, "Employee added successfully!\n");
    return 0;
}

/* --------------------------------------------------------------------- */
/* 2. Add Manager                                                        */
/* --------------------------------------------------------------------- */
int addManager(int socket_fd) {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

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

    int users_fd = lock_file("data/users.dat", F_WRLCK);
    if (users_fd == -1) {
        send_response(socket_fd, "Failed to lock users.dat\n");
        return -1;
    }

    if (find_user_by_username(username) != NULL) {
        unlock_file(users_fd);
        send_response(socket_fd, "Username already exists\n");
        return -1;
    }

    UserHeader hdr;
    read(users_fd, &hdr, sizeof(UserHeader));

    User new_user = {0};
    new_user.id = hdr.next_id++;
    strncpy(new_user.username, username, MAX_USERNAME_LEN - 1);
    strncpy(new_user.password_hash, password, MAX_PASSWORD_LEN - 1);
    new_user.role = ROLE_MANAGER;
    new_user.active = 1;
    new_user.last_login = 0;
    memset(new_user.reserved, 0, sizeof(new_user.reserved));

    lseek(users_fd, 0, SEEK_SET);
    write(users_fd, &hdr, sizeof(UserHeader));
    lseek(users_fd, 0, SEEK_END);
    write(users_fd, &new_user, sizeof(User));
    fsync(users_fd);
    unlock_file(users_fd);

    send_response(socket_fd, "Manager added successfully\n");
    return 0;
}

/* --------------------------------------------------------------------- */
/* 3. View All Users                                                     */
/* --------------------------------------------------------------------- */
int viewAllUsers(int socket_fd) {
    int fd = lock_file("data/users.dat", F_RDLCK);
    if (fd == -1) {
        send_response(socket_fd, "Failed to lock users.dat\n");
        return -1;
    }

    UserHeader hdr;
    read(fd, &hdr, sizeof(UserHeader));

    char line[256];
    snprintf(line, sizeof(line),
             "All Users (%d total):\n"
             "%-6s %-15s %-10s %-6s %-12s\n",
             hdr.record_count, "ID", "Username", "Role", "Active", "Last Login");
    send_response(socket_fd, line);
    snprintf(line, sizeof(line), "------------------------------------------------------\n");
    send_response(socket_fd, line);

    User user;
    while (read(fd, &user, sizeof(User)) == sizeof(User)) {
        const char *role_str = (user.role == ROLE_CUSTOMER) ? "Customer" :
                               (user.role == ROLE_EMPLOYEE) ? "Employee" :
                               (user.role == ROLE_MANAGER)  ? "Manager"  : "Admin";
        char time_str[30] = "Never";
        if (user.last_login != 0) {
            struct tm *tm_info = localtime(&user.last_login);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
        }
        snprintf(line, sizeof(line), "%-6d %-15s %-10s %-6s %-12s\n",
                 user.id, user.username, role_str,
                 user.active ? "Yes" : "No", time_str);
        send_response(socket_fd, line);
    }
    unlock_file(fd);
    return 0;
}

/* --------------------------------------------------------------------- */
/* 4. Deactivate User                                                    */
/* --------------------------------------------------------------------- */
int deactivateUser(int admin_id, int socket_fd) {
    char username[MAX_USERNAME_LEN];
    if (read_string_from_socket(socket_fd, username, MAX_USERNAME_LEN) != 0) {
        send_response(socket_fd, "Error reading username\n");
        return -1;
    }

    int users_fd = lock_file("data/users.dat", F_WRLCK);
    if (users_fd == -1) {
        send_response(socket_fd, "Failed to lock users.dat\n");
        return -1;
    }

    User *u = find_user_by_username(username);
    if (u == NULL) {
        unlock_file(users_fd);
        send_response(socket_fd, "User not found\n");
        return -1;
    }
    if (u->id == admin_id) {
        unlock_file(users_fd);
        send_response(socket_fd, "Cannot deactivate self\n");
        return -1;
    }
    if (u->role == ROLE_ADMIN) {
        unlock_file(users_fd);
        send_response(socket_fd, "Cannot deactivate another admin\n");
        return -1;
    }

    u->active = 0;
    lseek(users_fd, sizeof(UserHeader) + (u->id - 1) * sizeof(User), SEEK_SET);
    write(users_fd, u, sizeof(User));
    fsync(users_fd);
    unlock_file(users_fd);

    send_response(socket_fd, "User deactivated\n");
    return 0;
}

/* --------------------------------------------------------------------- */
/* 5. Reactivate User                                                    */
/* --------------------------------------------------------------------- */
int reactivateUser(int admin_id, int socket_fd) {
    char username[MAX_USERNAME_LEN];
    if (read_string_from_socket(socket_fd, username, MAX_USERNAME_LEN) != 0) {
        send_response(socket_fd, "Error reading username\n");
        return -1;
    }

    int users_fd = lock_file("data/users.dat", F_WRLCK);
    if (users_fd == -1) {
        send_response(socket_fd, "Failed to lock users.dat\n");
        return -1;
    }

    User *u = find_user_by_username(username);
    if (u == NULL) {
        unlock_file(users_fd);
        send_response(socket_fd, "User not found\n");
        return -1;
    }
    if (u->active) {
        unlock_file(users_fd);
        send_response(socket_fd, "User already active\n");
        return -1;
    }

    u->active = 1;
    lseek(users_fd, sizeof(UserHeader) + (u->id - 1) * sizeof(User), SEEK_SET);
    write(users_fd, u, sizeof(User));
    fsync(users_fd);
    unlock_file(users_fd);

    send_response(socket_fd, "User reactivated\n");
    return 0;
}

/* --------------------------------------------------------------------- */
/* 6. View System Logs                                                   */
/* --------------------------------------------------------------------- */
int viewSystemLogs(int socket_fd) {
    FILE *log = fopen("logs/server.log", "r");
    if (!log) {
        send_response(socket_fd, "No log file found\n");
        return -1;
    }

    char line[512];
    send_response(socket_fd, "=== Server Log ===\n");
    while (fgets(line, sizeof(line), log)) {
        send_response(socket_fd, line);
    }
    send_response(socket_fd, "=== End of Log ===\n");
    fclose(log);
    return 0;
}