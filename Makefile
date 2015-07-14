CC=gcc
CFLAGS=-c -g -Wall -std=gnu1x -O3 -I/opt/local/include
LDLIBS=-L/opt/local/lib -lm -lpng -pthread
SOURCES=$(wildcard *.c)\
        $(wildcard */*.c)
OBJECTS=$(SOURCES:%.c=%.o)
EXECUTABLE=GLTC

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
