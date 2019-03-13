CC = gcc
OBJECTS = client.o
BINARY = client
FLAGS = -pthread

all: client

client: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(BINARY) $(FLAGS)

test: all
	./client XYZ 2001 200.0.0.1 4000 false tcp
	./client ABC 3001 196.192.20.1 4020 true udp

clean:
	rm -f $(BINARY) $(OBJECTS)
