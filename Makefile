# Ορίζουμε τις μεταβλητές που θα χρησιμοποιήσουμε
CC = gcc
CFLAGS = -Wall -g

# Ορίζουμε τα targets και τις εξαρτήσεις τους

all: jobCommander jobExecutorServer 

jobCommander: jobCommander.c
	$(CC) $(CFLAGS) -o jobCommander jobCommander.c

jobExecutorServer: jobExecutorServer.c queue.h
	$(CC) $(CFLAGS) -o jobExecutorServer jobExecutorServer.c

# Ορίζουμε καθαρισμό για τα αρχεία object και τα εκτελέσιμα
clean:
	rm -f jobCommander jobExecutorServer
