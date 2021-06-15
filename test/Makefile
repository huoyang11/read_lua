SRC=$(wildcard src/*.c)
SRC_OBJ=$(patsubst %.c,%.o,$(SRC))
OBJ=$(patsubst src%,obj%,$(SRC_OBJ))
SRC_OUT=$(patsubst %.c,%,$(SRC))
TARGET=$(patsubst src/%,%,$(SRC_OUT))

CC=gcc
CFLAGS=-g2 -gdwarf-2

LIBPATH=-L./lib
LIBS=-llua -lm -ldl
INCLUDE=-I./include

all:$(TARGET)

$(TARGET):%:src/%.c
	$(CC) $(CFLAGS) -o bin/$@ $^ $(INCLUDE) $(LIBPATH) $(LIBS)

clean:
	rm -rf $(OBJS) bin/*

test:
	echo $(SRC) && echo $(OBJ)
