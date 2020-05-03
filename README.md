# WTF (**W**here's **T**he **F**ile)
## Setup
* Clone the source code and run `make` in the resulting directory.
* New binaries named `WTF` and `WTFserver` should have been created.
## Usage
### Server Program: `WTFserver`
* Run with `./WTFserver <port>`
* Running this binary with a port will bind the server to that port. Ensure this runs in the background of some machine you want to host the version control server on.
### Client Program: `WTF`
* `./WTF configure <IP> <port>`
    * Configure the client to connect to the IP (this is the remote IP of the machine the server is running on)
        * You can also run the server locally and connect to `localhost`
    * Enter the port that the server binded to.
* `./WTF checkout <project name>`
    * Download the full project from the server locally.
* `./WTF update <project name>`
    * Check for differences between server version of project and local version.
* `./WTF upgrade <project name>`
    * Upgrade project to server version if local version is behind.
* `./WTF commit <project name>`
    * Check for differences between local and server version of project and send differences to server.
* `./WTF push <project name>`
    * Upgrade server version of project to local version.
* `./WTF create <project name>`
    * Initialize the project on the server.
* `./WTF destroy <project name>`
    * Delete all project files on the server.
* `./WTF add <project name> <filename>`
    * Add file to the manifest to be tracked.
* `./WTF remove <project name> <filename>`
    * Remove file from the manifest to untrack it.
* `./WTF currentversion <project name>`
    * Gets the state of the project from the server.
* `./WTF history <project name>`
    * Get a history of all operations performed on all successful pushes since project creation.
* `./WTF rollback <project name> <version>`
    * Delete all versions after the specified version on the server. Rollback project version to version of the project specified.