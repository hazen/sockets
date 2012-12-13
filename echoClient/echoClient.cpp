// Brain-dead client to echo from a server
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>

#define SOCKET_PROTOCOL "max" // max             6074/tcp    # Microsoft Max

int main(void)
{
    int s, t;
    char str[100];

    struct addrinfo clntHints;
    struct addrinfo* clntResult;

    memset(&clntHints, '\0', sizeof(struct addrinfo));
    clntHints.ai_family = AF_UNSPEC;
    clntHints.ai_socktype = SOCK_STREAM;
    clntHints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, SOCKET_PROTOCOL, &clntHints, &clntResult) < 0) {
        perror("getaddrinfo");
        exit(1);
    }

    if ((s = socket(clntResult->ai_family, clntResult->ai_socktype, clntResult->ai_protocol)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    if (connect(s, clntResult->ai_addr, clntResult->ai_addrlen) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");

    while(printf("> "), fgets(str, 100, stdin), !feof(stdin)) {
        if (send(s, str, strlen(str), 0) == -1) {
            perror("send");
            exit(1);
        }

        if ((t=recv(s, str, 100, 0)) > 0) {
            str[t] = '\0';
            printf("echo> %s", str);
        } else {
            if (t < 0) perror("recv");
            else printf("Server closed connection\n");
            exit(1);
        }
    }

    close(s);

    return 0;
}
