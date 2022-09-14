CC = gcc
CFLAGS = --std=c99 -Wall -O3 -Wno-strict-aliasing
LFLAGS = -lm

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
HEADERS = $(wildcard *.h)

APP = raytracer
TESTS = raytracer_test

$(APP): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

$(TESTS): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

all: $(APP)

test: CFLAGS += -DRUN_TESTS
test: $(TESTS)
	./$(TESTS)

run: all 
	./$(APP)

clean:
	rm -f $(APP) $(TESTS) *.o *.stackdump

.PHONY: all clean run

