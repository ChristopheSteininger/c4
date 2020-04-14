CC = gcc
CFLAGS = -I -Wall -O3

SDIR = src
TDIR = tst
ODIR = obj

SRCS = $(wildcard $(SDIR)/*.c)
SOBJ = $(patsubst %.c,$(ODIR)/%.o,$(SRCS))

TSTS = $(wildcard $(TDIR)/*.c)
TOBJ = $(patsubst %.c,$(ODIR)/%.o,$(TSTS))

default: c4

.SUFFIX:
.SUFFIX: .o .c

$(ODIR)/%.o: %.c $(ODIR)/Makefile.deps
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

c4: $(SOBJ)
	$(CC) -o $@ $^ $(CFLAGS)

test: $(patsubst $(ODIR)/$(SDIR)/c4.o,,$(SOBJ)) $(TOBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY:
.PHONY: all run_main run_test clean clobber
all: Makefile.deps run_test run_main

runc4: c4
	./c4

runtest: test
	./test

clean:
	-rm -rf $(ODIR)

clobber: clean
	-rm -f c4 test

$(ODIR)/Makefile.deps:
	@mkdir -p $(@D)
	$(CC) -MM $(SRCS) $(TSTS) > $@
