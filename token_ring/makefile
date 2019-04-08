CC = gcc
OBJECTS = client.o
BINARY = client
FLAGS = -pthread

all: client

client: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(BINARY) $(FLAGS)

clean:
	rm -f $(BINARY) $(OBJECTS) *.txt
