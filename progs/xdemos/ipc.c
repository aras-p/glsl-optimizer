/* Copyright (c) 2003 Tungsten Graphics, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files ("the
 * Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:  The above copyright notice, the Tungsten
 * Graphics splash screen, and this permission notice shall be included
 * in all copies or substantial portions of the Software.  THE SOFTWARE
 * IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Simple IPC API
 * Brian Paul
 */


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include "ipc.h"

#if defined(IRIX) || defined(irix)
typedef int socklen_t;
#endif

#define NO_DELAY 1

#define DEFAULT_MASTER_PORT 7011


/*
 * Return my hostname in <nameOut>.
 * Return 1 for success, 0 for error.
 */
int
MyHostName(char *nameOut, int maxNameLength)
{
    int k = gethostname(nameOut, maxNameLength);
    return k==0;
}


/*
 * Create a socket attached to a port.  Later, we can call AcceptConnection
 * on the socket returned from this function.
 * Return the new socket number or -1 if error.
 */
int
CreatePort(int *port)
{
    char hostname[1000];
    struct sockaddr_in servaddr;
    struct hostent *hp;
    int so_reuseaddr = 1;
    int tcp_nodelay = 1;
    int sock, k;

    /* create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock > 2);

    /* get my host name */
    k = gethostname(hostname, 1000);
    assert(k == 0);

    /* get hostent info */
    hp = gethostbyname(hostname);
    assert(hp);

    /* initialize the servaddr struct */
    memset(&servaddr, 0, sizeof(servaddr) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((unsigned short) (*port));
    memcpy((char *) &servaddr.sin_addr, hp->h_addr,
	   sizeof(servaddr.sin_addr));

    /* deallocate when we exit */
    k = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &so_reuseaddr, sizeof(so_reuseaddr));
    assert(k==0);

    /* send packets immediately */
#if NO_DELAY
    k = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *) &tcp_nodelay, sizeof(tcp_nodelay));
    assert(k==0);
#endif

    if (*port == 0)
        *port = DEFAULT_MASTER_PORT;

    k = 1;
    while (k && (*port < 65534)) {
    	/* bind our address to the socket */
    	servaddr.sin_port = htons((unsigned short) (*port));
    	k = bind(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));
        if (k)
           *port = *port + 1;
    }

#if 0
    printf("###### Real Port: %d\n", *port);
#endif

    /* listen for connections */
    k = listen(sock, 100);
    assert(k == 0);

    return sock;
}


/*
 * Accept a connection on the named socket.
 * Return a new socket for the new connection, or -1 if error.
 */
int
AcceptConnection(int socket)
{
    struct sockaddr addr;
    socklen_t addrLen;
    int newSock;

    addrLen = sizeof(addr);
    newSock = accept(socket, &addr, &addrLen);
    if (newSock == 1)
	return -1;
    else
	return newSock;
}


/*
 * Contact the server running on the given host on the named port.
 * Return socket number or -1 if error.
 */
int
Connect(const char *hostname, int port)
{
    struct sockaddr_in servaddr;
    struct hostent *hp;
    int sock, k;
    int tcp_nodelay = 1;

    assert(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    hp = gethostbyname(hostname);
    assert(hp);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((unsigned short) port);
    memcpy((char *) &servaddr.sin_addr, hp->h_addr, sizeof(servaddr.sin_addr));

    k = connect(sock, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (k != 0) {
       perror("Connect:");
       return -1;
    }

#if NO_DELAY
    /* send packets immediately */
    k = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *) &tcp_nodelay, sizeof(tcp_nodelay));
    assert(k==0);
#endif

    return sock;
}


void
CloseSocket(int socket)
{
    close(socket);
}


int
SendData(int socket, const void *data, int bytes)
{
    int sent = 0;
    int b;

    while (sent < bytes) {
        b = write(socket, (char *) data + sent, bytes - sent);
        if (b <= 0)
            return -1; /* something broke */
        sent += b;
    }
    return sent;
}


int
ReceiveData(int socket, void *data, int bytes)
{
    int received = 0, b;

    while (received < bytes) {
        b = read(socket, (char *) data + received, bytes - received);
        if (b <= 0)
            return -1;
        received += b;
    }
    return received;
}


int
SendString(int socket, const char *str)
{
    const int len = strlen(str);
    int sent, b;

    /* first, send a 4-byte length indicator */
    b = write(socket, &len, sizeof(len));
    if (b <= 0)
	return -1;

    sent = SendData(socket, str, len);
    assert(sent == len);
    return sent;
}


int
ReceiveString(int socket, char *str, int maxLen)
{
    int len, received, b;

    /* first, read 4 bytes to see how long of string to receive */
    b = read(socket, &len, sizeof(len));
    if (b <= 0)
	return -1;

    assert(len <= maxLen);  /* XXX fix someday */
    assert(len >= 0);
    received = ReceiveData(socket, str, len);
    assert(received != -1);
    assert(received == len);
    str[len] = 0;
    return received;
}
