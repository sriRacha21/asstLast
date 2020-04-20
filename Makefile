all: client.c server.c fileIO.c fileIO.h definitions.h
	gcc -o client client.c fileIO.c
	gcc -o server server.c

clean:
	rm -f client; rm -f server; rm -f .Manifest;
