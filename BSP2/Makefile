CC = /usr/bin/gcc
CFLAGS = -g -Wall
LDFLAGS = -g -pthread -Wl,-V

SRC = main.c monitor.c
OBJ = $(SRC:%.c=%.o)

all: aufgabe2

aufgabe2: $(OBJ)
			$(CC) -o aufgabe2 $(LDFLAGS) $(OBJ)

.PHONY: deps
deps:
		$(CC) -MM $(SRC) > makefile.dependencies
		
clean:
		rm *.o aufgabe2
include makefile.dependencies