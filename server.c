//
// Created by Hsinwei Lin on 2024/1/28.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>  // Include for close function

int main(int argc, char * argv[]) {
    // server socket
    int s = socket(PF_INET, SOCK_STREAM, 0);
    // server address struct
    struct sockaddr_in myAddr;
    bzero(&myAddr, sizeof(myAddr));
    // assign values of server address struct
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = htons(atoi(argv[1]));

    // bind server address struct to server socket
    int bind_status = bind(s, (struct sockaddr *) &myAddr, sizeof(myAddr));
    printf("bind_status%d\n", bind_status);
    // keep at most 5 connections
    listen(s, 5);

    // client address/port
    struct sockaddr_in cliAddr;
    bzero(&cliAddr, sizeof(cliAddr));
    socklen_t len = sizeof(cliAddr);
//    // new socket for new connections
//    ns = accept(s, (struct sockaddr *) &cliAddr, &len);

    char buf[1024];
    // constantly receive client address/port
    while (1) {
        // new socket for new connections
        int ns = accept(s, (struct sockaddr *) &cliAddr, &len);
        ssize_t n = 0;
        uint32_t receive_len;
        while (recv(ns, &receive_len, sizeof(receive_len), 0) == sizeof(receive_len)) {//checking if it's a 4 byte number
            n = recv(ns, buf, sizeof(buf), 0);
            buf[n] = '\0';  // Add null terminator to properly terminate the buffer
            printf("%-4u\t%s", ntohl(receive_len), buf);
            memset(buf, 0, sizeof(buf));  // Reset buf
        }

        if (n == 0) {
            printf("Connection closed by peer\n");
        } else if (n < 0) {
            // Handle error
            perror("recv failed");
        }
        close(ns);
    }

}