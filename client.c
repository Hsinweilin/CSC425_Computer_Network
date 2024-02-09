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
    int s = socket(PF_INET, SOCK_STREAM, 0);
    // server address struct
    struct sockaddr_in serAddr;
    // clear serAddr
    bzero(&serAddr, sizeof(serAddr));
    // assign values of the server address struct
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(argv[1]);//convert IP address to internet byte order
    serAddr.sin_port = htons(atoi(argv[2]));//convert string to integer then to internet byte order

    // connect client socket to server socket
    int connection_status = connect(s, (struct sockaddr *) &serAddr, sizeof(serAddr));
    char *line = NULL; // a null line pointer that will be pointing to the line that is being read into the buffer
    size_t len = 0; // capacity of the buffer, initialized as 0, let getline know the initial size of buffer, and can be updated by getline to have the actual size of buffer
    ssize_t size; // actual length of input line in bytes
    if (connection_status == 0) {// if connect success
        printf("Connect success\n");
        while ((size = getline(&line, &len, stdin)) != -1) {
            uint32_t networkOrderSize = htonl((uint32_t)size);  // Convert size to network byte order
            send(s, &networkOrderSize, sizeof(networkOrderSize), 0);  // Send out the size as a 4-byte integer
            send(s, line, size, 0);//send out the line itself
            //printf("%s", line);
        }
        free(line);
    }
    else{
        perror("connection failed\n");
    }
}



