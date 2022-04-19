/* CHANGES FROM UNIX VERSION                                                   */
/*                                                                             */
/* 1.  Changed header files.                                                   */
/* 2.  Added WSAStartUP().                                                     */
/* 3.  Used Windows threads instead of POSIX.                                  */

#include "TCPEchoServerWS.h"   /* TCP echo server includes */

void *ThreadMain(void *arg);            /* Main program of a thread */

/* Structure of arguments to pass to client thread */
struct ThreadArgs
{
    int clntSock;                      /* Socket descriptor for client */
};

void main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short echoServPort;     /* Server port */
    DWORD  threadID;                 /* Thread ID from CreateThread() */
    struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */
    WSADATA wsaData;                 /* Structure for WinSock setup communication */

    if (argc != 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage:  %s <SERVER PORT>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* Local port */

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) /* Load Winsock 2.0 DLL */
    {
        fprintf(stderr, "WSAStartup() failed");
        exit(1);
    }

    servSock = CreateTCPServerSocket(echoServPort);

    for (;;) /* Run forever */
    {
       clntSock = AcceptTCPConnection(servSock);

        /* Create separate memory for client argument */
        threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
        threadArgs -> clntSock = clntSock;

        if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ThreadMain, threadArgs, 
              0, (LPDWORD) &threadID) == NULL)
            DieWithError("pthread_create() failed");
        printf("with thread %ld\n", threadID);
    }
    /* NOT REACHED */
}

void *ThreadMain(void *threadArgs)
{
    int clntSock;                   /* Socket descriptor for client connection */

    /* Guarantees that thread resources are deallocated upon return */

    /* Extract socket file descriptor from argument */
    clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;
    free(threadArgs);              /* Deallocate memory for argument */

    HandleTCPClient(clntSock);

    return (NULL);
}
