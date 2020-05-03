all: client/client.c server/server.c fileIO.c client/serverRequests.c md5.c manifestControl.c server/requestUtils.c server/LLSort.c fileIO.h client/serverRequests.h definitions.h md5.h manifestControl.h server/LLSort.h server/requestUtils.h server/LLSort.h
	mkdir -p client > /dev/null 2>&1
	mkdir -p server > /dev/null 2>&1
	gcc -o client/WTF client/client.c fileIO.c client/serverRequests.c manifestControl.c md5.c server/requestUtils.c server/LLSort.c
	gcc -o server/WTFserver server/server.c server/requestUtils.c fileIO.c server/exitLeaks.c md5.c manifestControl.c server/LLSort.c -pthread

clean:
	rm -f client/WTF; rm -f server/WTFserver;

cleanAll:
	rm -f client/WTF; rm -f server/WTFserver; rm -rf client/testFolder;
