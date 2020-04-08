CC = gcc
CFLAGS = -I -Wall -O3
DEPS = board.h solver.h table.h hashing.h settings.h minunit.h
SRCS = board.c solver.c table.c hashing.c
OBJS = $(subst .c,.o,$(SRCS))

default: main

.SUFFIX:
.SUFFIX: .o .c
%.o: %.c $(DEPS) Makefile.deps
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

test: test.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY:
.PHONY: all run_main run_test clean clobber
all: Makefile.deps run_test run_main

run_main: main
	./main

run_test: test
	./test

clean:
	-rm *.o Makefile.deps

clobber: clean
	-rm -f main test

Makefile.deps:
	$(CC) -MM $(SRCS) > $@
