CC = g++
CFLAGS = -I -Wall -O3 -std=c++17

SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
SRC_OBJ = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

default: arena

.SUFFIX:
.SUFFIX: .o .cpp

$(OBJ_DIR)/%.o: %.cpp $(OBJ_DIR)/Makefile.deps
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)

arena: $(SRC_OBJ) ../solver/obj/src/*.o
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY:
.PHONY: all clean clobber
all: arena

clean:
	-rm -rf $(OBJ_DIR)

clobber: clean
	-rm -f arena

$(OBJ_DIR)/Makefile.deps:
	@mkdir -p $(@D)
	$(CC) -MM $(SRCS) $(TSTS) > $@
