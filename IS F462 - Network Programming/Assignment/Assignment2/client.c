#include <stdio.h>
#include <sys/socket.h> //for socket(), connect(), send(), recv() functions
#include <arpa/inet.h>  // different address structures are declared here
#include <stdlib.h>     // atoi() which convert string to integer
#include <string.h>
#include <unistd.h>     // close() function
#define BUFSIZE 1024

void error_exit(char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int main()
{
    /* CREATE A TCP SOCKET*/
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        error_exit("socket");
    }

    printf ("Client Socket Created\n");
    
    /*CONSTRUCT SERVER ADDRESS STRUCTURE*/
    struct sockaddr_in serverAddr;
    memset (&serverAddr,0,sizeof(serverAddr)); /*memset() is used to fill a block of memory with a particular value*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8004);         //You can change port number here
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //Specify server's IP address here

    printf ("Address assigned\n");
    
    /*ESTABLISH CONNECTION*/
    int connectStatus = connect(sockfd , (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    printf ("Connection status : %d\n",connectStatus);
    
    if(connectStatus < 0){
         error_exit("connect");
    }
    printf ("Connection Established\n");
    
    /*SEND DATA*/
    printf ("ENTER MESSAGE FOR SERVER\n");
    
    char *msg = "GET /inde.html HTTP/1.1\n\
                Host: 127.0.0.1:8004\n\n";    
    
    int bytesSent = send(sockfd, msg, strlen(msg), 0);
    if (bytesSent != strlen(msg)){ 
        error_exit("send");
    }

    printf ("Data Sent to the server\n");
    
    /*RECEIVE BYTES*/
    char recvBuffer[BUFSIZE];
    int bytesRecvd;
    while( (bytesRecvd = recv (sockfd, recvBuffer, BUFSIZE-1, 0)) > 0){
        recvBuffer[bytesRecvd] = '\0';
        printf ("Received from the server : %s\n",recvBuffer);
    }
    
    while(1);
    // close(sockfd);
}