all: client.c fileIO.c fileIO.h definitions.h
	gcc -o client client.c fileIO.c

clean:
	rm -f client;