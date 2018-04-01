CC = gcc
CFLAGS = -O2 -Wall -I ..

all: adder subtract multiply divide

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c

subtract: subtract.c
	$(CC) $(CFLAGS) -o subtract subtract.c

multiply: multiply.c
	$(CC) $(CFLAGS) -o multiply multiply.c

divide: divide.c
	$(CC) $(CFLAGS) -o divide divide.c




clean:
	rm -f adder subtract multiply divide *~
