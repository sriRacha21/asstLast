all: client.c server.c fileIO.c serverRequests.c fileIO.h serverRequests.h definitions.h
	gcc -o client client.c fileIO.c serverRequests.c
	gcc -o server server.c -pthread

clean:
	rm -f client; rm -f server; rm -f .Manifest;
