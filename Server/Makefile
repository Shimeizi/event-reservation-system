CC = gcc
CFLAGS = -Wall -g
TARGET = server

OBJS = server.o operations.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

server.o: server.c operations.h
	$(CC) $(CFLAGS) -c server.c

operations.o: operations.c operations.h
	$(CC) $(CFLAGS) -c operations.c

clean:
	rm -f $(TARGET) $(OBJS)
