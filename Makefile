CC 		= gcc
CFLAGS 	= --std=c99 -Wall -g -Wno-strict-aliasing
LFLAGS 	= -lm

SRC 	= $(wildcard *.c)
OBJ 	= $(patsubst %.c, %.o, $(SRC))
HEADERS = $(wildcard *.h)

APP 	= raytracer
TESTS 	= raytracer_test

$(APP): main.o raytracer.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TESTS): test.o raytracer.o
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(APP)

test: CFLAGS += -DRUN_TESTS
test: $(TESTS)
	./$(TESTS)

run: all 
	./$(APP)

clean:
	rm -f $(APP) $(TESTS) *.o *.stackdump

.PHONY: all clean run

