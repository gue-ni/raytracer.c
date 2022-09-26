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

test: $(TESTS)
	./$(TESTS) > tests.log 2>&1

run: all 
	./$(APP) -w 640 -h 480 -o "result.png"

memcheck:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file="valgrind.log" ./$(APP) -w 100 -h 50 -o "result-valgrind.png"

perfcheck: CFLAGS += -pg
perfcheck: $(APP)
	gprof $(APP) > gprof.log 2>&1

clean:
	rm -f $(APP) $(TESTS) *.o *.stackdump

.PHONY: all clean run memcheck

