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
#include <unistd.h>
#include <time.h>

struct Header{
    int messageType;// 1 = actual data, 0 = heartbeat message
    int ack; // acknowledgment number
    int seq; // sequence number
    int sessionID;// keep track of session
};
void setAndSendHeader(int socket, int seq, int ack, int sID, int msg_type){
    struct Header header;
    header.sessionID = sID;
    header.messageType = msg_type;
    header.seq = seq;
    header.ack = ack;
    char headerBuf[sizeof(struct Header)];
    memcpy(headerBuf, &header, sizeof(struct Header));
    send(socket, headerBuf, sizeof(struct Header), 0);//send the first heartbeat message to sproxy
}
void sendHeartBeat(int s, int sID){
    struct Header header;
    header.sessionID = sID;
    header.messageType = 0;
    header.ack = -1;
    header.seq = -1;
    char headerBuf[sizeof(struct Header)];
    memcpy(headerBuf, &header, sizeof(struct Header));
    int res = send(s, headerBuf, sizeof(struct Header), 0);//send the first heartbeat message to sproxy
    printf("send heatbeat status: %d\n", res);
    uint32_t networkPayload = htonl((uint32_t)sID);
    int res2 = send(s, &networkPayload, sizeof(uint32_t), 0);// send the heartbeat playload to sproxy
    printf("send playload status: %d\n", res2);
}
void callSelect(int s, int ns, int sID) {//select() code
    int sessionID = sID;
    time_t lastReceiveTime, lastTransmitTime;
    //int seq_C, ack_C, seq_S, ack_S = -1;
    while(1){
        int n, MAX_LEN = 256;
        struct timeval tv;
        fd_set readfds;
//        char buf1[MAX_LEN], buf2[MAX_LEN];
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
        //time_t time1 = time(NULL);//get currecnt time
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        //select will return when at least one of the sockets has incoming traffic, or after timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
            return;
        }
        else if (rv == 0) {//Timeout occurred! No data after 1 seconds, means no data send out as well
            //printf("Timeout occurred! No data after 1 seconds\n");
//            struct Header heartBeat;
//            heartBeat.messageType = 0//means heartbeat message
//            send(s, buf2, fromTelnet_len, 0);//send heartbeat data to sproxy
            time_t newSendTime, newReceiveTime;
            newSendTime = time(NULL);
            newReceiveTime = time(NULL);
            if ((newSendTime - lastTransmitTime) >= 1){
                //todo send a heart beat message
                sendHeartBeat(s, sessionID);
            }
            else if((newReceiveTime - lastReceiveTime) >= 3){
                //todo timeout need to reconnect
                return;//since this connect is already lost
            }
        }
        else{
            // one or both of the descriptors have data

            if (FD_ISSET(s, &readfds)) {//receive from sproxy
                char buf1[MAX_LEN];
                char headerBuf[sizeof(struct Header)];
                if (recv(s, headerBuf, sizeof(struct Header), 0) < 0){//if receives fail
                    perror("error receiving header");
                }
                struct Header *receivedHeader = (struct Header *)headerBuf;//make the header buffer into Header struct
                printf("Received Header - messageType: %d, Seq: %d, Ack: %d, sessionID: %d\n", receivedHeader->messageType, receivedHeader->seq, receivedHeader->ack, receivedHeader->sessionID);
                ssize_t fromSproxy_len = recv(s, buf1, MAX_LEN, 0);//receive data from sproxy
                if (fromSproxy_len > 0){//if receive bytees
                    if (receivedHeader->messageType == 0) {//it's a heartbeat message, just receive it, do nothing
                        //TODO: update timestamp, and check session ID
                    }
                    else{//it's a actual data packet, need to receive and forward
                        send(ns, buf1, fromSproxy_len, 0);//send data to telnet
                    }
                }
                else if (fromSproxy_len == 0)
                    return;//connection closed, return
                else
                    return;//error happened, return
            }

            if (FD_ISSET(ns, &readfds)) {//receive data from telnet
                char buf2[MAX_LEN];
                ssize_t fromTelnet_len = recv(ns, buf2, MAX_LEN, 0);
                if (fromTelnet_len > 0){
                    setAndSendHeader(s, 0, 0, sessionID, 1);//set and send header
                    send(s, buf2, fromTelnet_len, 0);//send playload to sproxy
                }
                else if (fromTelnet_len == 0)
                    return;//connection closed, return
                else
                    return;//error happened, return
            }
        }
    }
}

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
    //printf("t socket bind_status%d\n", bind_status);
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
    while(1) {
        int sessionID;//keep track of this session ID
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
            //char *line = NULL; // a null line pointer that will be pointing to the line that is being read into the buffer
            //size_t len = 0; // capacity of the buffer, initialized as 0, let getline know the initial size of buffer, and can be updated by getline to have the actual size of buffer
            //ssize_t size; // actual length of input line in bytes
            if (connection_status == 0) {// if connect success
                printf("connect to sproxy success\n");
                // Seed the random number generator
                srand(time(NULL));
                // Generate a random number within the range of 0 to 100
                int randomNum = rand() % 101;  // rand() generates a number between 0 and RAND_MAX, so we use % 101 to get a number between 0 and 100
                sessionID = randomNum;
                sendHeartBeat(s, randomNum);//send first heartBeat message to sproxy with random generated session ID
                printf("sent out first heartBeat to sproxy\n");
                callSelect(s, ns, sessionID);//call the select method
            }
            else{
                perror("connect sproxy failed, attemping to reconnect\n");
            }
            close(ns);
            close(s);
        }
        else{
            perror("telnet fail");
        }
    }
    return 1;
}






