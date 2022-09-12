CC = gcc
CFLAGS = --std=c99 -O3 -Wall 
LFLAGS = -lm

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))
HEADERS = $(wildcard *.h)

PROGRAM = raytracer


$(PROGRAM): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS) $(HEADERS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(PROGRAM)

run: all 
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o

.PHONY: all clean run

