CC = gcc
CFLAGS = -Wall -g -std=c99 -pedantic

all: schedule two

schedule: schedule.o
	$(CC) $(CFLAGS) -o schedule schedule.o

schedule.o: schedule.c
	$(CC) $(CFLAGS) -c schedule.c

two: two.o
	$(CC) $(CFLAGS) -o two two.o

two.o: two.c
	$(CC) $(CFLAGS) -c two.c

clean:
	rm -f schedule two *.o