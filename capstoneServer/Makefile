CC = gcc
CFLAGS = -std=c99 -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: capstoneServer cgi

capstoneServer: capstoneServer.c csapp.o sbuf.o
	$(CC) $(CFLAGS) -o capstoneServer capstoneServer.c csapp.o sbuf.o $(LIB)

csapp.o:
	$(CC) $(CFLAGS) -c csapp.c

sbuf.o: csapp.o
	$(CC) $(CFLAGS) -c sbuf.c csapp.o

cgi:
	(cd cgi-bin; make)

clean:
	rm -f *.o capstoneServer *~
	(cd cgi-bin; make clean)

