# TODO
1. server currentversion doesn't return Manifest properly

Client:
```
[as2784@crayon client]$ ./WTF currentversion testFolder
Attempting to read file .configure
Attempting to connect to IP: 127.0.0.1:6970
Successfully connected to server!
Sent request to server: "project:testFolder"
Received from server: "exists"
Project found on filesystem!
^C
```
Server:
```
[as2784@crayon server]$ ./WTFserver 6970
Server successfully started.
Awaiting connection...
Thread successfully started for client socket.
Received "project:testFolder", getting project name then sending whether or not it exists.
Project name: testFolder
Finished verifying existence.
Project exists.
Received "current version:testFolder", fetching and sending...
Project name: testFolder.  Sending project state
Attempting to read file .Manifest
Could not read file .Manifest.
Reached atexit().  Server is now shutting down...
```

2. upgrade not not reading file right

Client:
```
[as2784@crayon client]$ ./WTF upgrade testFolder
Attempting to read file .configure
Attempting to connect to IP: 127.0.0.1:6970
Successfully connected to server!
Sent request to server: "project:testFolder"
Received from server: "exists"
Project found on filesystem!
Attempting to read file testFolder/.Manifest
Attempting to read file testFolder/.Update
Update entry: M testFolder/testSub/asunoyo.txt 1fe838790b38836a2ddfdaac8ba8ce0f
^C
```
Server:
```
Thread successfully started for client socket.
Received "project:testFolder", getting project name then sending whether or not it exists.
Project name: testFolder
Finished verifying existence.
Project exists.
Received "specific project file:testFolder:testFolder/testSub/asunoyo.txt", sending file specs.
Project name: testFolder. File's filepath: testFolder/testSub/asunoyo.txt0V�.
Attempting to read file testFolder/testSub/asunoyo.txt0V�
Could not read file testFolder/testSub/asunoyo.txt0V�.
Reached atexit().  Server is now shutting down...
```