CC = gcc
CFLAGS = -I -Wall
DEPS = board.h settings.h minunit.h
OBJS = board.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
	./main

test: test.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
	./test

clean:
	rm *.o main test
