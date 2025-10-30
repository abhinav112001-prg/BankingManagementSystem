#ifndef DATABASE_H
#define DATABASE_H
#include "types.h"

static int fetch_and_increment_id(int fd);
Account *find_account_by_user_id(int user_id);
User *find_user_by_id(int id);
int lock_file(const char *filename, int type);
int unlock_file(int fd);

#endif