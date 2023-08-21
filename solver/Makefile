CC = g++
CFLAGS = -I -Wall -O3 -std=c++20

SRC_DIR = src
TST_DIR = tst
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
SRC_OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

TSTS = $(wildcard $(TST_DIR)/*.cpp)
TST_OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TSTS))

default: c4

.SUFFIX:
.SUFFIX: .o .cpp

$(OBJ_DIR)/%.o: %.cpp $(OBJ_DIR)/Makefile.deps
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

c4: $(SRC_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

test: $(patsubst $(OBJ_DIR)/$(SRC_DIR)/c4.o,,$(SRC_OBJ)) $(TST_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY:
.PHONY: all runc4 runtest clean clobber
all: c4 test

runc4: c4
	./c4

runtest: test
	./test

clean:
	-rm -rf $(OBJ_DIR)

clobber: clean
	-rm -f c4 test

$(OBJ_DIR)/Makefile.deps:
	@mkdir -p $(@D)
	$(CC) -MM $(SRCS) $(TSTS) > $@
