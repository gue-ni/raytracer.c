CC      = gcc
CFLAGS  = --std=c99 -Wall -Wno-strict-aliasing -Wno-unused-variable -Wno-unused-function -fopenmp -O3
LFLAGS  = -lm

SRC     = $(wildcard *.c)
OBJ     = $(patsubst %.c, bin/%.o, $(SRC))
HEADERS = $(wildcard *.h)

DATE    		= $(shell date "+%Y-%m-%d")
TIMESTAMP 	=	$(shell date +%s)
COMMITHASH	=	$(shell git log -1 --pretty=format:%h)

WIDTH 	= 640
HEIGHT 	= 380
SAMPLES = 32

PROG    = raytracer
TESTS   = raytracer_test
COL			= col

$(PROG): obj/main.o obj/raytracer.o
	$(CC) $(CFLAGS) -o bin/$@ $^ $(LFLAGS)

$(TESTS): obj/test.o obj/raytracer.o
	$(CC) $(CFLAGS) -o bin/$@ $^ $(LFLAGS)

$(COL): obj/collision.o
	$(CC) $(CFLAGS) -o bin/$@ $^ $(LFLAGS)

obj/%.o: %.c
	@mkdir -p bin/ obj/
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(PROG) $(TESTS)

test: $(TESTS)
	./bin/$(TESTS) | tee tests.log 2>&1

run: all 
	./bin/$(PROG) -w 320 -h 180 -s 128 -o "result.png"

render: $(PROG)
	@mkdir -p results/$(DATE)
	echo "Rendering $(WIDTH)x$(HEIGHT) with $(SAMPLES) samples"
	./bin/$(PROG) -w $(WIDTH) -h $(HEIGHT) -s $(SAMPLES) \
		-o "results/$(DATE)/image-$(WIDTH)x$(HEIGHT)-s$(SAMPLES)-$(TIMESTAMP)-$(COMMITHASH).png"

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

