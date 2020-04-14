CC = gcc
CFLAGS = -I -Wall -O3
DEPS = board.h solver.h table.h hashing.h test/known_states.h settings.h test/minunit.h
SRCS = board.c solver.c table.c hashing.c
OBJS = $(subst .c,.o,$(SRCS))

default: main

.SUFFIX:
.SUFFIX: .o .c
%.o: %.c $(DEPS) Makefile.deps
	$(CC) -c -o $@ $< $(CFLAGS)

main: main.o $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

test: test/test.o $(OBJS) test/known_states.c
	$(CC) -o test/$@ $^ $(CFLAGS)

.PHONY:
.PHONY: all run_main run_test clean clobber
all: Makefile.deps run_test run_main

runmain: main
	./main

runtest: test
	test/test

clean:
	-rm *.o test/*.o Makefile.deps

clobber: clean
	-rm -f main test/test

Makefile.deps:
	$(CC) -MM $(SRCS) > $@
