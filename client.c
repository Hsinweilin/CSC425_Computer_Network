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

struct Data{
    int sequenceNum;
    char buf[256];
};
struct Node{
    struct Data data;
    struct Node *next;
};
struct Node* createNode(int num, const char* text){
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node));
    newNode->data.sequenceNum = num;
    strncpy(newNode->data.buf, text, 256);
    newNode->next = NULL;
    return newNode;
}
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
void sendHeartBeat(int s, int sID, int seq, int ack){
    //printf("send heart beat to sproxy\n");
    struct Header header;
    header.sessionID = sID;
    header.messageType = 0;
    header.ack = ack;
    header.seq = seq;
    char headerBuf[sizeof(struct Header)];
    memcpy(headerBuf, &header, sizeof(struct Header));
    send(s, headerBuf, sizeof(struct Header), 0);//send the first heartbeat message to sproxy
    //printf("send heatbeat status: %d\n", res);
    uint32_t networkPayload = htonl((uint32_t)sID);
    send(s, &networkPayload, sizeof(uint32_t), 0);// send the heartbeat playload to sproxy
    //printf("send playload status: %d\n", res2);
}
int callSelect(int s, int ns, int sID, int *seq_C, int *ack_C) {//select() code
    int sessionID = sID;
    time_t lastReceiveTime, lastTransmitTime;
    lastReceiveTime = time(NULL);
    lastTransmitTime = time(NULL);
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

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        //select will return when at least one of the sockets has incoming traffic, or after timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
            return 0;
        }
        else if (rv == 0) {//Timeout occurred! No data after 1 seconds, means no data send out as well
            time_t newSendTime, newReceiveTime;
            newSendTime = time(NULL);
            newReceiveTime = time(NULL);
            if ((newSendTime - lastTransmitTime) >= 1){
                //todo send a heart beat message
                //printf("send heart beat\n");
                sendHeartBeat(s, sessionID, -1, -1);
                lastTransmitTime = time(NULL);
            }

            if ((newReceiveTime - lastReceiveTime) >= 3){
                //todo timeout need to reconnect
                //printf("connection lost, reconnecting\n");
                return 0;//since this connect is already lost
            }
        }
        else{// one or both of the descriptors have data
            if (FD_ISSET(s, &readfds)) {//receive from sproxy
                time_t newSendTime = time(NULL);
                if ((newSendTime - lastTransmitTime) >= 1){
                    //todo send a heart beat message
                    //printf("send heart beat\n");
                    sendHeartBeat(s, sessionID, *seq_C, *ack_C);
                    lastTransmitTime = time(NULL);
                }
                lastReceiveTime = time(NULL);//update last receiveTime once receive data from sproxy
                //printf("update receive time %ld\n", lastReceiveTime);
                char buf1[MAX_LEN];
                char headerBuf[sizeof(struct Header)];
                if (recv(s, headerBuf, sizeof(struct Header), 0) < 0){//if receives fail
                    //printf("error receiving header");
                    perror("error receiving header");
                }
                struct Header *receivedHeader = (struct Header *)headerBuf;//make the header buffer into Header struct
                //printf("Received Header - messageType: %d, Seq: %d, Ack: %d, sessionID: %d\n", receivedHeader->messageType, receivedHeader->seq, receivedHeader->ack, receivedHeader->sessionID);
                if (receivedHeader->seq != -1)
                    *ack_C = receivedHeader->seq + 1;

                memset(buf1, 0, sizeof(buf1));//initialize buffer
                ssize_t fromSproxy_len = recv(s, buf1, MAX_LEN, 0);//receive data from sproxy
                if (fromSproxy_len > 0){//if receive bytees
                    if (receivedHeader->messageType == 0) {//it's a heartbeat message, just receive it, do nothing
                        //TODO: update timestamp, and check session ID
                        //printf("received heart beat\n");
                    }
                    else{//it's a actual data packet, need to receive and forward
                        //printf("%s %zd\n", buf1, fromSproxy_len);
                        send(ns, buf1, fromSproxy_len, 0);//send data to telnet
                    }
                }
                else if (fromSproxy_len == 0)
                    return 0;//connection closed, return
                else{
                    perror("error receiving from sproxy");
                    return 0;//error happened, return
                }
            }

            if (FD_ISSET(ns, &readfds)) {//receive data from telnet
                char buf2[MAX_LEN];
                ssize_t fromTelnet_len = recv(ns, buf2, MAX_LEN, 0);
                if (fromTelnet_len > 0){
                    *seq_C += 1;
                    setAndSendHeader(s, *seq_C, *ack_C, sessionID, 1);//set and send header
                    send(s, buf2, fromTelnet_len, 0);//send playload to sproxy
                    lastTransmitTime = time(NULL);//update lastTransmitTime once send out data to sproxy
                }
                else if (fromTelnet_len == 0)
                    return 1;//connection closed, return
                else
                    return 1;//error happened, return
            }
        }
    }
}

int main (int argc, char * argv[])
{
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
        int ns = accept(t, (struct sockaddr *) &cliAddr, &tlen);
        if (ns > 0) {// if connection from telnet success, try to connect to sproxy
            //printf("connect to telnet success\n");
            int sessionID;//keep track of this session ID
            srand(time(NULL));// Seed the random number generator
            sessionID = rand() % 101;  // rand() generates a number between 0 and RAND_MAX, so we use % 101 to get a number between 0 and 100
            int num1 = 0;
            int num2 = 1;
//            int *seq_C = &num1;
//            int *ack_C = &num2;
            // connect cproxy socket to sproxy socket
            while(1){//create and keep trying to reconnect unless user stop the connection
                //create connection to sproxy
                int s = socket(PF_INET, SOCK_STREAM, 0);//socket to sproxy
                struct sockaddr_in serAddr;
                // clear serAddr
                bzero(&serAddr, sizeof(serAddr));
                // assign values of the sproxy address struct
                serAddr.sin_family = AF_INET;
                serAddr.sin_addr.s_addr = inet_addr(argv[2]);//convert IP address to internet byte order
                serAddr.sin_port = htons(atoi(argv[3]));//convert string to integer then to internet byte order
                int connection_status = connect(s, (struct sockaddr *) &serAddr, sizeof(serAddr));
                sleep(1);
                if (connection_status == 0) {// if connect success
                    //printf("connect to sproxy success\n");
                    sendHeartBeat(s, sessionID, num1, num2);//send first heartBeat message to sproxy with random generated session ID
//                    printf("sent out first heartBeat to sproxy\n");
                    int r = callSelect(s, ns, sessionID, &num1, &num2);//call the select method
                    if (r == 1){
                        //printf("connection to telnet close, waiting for next telnet session\n");
                        num1 = 0;
                        num2 = 1;
                        break;
                    }
                    else
                        //printf("connection to sproxy closed, reconnecting\n");
                    close(s);
                }
                else{
                    //perror("connect sproxy failed, attemping to reconnect\n");
                }
            }
            close(ns);
//            close(s);
        }
        else{
            perror("telnet fail");
        }
    }
    return 1;
}






