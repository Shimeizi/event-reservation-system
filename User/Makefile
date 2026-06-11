CC = gcc
CFLAGS = -Wall -g

all: user

user: user.o user_operations.o
	$(CC) $(CFLAGS) -o user user.o user_operations.o

user.o: user.c user_operations.h
	$(CC) $(CFLAGS) -c user.c

operations.o: operations.c user_operations.h
	$(CC) $(CFLAGS) -c user_operations.c

clean:
	rm -f *.o user
