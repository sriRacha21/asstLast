all: client.c server.c fileIO.c serverRequests.c md5.c fileIO.h serverRequests.h definitions.h md5.h
	gcc -o client client.c fileIO.c serverRequests.c md5.c
	gcc -o server server.c requestUtils.c -pthread

clean:
	rm -f client; rm -f server; rm -f .Manifest;
