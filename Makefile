CC=clang
CFLAGS=-Wall -Werror -pedantic -pipe

all:
	$(CC) $(CFLAGS) project3.c -o project3
clean:
	rm -fr *.o project3
