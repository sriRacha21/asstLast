all: client.c server.c fileIO.c serverRequests.c md5.c manifestControl.c fileIO.h serverRequests.h definitions.h md5.h manifestControl.h
	gcc -o client client.c fileIO.c serverRequests.c manifestControl.c md5.c
	gcc -o server server.c requestUtils.c fileIO.c exitLeaks.c md5.c manifestControl.c -pthread

clean:
	rm -f client; rm -f server; rm -f .Manifest;
