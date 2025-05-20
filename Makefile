CC = gcc
CFLAGS = -Wall -Iinclude
SRC = src/protocol.c

all: server client_sender client_receiver

server: server/main.c $(SRC)
	$(CC) $(CFLAGS) -o server/server server/main.c $(SRC)

client_sender: client_sender/main.c $(SRC)
	$(CC) $(CFLAGS) -o client_sender/client_sender client_sender/main.c $(SRC)

client_receiver: client_receiver/main.c $(SRC)
	$(CC) $(CFLAGS) -o client_receiver/client_receiver client_receiver/main.c $(SRC)

clean:
	rm -f server/server client_sender/client_sender client_receiver/client_receiver
