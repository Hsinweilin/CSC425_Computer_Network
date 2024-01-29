//
// Created by Hsinwei Lin on 2024/1/28.
//
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main() {
    // server socket
    int s = socket(PF_INET, int SOCK_STREAM, 0);
    // server address struct
    struct sockaddr_in myAddr;
    bzero(&myAddr, sizeof(myAddr));
    // assign values of server address struct
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = hton(80);

    // bind server address struct to server socket
    bind(s, (struct sockaddr *) &myAddr, sizeof(myAddr));
    // keep at most 5 connections
    listen(s, 5);

    // client address/port
    struct sockaddr_in cliAddr;
    bzero(&cliAddr, sizeof(cliAddr));
    len = sizeof(cliAddr);
//    // new socket for new connections
//    ns = accept(s, (struct sockaddr *) &cliAddr, &len);

    // constantly receive client address/port
    buf = char[1024];
    while (1) {
        // new socket for new connections
        ns = accept(s, &cliAddr, &len);
        while (n = recv(ns, buf, sizeof(buf), 0)) {
            printf("%s\n", n);
        }
        close(ns);
    }

}