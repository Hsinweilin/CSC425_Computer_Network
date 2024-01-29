//
// Created by Hsinwei Lin on 2024/1/28.
//
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

int main (int argc, char * argv[])
{
    // client socket
    int s = socket(PF_INET, int SOCK_STREAM, 0);
    // server address struct
    struct sockaddr_in serAddr;
    // clear serAddr
    bzero(&serAddr, sizeof(serAddr));
    // assign values of the server address struct
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(argv[0]);
    serAddr.sin_port = hton(argv[1]);

    // connect
    connect(s, (stuct socketaddr *) &serAddr, sizeof(serAddr));

}



