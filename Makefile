
CC = gcc
CFLAGS =
LFLAGS = -lm

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

PROGRAM = raytracer

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<



run:
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM) *.o 

.PHONY: all clean run


