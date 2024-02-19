//
// Created by Hsinwei Lin on 2024/1/28.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

int main (int argc, char * argv[])
{
    // client socket
//    int s = socket(PF_INET, SOCK_STREAM, 0);//socket to sproxy, sending
//    int t = socket(PF_INET, SOCK_STREAM, 0);//socket to telnet, receiving

    // s socket code
    // sproxt address struct
//    struct sockaddr_in serAddr;
//    // clear serAddr
//    bzero(&serAddr, sizeof(serAddr));
//    // assign values of the sproxy address struct
//    serAddr.sin_family = AF_INET;
//    serAddr.sin_addr.s_addr = inet_addr(argv[2]);//convert IP address to internet byte order
//    serAddr.sin_port = htons(atoi(argv[3]));//convert string to integer then to internet byte order
//
//    // connect client socket to sproxy socket
//    int connection_status = connect(s, (struct sockaddr *) &serAddr, sizeof(serAddr));
//    char *line = NULL; // a null line pointer that will be pointing to the line that is being read into the buffer
//    size_t len = 0; // capacity of the buffer, initialized as 0, let getline know the initial size of buffer, and can be updated by getline to have the actual size of buffer
//    ssize_t size; // actual length of input line in bytes
//    if (connection_status == 0) {// if connect success
//        printf("Connect success\n");
//        while ((size = getline(&line, &len, stdin)) != -1) {
//            uint32_t networkOrderSize = htonl((uint32_t)size);  // Convert size to network byte order
//            send(s, &networkOrderSize, 4, 0);  // Send out the size as a 4-byte integer
//            send(s, line, size, 0);//send out the line itself
//            //printf("%s", line);
//        }
//        free(line);
//    }
//    else{
//        perror("connection failed\n");
//    }


    //t socket code, as server, accepting connection from telnet
    int t = socket(PF_INET, SOCK_STREAM, 0);//socket to telnet, receiving
    struct sockaddr_in myAddr;
    bzero(&myAddr, sizeof(myAddr));
    // assign values of server address struct
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = INADDR_ANY;
    myAddr.sin_port = htons(atoi(argv[1]));

    // bind server address struct to t socket
    int bind_status = bind(t, (struct sockaddr *) &myAddr, sizeof(myAddr));
    printf("t socket bind_status%d\n", bind_status);
    if (bind_status < 0) {
        perror("t socket bind");
        exit(EXIT_FAILURE);
    }
    // keep at most 5 connections
    listen(t, 5);

    // telnet address/port
    struct sockaddr_in cliAddr;
    bzero(&cliAddr, sizeof(cliAddr));
    socklen_t tlen = sizeof(cliAddr);
    int ns = accept(t, (struct sockaddr *) &cliAddr, &tlen);
    if (ns > 0) {// if connection from telnet success, try to connect to sproxy
        printf("connect to telnet success\n");
        //create connection to sproxy
        int s = socket(PF_INET, SOCK_STREAM, 0);//socket to sproxy
        struct sockaddr_in serAddr;
        // clear serAddr
        bzero(&serAddr, sizeof(serAddr));
        // assign values of the sproxy address struct
        serAddr.sin_family = AF_INET;
        serAddr.sin_addr.s_addr = inet_addr(argv[2]);//convert IP address to internet byte order
        serAddr.sin_port = htons(atoi(argv[3]));//convert string to integer then to internet byte order

        // connect cproxy socket to sproxy socket
        int connection_status = connect(s, (struct sockaddr *) &serAddr, sizeof(serAddr));
        char *line = NULL; // a null line pointer that will be pointing to the line that is being read into the buffer
        size_t len = 0; // capacity of the buffer, initialized as 0, let getline know the initial size of buffer, and can be updated by getline to have the actual size of buffer
        ssize_t size; // actual length of input line in bytes
        if (connection_status == 0) {// if connect success
            printf("Connect to sproxy success\n");
            //callSelect(s, ns)//call the select method
        }
        else{
            perror("connect sproxy failed\n");
        }
        close(ns);
        close(s);
    }
    else{
        perror("telnet fail");
    }
}

void callSelect(int s, int ns) {//select() code
    int n, MAX_LEN = 256;
    struct timeval tv;
    fd_set readfds;
    char buf1[MAX_LEN], buf2[MAX_LEN];
    //clear the set
    FD_ZERO(&readfds);
    //add descriptors to the set
    FD_SET(s, &readfds);
    FD_SET(ns, &readfds);
    //find the largest descriptor, and + 1
    if (s > ns)
        n = s + 1;
    else
        n = ns + 1;

    //wait until either socket has data ready to recv() timeout 10.5 secs
    tv.tv_sec = 10;
    tv.tv_usec = 500000;

    while(1){
        //select will return when at least one of the sockets has incoming traffic, or after timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
        }
        else if (rv == 0) {
            printf("Timeout occurred! No data after 10.5 seconds\n");
        }
        else{
            // one or both of the descriptors have data
            if (FD_ISSET(s, &readfds)) {
                recv(s, buf1, MAX_LEN, 0);//receive data from sproxy
                //TODO: send data to telnet
            }

            if (FD_ISSET(ns, &readfds))
                recv(ns, buf2, MAX_LEN, 0);//receive data from telnet
            //TODO: send data to sproxy
        }
    }
}



