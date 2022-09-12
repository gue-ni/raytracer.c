CC = gcc
CFLAGS = --std=c99 -O3 -Wall 
LFLAGS = -lm

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
HEADERS = $(wildcard *.h)

PROGRAM = raytracer
TESTS = raytracer_test


$(PROGRAM): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

$(TESTS): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(PROGRAM)

test: CFLAGS += -DRUN_TESTS
test: $(TESTS)
	./$(TESTS)

run: all 
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o

.PHONY: all clean run

