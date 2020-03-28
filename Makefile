CC = gcc
CFLAGS = -I -Wall -O3
DEPS = board.h solver.h settings.h minunit.h
OBJS = board.o solver.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

test: test.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
	./test

clean:
	rm *.o main test
