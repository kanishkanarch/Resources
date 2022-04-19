/*TCPServerwithSelectNBIO.c*/  
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/fcntl.h>
  
#define LISTENQ 15
#define MAXLINE 80
#define EXPECTED_LEN 10
  struct clientstate
{
  int fd;
   int state;			//0 connection accepted, 1-request being read, 2-processing, 3-replying back
  char buf[MAXLINE];
   int in_index;
   int out_index;
 };
 void
processClient (struct clientstate *cs)
{
  int i;
  for (i = 0; i < EXPECTED_LEN; i++)
    cs->buf[i] = toupper (cs->buf[i]);
}

 int 
main (int argc, char **argv) 
{
  int i, maxi, maxfd, listenfd, connfd, sockfd;
  int toProcess = 0;
  int nready;
  struct clientstate client[FD_SETSIZE];
  ssize_t n;
  fd_set rset, allsetr, wset, allsetw;
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;
   listenfd = socket (AF_INET, SOCK_STREAM, 0);
   bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi (argv[1]));
   bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr));
   listen (listenfd, LISTENQ);
   maxfd = listenfd;		/* initialize */
  maxi = -1;			/* index into client[] array */
  for (i = 0; i < FD_SETSIZE; i++)
    {
      client[i].fd = -1;	/* -1 indicates available entry */
      client[i].state = -1;
      client[i].in_index = 0;
      client[i].out_index = 0;
    }
  FD_ZERO (&allsetr);
  FD_ZERO (&allsetw);
  FD_SET (listenfd, &allsetr);
   for (;;)
    
    {
      rset = allsetr;		/* structure assignment */
      wset = allsetw;
      nready = select (maxfd + 1, &rset, &wset, NULL, NULL);
       if (FD_ISSET (listenfd, &rset))
	
	{			/* new client connection */
	  clilen = sizeof (cliaddr);
	  connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
		int fl=fcntl(connfd, F_GETFL, 0);
		fl=fl|O_NONBLOCK;
		fcntl(connfd, F_SETFL, &fl);
	  printf ("new client: %s, port %d\n",
		   inet_ntop (AF_INET, &cliaddr.sin_addr, 4, NULL),
		   ntohs (cliaddr.sin_port));
	   for (i = 0; i < FD_SETSIZE; i++)
	    if (client[i].fd < 0)
	      
	      {
		client[i].fd = connfd;	/* save descriptor */
		client[i].state = 0;
		break;
	      }
	  if (i == FD_SETSIZE)
	    {
	       printf ("too many clients");
	      exit (0);
	    }
	  FD_SET (connfd, &allsetr);	/* add new descriptor to set */
	  if (connfd > maxfd)
	    maxfd = connfd;	/* for select */
	  if (i > maxi)
	    maxi = i;		/* max index in client[] array */
	   if (--nready <= 0)
	    continue;		/* no more readable descriptors */
	}
       for (i = 0; i <= maxi; i++)
	
	{			/* check all clients for data */
	  if ((sockfd = client[i].fd) < 0)
	    continue;
	  printf ("%d %d %d %d %s\n", sockfd, client[i].state,
		   client[i].in_index, client[i].out_index, client[i].buf);
	   if (client[i].state == 3 && FD_ISSET (sockfd, &wset))
	    {
		printf("Writing\n");
	       n =
		      write (sockfd, client[i].buf + client[i].out_index,EXPECTED_LEN);
		
		
		  client[i].out_index = client[i].out_index + n;
		  if (client[i].out_index >= EXPECTED_LEN)
		    {
		      close (client[i].fd);
		      client[i].state = -1;
			client[i].in_index=0;
			client[i].out_index=0;
		      client[i].fd = -1;
		      FD_CLR (sockfd, &allsetw);
		    }
	
	    }
	  if ((client[i].state == 0 || client[i].state == 1)
	       && FD_ISSET (sockfd, &rset))
	    
	    {
	      if ((n =
		    read (sockfd, client[i].buf + client[i].in_index,
			  MAXLINE)) == 0)
		
		{
		  
		    /*connection closed by client */ 
		    close (sockfd);
		  FD_CLR (sockfd, &allsetr);
		  client[i].fd = -1;
		  client[i].state = -1;
		}
	      
	      else
		{
		  client[i].state = 1;
		  client[i].in_index = client[i].in_index + n;
		  if (client[i].in_index >= EXPECTED_LEN)
		    {
		      client[i].state = 2;
		      FD_CLR (sockfd, &allsetr);
		      toProcess++;
		    }
		  
		    //write (sockfd, buf, n);
		}
	       if (--nready <= 0)
		break;		/* no more readable descriptors */
	    }
	}
      if (toProcess > 0)
	for (i = 0; i <= maxi; i++)
	  
	  {			/* check all clients for data */
	    if ((sockfd = client[i].fd) < 0)
	      continue;
	    if (client[i].state == 2)
	      {
		processClient (&client[i]);
		toProcess--;
		FD_SET (sockfd, &allsetw);
		client[i].state = 3;
	      }
	  }
    }
}

 
