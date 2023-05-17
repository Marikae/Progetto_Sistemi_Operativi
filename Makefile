
CFLAGS=-Wall -std=gnu99
INCLUDES=-I./inc

SERVER_SRCS=src/F4Server.c src/shared_memory.c src/matrixlib.c src/errExit.c
CLIENT_SRCS=src/F4Client.c src/shared_memory.c src/matrixlib.c src/errExit.c

SERVER_OBJS=$(SERVER_SRCS:.c=.o)
CLIENT_OBJS=$(CLIENT_SRCS:.c=.o)

all: server client

server: $(SERVER_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

client: $(CLIENT_OBJS)
	@echo "Making executable: "$@
	@$(CC) $^ -o $@

.c.o:
	@echo "Compiling: "$<
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean

clean:
	@rm -f src/*.o client server
	@echo "Removed object files and executables..."