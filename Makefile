CC 			= gcc
CFLAGS 	= --std=c99 -Wall -g -Wno-strict-aliasing -Wno-unused-variable -Wno-unused-function -O3 -fopenmp
LFLAGS 	= -lm

SRC 		= $(wildcard *.c)
OBJ 		= $(patsubst %.c, bin/%.o, $(SRC))
HEADERS = $(wildcard *.h)

DATE 		:= $(shell date "+%Y-%m-%d")

APP 		= raytracer
TESTS 	= raytracer_test

$(APP): bin/main.o bin/raytracer.o raytracer.h
	@mkdir -p bin/
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

$(TESTS): bin/test.o bin/raytracer.o raytracer.h
	@mkdir -p bin/
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

bin/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(APP) $(TESTS)

test: $(TESTS)
	./$(TESTS) | tee tests.log 2>&1

run: all 
	./$(APP) -w 640 -h 480 -s 50 -o "result.png"

pretty: all
	./$(APP) -w 1920 -h 1080 -s 500 -o image-$(DATE).png

memcheck: $(APP)
	valgrind --leak-check=full \
		 --show-leak-kinds=all \
		 --track-origins=yes \
		 --verbose \
		 --log-file="valgrind.log" \
		 ./$(APP) -w 100 -h 50 -s 50 -o "bin/valgrind.png"

perfcheck: CFLAGS += -pg
perfcheck: $(APP)
	./$(APP) -w 320 -h 180 -s 50 -o "bin/gprof.png"
	gprof $(APP) gmon.out > gprof.log 2>&1

clean:
	rm -f $(APP) $(TESTS) *.o *.stackdump *.log *.out bin/*

.PHONY: all clean run memcheck render

