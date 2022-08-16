/* sockets test program - client (telnet works too) */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

void*
handleOutput(vfd)
void *vfd;
{
    int serverfd = (int)vfd;
    char buf[700];

    for(;;) {
        bzero(buf, 700);
        read(serverfd, buf, 700);
        write(fileno(stdout), buf, strlen(buf));
    }
}

int
main(argc, args)
int argc;
char **args;
{
    int r, serverfd, sockfd;
    struct sockaddr_in addr;
    pthread_t thrid;
    char buf[200];
    int err;
    int errlen = sizeof(int);

    assert(argc == 2);

    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(serverfd >= 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(args[1]));

    r = inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    assert(r >= 0);

    sockfd = connect(
        serverfd,
        (struct sockaddr*)&addr,
        sizeof(struct sockaddr_in));
    assert(sockfd >= 0);

    pthread_create(&thrid, NULL, handleOutput, (void*)serverfd);

    do {
        bzero(buf, 200);
        read(fileno(stdin), buf, 199);
        write(serverfd, buf, strlen(buf));
    } while(write(serverfd, 0, 0) != -1);

    pthread_kill(thrid, SIGINT);

    close(sockfd);
    return 0;
}
