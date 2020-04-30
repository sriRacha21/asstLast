all: client/client.c server/server.c fileIO.c client/serverRequests.c md5.c manifestControl.c fileIO.h client/serverRequests.h definitions.h md5.h manifestControl.h
	gcc -o WTFclient client/client.c fileIO.c client/serverRequests.c manifestControl.c md5.c
	gcc -o WTFserver server/server.c server/requestUtils.c fileIO.c server/exitLeaks.c md5.c manifestControl.c -pthread

clean:
	rm -f WTFclient; rm -f WTFserver;