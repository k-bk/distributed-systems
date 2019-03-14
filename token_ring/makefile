CC = gcc
OBJECTS = client.o
BINARY = client
FLAGS = -pthread

all: client

client: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(BINARY) $(FLAGS)

test: all
	./client ABC 3001 196.192.20.1 4020 true udp

clean:
	rm -f $(BINARY) $(OBJECTS) log.txt
