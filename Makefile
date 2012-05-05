CC=clang

all:
	$(CC) -Wall -pipe project3.c -o project3
clean:
	rm -fr *.o project3
