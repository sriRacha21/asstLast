all: client.c server.c fileIO.c serverRequests.c md5.c manifestControl.c requestUtils.c LLSort.c exitLeaks.c fileIO.h serverRequests.h definitions.h md5.h manifestControl.h LLSort.h requestUtils.h LLSort.h exitLeaks.h
	mkdir -p client > /dev/null 2>&1
	mkdir -p server > /dev/null 2>&1
	mkdir -p server/backups > /dev/null 2>&1
	gcc -o WTF client.c fileIO.c serverRequests.c manifestControl.c md5.c requestUtils.c LLSort.c exitLeaks.c -pthread
	gcc -o WTFserver server.c requestUtils.c fileIO.c exitLeaks.c md5.c manifestControl.c LLSort.c -pthread

test: clean all WTFtest.c
	gcc -o WTFtest WTFtest.c -pthread

clean:
	rm -f WTF;
	rm -f WTFserver;
	rm -f WTFtest;
	rm -rf client;
	rm -rf server/backups;
	rm -rf server;

cleanAll:
	rm -f WTF;
	rm -f WTFserver;
	rm -f WTFtest;
	chmod 777 testFolder;
	rm -rf testFolder;
