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

void callSelect(int td, int ns) {//select() code

    while(1){
        int n, MAX_LEN = 256;
        struct timeval tv;
        fd_set readfds;
        char buf1[MAX_LEN], buf2[MAX_LEN];
        //clear the set
        FD_ZERO(&readfds);
        //add descriptors to the set
        FD_SET(td, &readfds);
        FD_SET(ns, &readfds);
        //find the largest descriptor, and + 1
        if (td > ns)
            n = td + 1;
        else
            n = ns + 1;

        //wait until either socket has data ready to recv() timeout 10.5 secs
        tv.tv_sec = 10;
        tv.tv_usec = 500000;
        //select will return when at least one of the sockets has incoming traffic, or after timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        else if (rv == 0) {
            printf("Timeout occurred! No data after 10.5 seconds\n");
        }
        else{
            // one or both of the descriptors have data
            if (FD_ISSET(td, &readfds)) {
                ssize_t fromTDaemon_len = recv(td, buf1, MAX_LEN, 0);//receive data from telnet daemon
                buf1[fromTDaemon_len] = '\0';
                printf("%s\n", buf1);//print received data
                send(ns, buf1, fromTDaemon_len, 0);//send data to cproxy
            }

            if (FD_ISSET(ns, &readfds)) {
                ssize_t fromCproxy_len = recv(ns, buf2, MAX_LEN, 0);//receive data from cproxy
                buf2[fromCproxy_len] = '\0';
                printf("%s\n", buf2);//print received data
                send(td, buf2, fromCproxy_len, 0);//send data to telnet daemon
            }
        }
    }
}

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
    printf("sproxy bind_status%d\n", bind_status);
    if (bind_status < 0) {
        perror("to daemon socket bind");
        exit(EXIT_FAILURE);
    }

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
//    while (1) {
//        // new socket for new connections
//        int ns = accept(s, (struct sockaddr *) &cliAddr, &len);
//        uint32_t n;//the 4 byte length of playload
//        while (recv(ns, &n, 4, 0) > 0) {// keep receive message from client, receive the 4 byte int, length of playload
//            recv(ns, buf, ntohl(n), 0);//
//            buf[ntohl(n)] = '\0';  // Add null terminator to properly terminate the buffer
//            printf("%-4u\t%s", ntohl(n), buf);
//            memset(buf, 0, sizeof(buf));  // Reset buf
//        }
//
//        close(ns);
//    }
    int ns = accept(s, (struct sockaddr *) &cliAddr, &len);
    if (ns > 0) {
        printf("connect with cproxy success\n");
        // td socket code
        int td = socket(PF_INET, SOCK_STREAM, 0);//for connection to telnet daemon
        //telnet daemon struct
        struct sockaddr_in telnetAddr;
        // clear serAddr
        bzero(&telnetAddr, sizeof(telnetAddr));
        // assign values of the telnet address struct
        telnetAddr.sin_family = AF_INET;
        telnetAddr.sin_addr.s_addr = inet_addr("127.0.0.1");//convert localhost to internet byte order
        telnetAddr.sin_port = htons(atoi("23"));//convert port 23 to integer then to internet byte order

        // connect sproxy client socket to telnet daemon socket
        int connection_status = connect(td, (struct sockaddr *) &telnetAddr, sizeof(telnetAddr));
        char *line = NULL; // a null line pointer that will be pointing to the line that is being read into the buffer
        size_t len = 0; // capacity of the buffer, initialized as 0, let getline know the initial size of buffer, and can be updated by getline to have the actual size of buffer
        ssize_t size; // actual length of input line in bytes
        if (connection_status == 0) {// if connect success
            printf("Connect to telnet daemon success\n");
            callSelect(td, ns);//call the select method
//            while ((size = getline(&line, &len, stdin)) != -1) {
//                uint32_t networkOrderSize = htonl((uint32_t)size);  // Convert size to network byte order
//                send(s, &networkOrderSize, 4, 0);  // Send out the size as a 4-byte integer
//                send(s, line, size, 0);//send out the line itself
//                //printf("%s", line);
//            }
//            free(line);
        }
        else{
            perror("connection to telnet daemon failed\n");
        }

        close(ns);
        close(td);
    }


}