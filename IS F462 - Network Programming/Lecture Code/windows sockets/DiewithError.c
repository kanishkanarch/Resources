/* CHANGES FROM UNIX VERSION                                                   */
/*                                                                             */
/* 1.  Changed header files.                                                   */
/* 2.  Used WSAGetLastError() instead of perror().                             */ 

#include <stdio.h>    /* for fprintf() */
#include <winsock.h>  /* for WSAGetLastError() */
#include <stdlib.h>   /* for exit() */

void DieWithError(char *errorMessage)
{
    fprintf(stderr,"%s: %d\n", errorMessage, WSAGetLastError());
    exit(1);
}