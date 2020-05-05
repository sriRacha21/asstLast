all: client.c server.c fileIO.c serverRequests.c md5.c manifestControl.c requestUtils.c LLSort.c exitLeaks.c fileIO.h serverRequests.h definitions.h md5.h manifestControl.h LLSort.h requestUtils.h LLSort.h exitLeaks.h
	mkdir -p client > /dev/null 2>&1
	mkdir -p server > /dev/null 2>&1
	gcc -o client/WTF client.c fileIO.c serverRequests.c manifestControl.c md5.c requestUtils.c LLSort.c exitLeaks.c -pthread
	gcc -o server/WTFserver server.c requestUtils.c fileIO.c exitLeaks.c md5.c manifestControl.c LLSort.c -pthread

clean:
	rm -f client/WTF
	rm -f server/WTFserver;

cleanAll:
	rm -f client/WTF
	rm -f server/WTFserver
	chmod 777 client/testFolder;
	rm -rf client/testFolder;
