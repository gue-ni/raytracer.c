CC      = gcc
CFLAGS  = --std=c99 -Wall -Wno-strict-aliasing -Wno-unused-variable -Wno-unused-function -fopenmp -O3 -ffast-math
LFLAGS  = -lm

SRC     = $(wildcard *.c)
OBJ     = $(patsubst %.c, bin/%.o, $(SRC))
HEADERS = $(wildcard *.h)

DATE    = $(shell date "+%Y-%m-%d")

PROG    = raytracer
TESTS   = test

$(PROG): obj/main.o obj/raytracer.o
	$(CC) $(CFLAGS) -o bin/$@ $^ $(LFLAGS)

$(TESTS): obj/test.o obj/raytracer.o
	$(CC) $(CFLAGS) -o bin/$@ $^ $(LFLAGS)

obj/%.o: %.c
	@mkdir -p bin/ obj/
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(PROG)

test: $(TESTS)
	./$(TESTS) | tee tests.log 2>&1

run: all 
	./bin/$(PROG) -w 640 -h 480 -s 128 -o "result.png"

render: $(PROG)
	./bin/$(PROG) -w 1920 -h 1080 -s 2048 -o "image-1920x1080-$(DATE).png"

memcheck: $(PROG)
	valgrind --leak-check=full \
		 --show-leak-kinds=all \
		 --track-origins=yes \
		 --verbose \
		 --log-file="valgrind.log" \
		 ./bin/$(PROG) -w 100 -h 50 -s 64 -o "bin/valgrind.png"

perfcheck: CFLAGS += -pg
perfcheck: $(PROG)
	./bin/$(PROG) -w 320 -h 180 -s 64 -o "bin/gprof.png"
	gprof $(PROG) gmon.out > gprof.log 2>&1

clean:
	rm -f $(PROG) $(TESTS) *.o *.stackdump *.log *.out bin/* obj/*

.PHONY: all clean run memcheck render highres perfcheck render

