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
    serAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serAddr.sin_port = htons(atoi(argv[2]));

    // connect
    int connection_status = connect(s, (struct sockaddr *) &serAddr, sizeof(serAddr));
    char *line = NULL;
    size_t len = 0;
    ssize_t     size;
    if (connection_status == 0) {// if connect success
        printf("Connect success\n");
        while ((size = getline(&line, &len, stdin)) != -1) {
            uint32_t networkOrderSize = htonl((uint32_t)size);  // Convert size to network byte order
            send(s, &networkOrderSize, sizeof(uint32_t), 0);  // Send out the size as a 4-byte integer
            send(s, line, size, 0);//send out the line itself
            //printf("%s", line);
        }
    }
    else{
        perror("connection failed\n");
    }
}



