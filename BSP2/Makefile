CC = /usr/bin/gcc
CFLAGS = -pthread -g -Wall
LDFLAGS = -lpthread

philosopher.o: main.c monitor.c general.h
							$(CC) -c main.c monitor.c general.h
							
OBJ = philosopher.o

philosopher: $(OBJ)
					$(CC) $(CFLAGS) $(LDFLAGS) -o
					philosopher $(OBJ)
%.o: %.c
		$(CC) $(CFLAGS) -c $<
