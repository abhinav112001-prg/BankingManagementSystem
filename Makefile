CC = gcc
CFLAGS = -pthread -Iinclude
SRCS = src/server.c src/database.c src/helpers.c src/customer.c src/employee.c src/admin.c src/transactions.c
CLIENT = src/client.c src/helpers.c

all: server client

server: $(SRCS)
	$(CC) $(CFLAGS) -o server $(SRCS)

client: $(CLIENT)
	$(CC) $(CFLAGS) -o client $(CLIENT)

clean:
	rm -f server client logs/server.log
	rm -f server client data/*.dat logs/server.log

.PHONY: all clean