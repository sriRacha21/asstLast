// standard imports
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// networking imports
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
// errors
#include <errno.h>

// definitions
#define DEBUG 1
#define PORT 8080

int main( int argc, char** argv ) {
    // construct a new socket
    int sfd = socket( AF_INET, SOCK_STREAM, SOCK_NONBLOCK );
    // create struct to use with bind
    struct sockaddr_in addr;
    memset( &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);
    // reserve a port for the socket
    int bindError = bind( sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );
    // listen on a port accept 100 requests before starting to refuse
    listen( sfd , 100 );
}