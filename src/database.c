#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "types.h"

static Account account_buffer;
static User user_buffer;
static int fetch_and_increment_id(int fd) {
    LoanHeader header;
    ssize_t bytes_read = read(fd, &header, sizeof(LoanHeader));
    if (bytes_read != sizeof(LoanHeader)) {
        // Handle error (file empty or corrupt)
        header.next_id = 1;
        header.record_count = 0;
    }
    int id = header.next_id;
    
    // Updation
    header.next_id++;
    header.record_count++;
    lseek(fd, 0, SEEK_SET);
    write(fd, &header, sizeof(LoanHeader));
    fsync(fd);
    return id;
}
Account *find_account_by_user_id(int user_id) {
    int fd = open("accounts.dat", O_RDONLY);
    if (fd == -1) return NULL;

    struct flock lock = { 
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET, 
        .l_start = 0,
        .l_len = 0 
    };
    fcntl(fd, F_SETLKW, &lock);

    AccountHeader header;
    read(fd, &header, sizeof(AccountHeader));
    lseek(fd, sizeof(AccountHeader), SEEK_SET);

    Account account;
    while (read(fd, &account, sizeof(Account)) == sizeof(Account)) {
        if (account.userID == user_id) {
            account_buffer = account;
            unlock_file(fd);
            close(fd);
            return &account_buffer;
        }
    }
    unlock_file(fd);
    close(fd);
    return NULL;
}
User *find_user_by_id(int id) {
    int fd = lock_file("users.dat", F_RDLCK);
    if (fd == -1) return NULL;
    lseek(fd, sizeof(UserHeader), SEEK_SET);
    User user;
    while (read(fd, &user, sizeof(User)) == sizeof(User)) {
        if (user.id == id) {
            user_buffer = user;
            unlock_file(fd);
            return &user_buffer;
        }
    }
    unlock_file(fd);
    return NULL;
}

int lock_file(const char *filename, int type) {
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) return -1;

    struct flock lock = { .l_type = type, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0 };
    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        close(fd);
        return -1;
    }
    return fd;
}

int unlock_file(int fd) {
    struct flock lock = { .l_type = F_UNLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 0 };
    int result = fcntl(fd, F_SETLK, &lock);
    close(fd);
    return result;
}
