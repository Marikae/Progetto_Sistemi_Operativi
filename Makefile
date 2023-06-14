
CFLAGS=-Wall -std=gnu99
INCLUDES=-I./inc

SERVER_SRCS=src/F4Server.c inc/shared_memory.c inc/errExit.c inc/matrixLib.c inc/semaphore.c
CLIENT_SRCS=src/F4Client.c inc/shared_memory.c inc/errExit.c inc/matrixLib.c inc/semaphore.c

SERVER_OBJS=$(SERVER_SRCS:.c=.o)
CLIENT_OBJS=$(CLIENT_SRCS:.c=.o)

all: F4Server F4Client

F4Server: $(SERVER_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

F4Client: $(CLIENT_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

.c.o:
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean

clean:
	@rm -f src/*.o client server
	@echo "Removed object files and executables..."