CC = g++
CFLAGS = -I -Wall -std=c++20

SRC_DIR = src
TST_DIR = tst
OBJ_DIR = obj

SRCS = $(shell find $(SRC_DIR) -name "*.cpp")
SRC_OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

TSTS = $(wildcard $(TST_DIR)/*.cpp)
TST_OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(TSTS))


ifeq ($(debug), yes)
	CFLAGS += -g
else ifeq ($(optimise), yes)
	CFLAGS += -O3 -flto=full -DNDEBUG
else
	CFLAGS += -O3 -flto=full
endif


default: all

.SUFFIX:
.SUFFIX: .o .cpp

$(OBJ_DIR)/%.o: %.cpp $(OBJ_DIR)/Makefile.deps
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

c4: $(filter-out %/main.o, $(SRC_OBJ))
	$(CC) -o $@ $^ $(CFLAGS)

arena: $(filter-out %/c4.o, $(SRC_OBJ))
	$(CC) -o $@ $^ $(CFLAGS)

test: $(filter-out %/c4.o %/main.o, $(SRC_OBJ)) $(TST_OBJ)
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
	-rm -f c4 arena test

$(OBJ_DIR)/Makefile.deps:
	@mkdir -p $(@D)
	$(CC) -MM $(SRCS) $(TSTS) > $@
