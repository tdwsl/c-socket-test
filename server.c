/* sockets test program - server */

#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#define MAX_CONN 300

struct conn {
    int fd;
    char name[20];
};

int g_serverfd;
struct conn *g_connections[MAX_CONN] = {0};
int g_sem;

char*
split(str)
char *str;
{
    char *s;
    char p;

    for(s = str; *s > ' '; s++);
    p = *s;
    *s = 0;
    if(!p) return s;

    for(s++; *s <= ' '; s++);
    return s;
}

void
limit(s, max)
char *s;
int max;
{
    if(strlen(s) > max)
        s[max] = 0;
}

void
lowercase(s)
char *s;
{
    int i;
    for(i = 0; s[i]; i++)
        if(s[i] >= 'A' && s[i] <= 'Z')
            s[i] += 'a'-'A';
}

void
say(msg)
char *msg;
{
    int i;
    for(i = 0; i < MAX_CONN; i++)
        if(g_connections[i])
            write(g_connections[i]->fd, msg, strlen(msg)+1);
    write(fileno(stdout), msg, strlen(msg));
}

void
trim(s)
char *s;
{
    int i;
    for(i = 0; s[i]; i++)
        if(s[i] == '\n' || s[i] == '\r') s[i] = 0;
}

void
changeName(conn, name)
struct conn *conn;
char *name;
{
    int i;
    char buf[120];
    const char *sl;

    if(!strcmp(conn->name, name)) {
        sl = "You are already called that!\n";
        write(conn->fd, sl, strlen(sl)+1);
        return;
    }

    if(strstr(name, "newbie") == name) {
        sl = "Only the server gets to name newbies.\n";
        write(conn->fd, sl, strlen(sl)+1);
        return;
    }

    for(i = 0; i < MAX_CONN; i++)
        if(g_connections[i])
            if(!strcmp(g_connections[i]->name, name)) {
                sl = "That name is already taken.\n";
                write(conn->fd, sl, strlen(sl)+1);
                return;
            }

    sprintf(buf, "%s changed their name to %s.\n", conn->name, name);
    strcpy(conn->name, name);
    say(buf);
}

void*
handleClient(vfd)
void *vfd;
{
    struct conn *conn;
    int i, coni;
    char name[20];
    char recv[200];
    char buf[300];
    char *comm, *a1, *a2, *a3;
    const char *sl;

    conn = malloc(sizeof(struct conn));
    conn->fd = (int)vfd;

    while(g_sem);
    g_sem++;
    for(i = 0; i < MAX_CONN; i++)
        if(!g_connections[i])
            if(--g_sem == 0) break;
    g_connections[i] = conn;
    coni = i;

    sprintf(conn->name, "newbie%d", i+1);

    for(;;) {
        bzero(recv, 200);
        read(conn->fd, recv, 200);
        trim(recv);
        comm = recv;
        a1 = split(comm);
        lowercase(comm);

        if(!(*comm)) continue;

        if(!strcmp(comm, "name")) {
            if(a1[0]) {
                limit(a1, 19);
                changeName(conn, a1);
            } else {
                sl = "You can't be called nothing.\n";
                write(conn->fd, sl, strlen(sl)+1);
            }
        } else if(!strcmp(comm, "hello")) {
            sprintf(buf, "Hello, %s!\n", conn->name);
            write(conn->fd, buf, strlen(buf)+1);
        } else if(!strcmp(comm, "say")) {
            limit(a1, 170);
            sprintf(buf, "%s: %s\n", conn->name, a1);
            say(buf);
        } else if(!strcmp(comm, "look")) {
            sprintf(buf, "You don't see anything interesting.\n");
            write(conn->fd, buf, strlen(buf)+1);
        } else if(!strcmp(comm, "quit")) {
            sl = "Bye-bye!\n";
            write(conn->fd, sl, strlen(sl)+1);
            break;
        } else if(!strcmp(comm, "help")) {
            sl = "Commands are:\nname, hello, look, say, help, quit\n";
            write(conn->fd, sl, strlen(sl)+1);
        } else {
            sprintf(buf, "You can't %s.\n", comm);
            write(conn->fd, buf, strlen(buf)+1);
        }
    }

    sprintf(buf, "%s left.\n", conn->name);
    g_connections[coni] = 0;
    close(conn->fd);
    free(conn);
    say(buf);
    return 0;
}

int
main()
{
    int port, sockfd, r, opt;
    struct sockaddr_in addr;
    pthread_t thrid;
    int addrlen = sizeof(struct sockaddr_in);

    g_serverfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(g_serverfd);

    r = setsockopt(
        g_serverfd,
        SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
        &opt,
        sizeof(int));
    assert(!r);

    srand(time(0));
    port = 3000 + rand()%2500;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    r = bind(
        g_serverfd,
        (struct sockaddr*)&addr,
        sizeof(struct sockaddr_in));
    assert(r >= 0);

    printf("Port is: %d\n", port);

    for(;;) {
        if(listen(g_serverfd, 3) < 0)
            continue;

        sockfd = accept(
            g_serverfd,
            (struct sockaddr*)&addr,
            (socklen_t*)&addrlen);
        if(sockfd < 0)
            continue;

        pthread_create(&thrid, NULL, handleClient, (void*)sockfd);
    }

    return 0;
}
