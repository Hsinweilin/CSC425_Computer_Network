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
#include <arpa/inet.h>// Include for inet_addr function
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
    send(socket, headerBuf, sizeof(struct Header), 0);//send the header
}

void sendHeartBeat(int s, int sID){
    struct Header header;
    header.sessionID = sID;
    header.messageType = 0;
    header.ack = -1;
    header.seq = -1;
    char headerBuf[sizeof(struct Header)];
    memcpy(headerBuf, &header, sizeof(struct Header));
    send(s, headerBuf, sizeof(struct Header), 0);//send the first heartbeat message to sproxy
    uint32_t networkPayload = htonl((uint32_t)sID);
    send(s, &networkPayload, sizeof(uint32_t), 0);// send the heartbeat playload to sproxy
}
void callSelect(int td, int ns, int sID) {//select() code
    int seq_C, ack_C, seq_S, ack_S = -1;
    int sessionID = sID;
    time_t lastReceiveTime, lastTransmitTime;
    while(1){
        int n, MAX_LEN = 256;
        struct timeval tv;
        fd_set readfds;
//        char buf1[MAX_LEN], buf2[MAX_LEN];
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
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        //select will return when at least one of the sockets has incoming traffic, or after timeout
        int rv = select(n, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
//            exit(EXIT_FAILURE);
            return;
        }
        else if (rv == 0) {
            //printf("Timeout occurred! No data after 1 seconds\n");
            time_t newSendTime, newReceiveTime;
            newSendTime = time(NULL);
            newReceiveTime = time(NULL);
            if ((newSendTime - lastTransmitTime) >= 1){
                //todo send a heart beat message
                sendHeartBeat(ns, sessionID);
            }
            else if((newReceiveTime - lastReceiveTime) >= 3){
                //todo timeout need to reconnect
                return;//since this connect is already lost
            }
        }
        else{
            // one or both of the descriptors have data
            if (FD_ISSET(td, &readfds)) {
                char buf1[MAX_LEN];
                ssize_t fromTDaemon_len = recv(td, buf1, MAX_LEN, 0);//receive data from telnet daemon
                if (fromTDaemon_len > 0){
                    if (seq_S == -1){// if the first data transmit, initialize seq, ack
                        seq_S = 0;
                        ack_S = 0;
                    }
                    //TODO set and send header
                    setAndSendHeader(ns, seq_S, ack_S, sessionID, 1);//send header to cproxy
                    send(ns, buf1, fromTDaemon_len, 0);//send data to cproxy
                    lastTransmitTime = time(NULL);//update lastTransmitTime once send out data to cproxy
                }
                else if (fromTDaemon_len == 0)
                    return;//connection closed, return
                else
                    return;//error happened, return
            }

            if (FD_ISSET(ns, &readfds)) {// if receive data from cproxy
                char buf2[MAX_LEN];
                char headerBuf[sizeof(struct Header)];
                if (recv(ns, headerBuf, sizeof(struct Header), 0) < 0){//if receives fail
                    perror("error receiving header");
                }
                struct Header *receivedHeader = (struct Header *)headerBuf;//make the header buffer into Header struct
                printf("Received Header - messageType: %d, Seq: %d, Ack: %d, sessionID: %d\n", receivedHeader->messageType, receivedHeader->seq, receivedHeader->ack, receivedHeader->sessionID);
                seq_C = receivedHeader->seq;
                ack_C = receivedHeader->ack;
                seq_C++;
                ack_C++;
                ssize_t fromCproxy_len = recv(ns, buf2, MAX_LEN, 0);//receive data from cproxy
                lastReceiveTime = time(NULL);//update last receiveTime once receive data from cproxy
                if (fromCproxy_len > 0){
                    if (receivedHeader->messageType == 0){
                        printf("it's a heart beat message\n");
                    }
                    else{
                        send(td, buf2, fromCproxy_len, 0);//send data to telnet daemon
                    }
                }
                else if (fromCproxy_len == 0)
                    return;//connection closed, return
                else
                    return;//error happened, return
                //buf2[fromCproxy_len] = '\0';
                //printf("%s\n", buf2);//print received data

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
    if (bind_status < 0) {
        perror("to cproxy socket bind");
        exit(EXIT_FAILURE);
    }
    // keep at most 5 connections
    int listen_status = listen(s, 5);
    if (listen_status < 0){
        perror("to cproxy socket listen");
        exit(EXIT_FAILURE);
    }
    // client address/port
    struct sockaddr_in cliAddr;
    bzero(&cliAddr, sizeof(cliAddr));
    socklen_t len = sizeof(cliAddr);

    while(1) {
        int sessionID = -1;// keep track of this session
        printf("accepting sockets\n");
        int ns = accept(s, (struct sockaddr *) &cliAddr, &len);
        if (ns > 0) {
            printf("connect with cproxy success\n");

            //receive first heartBeat message from cproxy
            char headerBuf[sizeof(struct Header)];
            if (recv(ns, headerBuf, sizeof(struct Header), 0) < 0){//if receives fail
                perror("error receiving header");
            }
            struct Header *receivedHeader = (struct Header *)headerBuf;//make the header buffer into Header struct
            //printf("Received Header - messageType: %d, Seq: %d, Ack: %d, sessionID: %d", receivedHeader->messageType, receivedHeader->seq, receivedHeader->ack, receivedHeader->sessionID);
            if (sessionID == -1){
                sessionID = receivedHeader->sessionID;
            }
            char buf1[256];
            ssize_t fromCproxy_len = recv(ns, buf1, 256, 0);//receive first data from cproxy
            if (fromCproxy_len > 0)
                printf("recieve first heartbeat success\n");
            else if (fromCproxy_len == 0)
                return 0;//connection closed, return
            else
                return 0;//error happened, return

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
            //char *line = NULL; // a null line pointer that will be pointing to the line that is being read into the buffer
            //size_t len = 0; // capacity of the buffer, initialized as 0, let getline know the initial size of buffer, and can be updated by getline to have the actual size of buffer
            //ssize_t size; // actual length of input line in bytes
            if (connection_status == 0) {// if connect success
                printf("Connect to telnet daemon success\n");
                callSelect(td, ns, sessionID);//call the select method
            }
            else{
                perror("connection to telnet daemon failed\n");
            }

            close(ns);
            close(td);
        }
        else{
            printf("connect to cproxy failed\n");
        }
    }

    return 1;
}
