SRC=$(wildcard src/*.c)
SRC_OBJ=$(patsubst %.c,%.o,$(SRC))
OBJ=$(patsubst src%,obj%,$(SRC_OBJ))
SRC_OUT=$(patsubst %.c,%,$(SRC))
TARGET=$(patsubst src/%,%,$(SRC_OUT))

LIBSRC=$(wildcard libsrc/*.c)
LIBSRC_OUT=$(patsubst %.c,%,$(LIBSRC))
LIBTARGET=$(patsubst libsrc/%,%.so,$(LIBSRC_OUT))

LIBSRCP=$(wildcard libsrc/*.cpp)
LIBSRCP_OUT=$(patsubst %.cpp,%,$(LIBSRCP))
LIBTARGETP=$(patsubst libsrc/%,%.so,$(LIBSRCP_OUT))

CC=gcc
CXX=g++
CFLAGS=-g2 -gdwarf-2

LIBPATH=-L./lib
LIBS=-l:liblua.a -lm -ldl
INCLUDE=-I./include

all:$(TARGET) $(LIBTARGET) $(LIBTARGETP)

$(TARGET):%:src/%.c
	$(CC) $(CFLAGS) -o bin/$@ $^ $(INCLUDE) $(LIBPATH) $(LIBS)

$(LIBTARGET):%.so:libsrc/%.c
	$(CC) $(CFLAGS) -shared -o lib/$@ $^ $(INCLUDE) $(LIBPATH) -llua

$(LIBTARGETP):%.so:libsrc/%.cpp
	$(CXX) $(CFLAGS) -shared -o lib/$@ $^ $(INCLUDE) $(LIBPATH) -llua

clean:
	rm -rf $(OBJS) bin/*

test:
	@echo $(SRC) && echo $(OBJ)
	@echo $(LIBTARGET)
