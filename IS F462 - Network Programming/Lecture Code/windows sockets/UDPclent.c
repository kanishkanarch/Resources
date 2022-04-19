/* CHANGES FROM UNIX VERSION                                                   */
/*                                                                             */
/* 1.  Changed header files.                                                   */
/* 2.  Added WSAStartUP() and WSACleanUp().                                    */
/* 3.  Used closesocket() instead of close().                                  */ 

#include <stdio.h>      /* for printf(), fprintf() */
#include <winsock.h>    /* for socket(),... */
#include <stdlib.h>     /* for exit() */

#define ECHOMAX 255     /* Longest string to echo */

void DieWithError(char *errorMessage);  /* External error handling function */

void main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned short echoServPort;     /* Echo server port */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    char *servIP;                    /* IP address of server */
    char *echoString;                /* String to send to echo server */
    char echoBuffer[ECHOMAX];        /* Buffer for echo string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;               /* Length of response string */
    WSADATA wsaData;                 /* Structure for WinSock setup communication */

    if ((argc < 3) || (argc > 4))    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <Server IP> <Echo Word> [<Echo Port>]\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];           /* first arg: server IP address (dotted quad)*/
    echoString = argv[2];       /* second arg: string to echo */

    if ((echoStringLen = strlen(echoString) + 1) > ECHOMAX)  /* Check input length */
        DieWithError("Echo word too long");

    if (argc == 4)
        echoServPort = atoi(argv[3]);  /* Use given port, if any */
    else
        echoServPort = 7;  /* otherwise, 7 is the well-known port for the echo service */

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) /* Load Winsock 2.0 DLL */
    {
        fprintf(stderr, "WSAStartup() failed");
        exit(1);
    }

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                 /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port   = htons(echoServPort);     /* Server port */

    /* Send the string, including the null terminator, to the server */
    if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
               &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
        DieWithError("sendto() sent a different number of bytes than expected");
  
    /* Recv a response */
    
    fromSize = sizeof(fromAddr);
    if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, 
                 &fromSize)) != echoStringLen)
        DieWithError("recvfrom() failed");

    if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr)
    {
        fprintf(stderr,"Error: received a packet from unknown source.\n");
        exit(1);
    }

    if (echoBuffer[respStringLen-1])  /* Do not printf unless it is terminated */
        printf("Received an unterminated string\n");
    else
        printf("Received: %s\n", echoBuffer);    /* Print the echoed arg */

    closesocket(sock);
    WSACleanup();  /* Cleanup Winsock */

    exit(0);
}
