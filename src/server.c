#include<assert.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include<stdbool.h>

const size_t max_msg = 4096;

static void err_msg(char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n",err,msg);
    abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {0};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

    if (n < 0) {
        err_msg("read() error. \n");
        return;
    }
    printf("Client says: %s\n",rbuf);

    char wbuf[] = "world";
    write(connfd,wbuf, strlen(wbuf));
}

static int32_t read_full(int fd, char *buf, size_t n){
    while(n > 0) {
        ssize_t rv = read(fd,buf,n);
        if (rv <= 0)
            return -1;
        assert((size_t)rv <= n);
        n -=(size_t)rv;
        buf += rv;
    }
    return 0;
}


static int32_t write_all(int fd, const char *buf,size_t n) {
    while(n>0){
        ssize_t rv = write(fd,buf,n);
        if (rv <=0){
            return -1;
        }
        assert((size_t)rv <= n);
        n -=(size_t)rv;
        buf += rv;
    }
    return 0;
}
// Deals with multiple client requests comming from one connection
// Will handle communication protocol
//
static int32_t handle_request(int connfd){
    //4 bytes header
    char rbuf[4 + max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd,rbuf,4);
    if (err) {
        if (errno == 0)
            err_msg("EOF");
        else
            err_msg("read() error");
        return err;
    }

    //get the message header which is 4bytes long.
    uint32_t len = 0;
    memcpy(&len,rbuf,4);
    if (len > max_msg) {
        err_msg("message too long.");
        return -1;
    }

    //request body of the message
    err = read_full(connfd,&rbuf[4],len);
    if (err) {
        err_msg("read() - failed to read message body.");
        return err;
    }

    //do something
    rbuf[4 + len] = '\0';
    printf("client says: %s\n",&rbuf[4]);

    //reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    //write header
    memcpy(wbuf,&len,4);
    //write body
    memcpy(&wbuf[4],reply,len);
    return write_all(connfd,wbuf,4+len);
}

/**
 * main function for the server.
 *
 */
int main() {
    printf("Starting server. \n");
    int fd = socket(AF_INET,SOCK_STREAM,0);

    if (fd < 0)
        die("Could not create a socket");

    //SO_REUSEADDR  Reuse of local addresses is supported.
    int val = 1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));

    //Bind to the socket, listen and then accept connections
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234); //The ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.
    addr.sin_addr.s_addr = ntohl(0); //wildcard address 0.0.0.0

    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0)
        die("bind()");

    rv = listen(fd,SOMAXCONN);
    if (rv <0)
        die("listen()");

    while(true) {
        struct sockaddr_in client_addr = {0};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0)
            continue;

        //handle multiple requests, however the server serves one client
        //connection at a time.
        while(true){
            int32_t err = handle_request(connfd);
            if (err < 0) {
                break;
            }
        }
        close(connfd);
    }
    return 0;
}
