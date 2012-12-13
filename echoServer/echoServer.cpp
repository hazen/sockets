// Brain-dead server to respond with whatever is thrown at it (serially)
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>

#define SOCKET_PROTOCOL "max" // max             6074/tcp    # Microsoft Max

using namespace std;

void sigchld_handler(int ignored) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void)
{
    int s, s2;
    char str[100];

    char hostname[128];
    gethostname(hostname, sizeof(hostname));
    cout << "I am " << hostname << endl;

    struct addrinfo hints, *res, *info;
    char *cause;
    int error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ALL;
    error = getaddrinfo("minimus.local", "aol", &hints, &info);
    if (error) {
        error = getaddrinfo("rdspc1.stp.ime.reuters.com", "ssh", &hints, &info);
    }
    if (error) {
        perror("getaddrinfo");
        exit(1);
    }

    for (res = info; res; res = res->ai_next) {
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s < 0) {
            cause = "socket";
            continue;
        }

        if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
            cause = "connect";
            close(s);
            s = -1;
            continue;
        }

        stringstream address;
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        if (getnameinfo(res->ai_addr, res->ai_addr->sa_len, hbuf, sizeof(hbuf), sbuf,
//            sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV)) {
            sizeof(sbuf), NI_NUMERICHOST)) {
                perror("could not get numeric hostname");
                exit(1);
        }

        cout << "Found: " << sbuf << ": " << hbuf << endl;
        break;  /* okay we got one */
    }
    if (s < 0) {
        fprintf(stderr, "err %s", cause);
        exit(1);
    }

    freeaddrinfo(info);

    // Now on with the server!
    struct addrinfo srvHints;
    struct addrinfo* srvResult;

    memset(&srvHints, '\0', sizeof(struct addrinfo));
    srvHints.ai_family = AF_UNSPEC;
    srvHints.ai_socktype = SOCK_STREAM;
    srvHints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, SOCKET_PROTOCOL, &srvHints, &srvResult) < 0) {
        perror("getaddrinfo");
        exit(1);
    }

    if ((s = socket(srvResult->ai_family, srvResult->ai_socktype, srvResult->ai_protocol)) == -1) {
        perror("socket");
        exit(1);
    }
    int on = 1;
    printf("setsockopt(SO_REUSEADDR)\n");
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(s, srvResult->ai_addr, srvResult->ai_addrlen) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(s, 5) == -1) {
        perror("listen");
        exit(1);
    }

    for(;;) {
        socklen_t t;
        int done, n;
        struct sockaddr remote;
        printf("Waiting for a connection...\n");
        t = sizeof(remote);
        if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
            perror("accept");
            exit(1);
        }

        printf("Connected.\n");

        done = 0;
        do {
            n = recv(s2, str, 100, 0);
            if (n <= 0) {
                if (n < 0) perror("recv");
                done = 1;
            }
            str[n] = '\0';
            printf("echo> %s", str);

            if (!done)
                if (send(s2, str, n, 0) < 0) {
                    perror("send");
                    done = 1;
                }
        } while (!done);

        close(s2);
    }

    freeaddrinfo(srvResult);
    return 0;
}
