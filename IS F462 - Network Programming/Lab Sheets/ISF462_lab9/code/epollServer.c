#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

extern int errno;
#define LISTENQ 5
#define MAX_BUF 10		/* Maximum bytes fetched by a single read() */
#define MAX_EVENTS 5		/* Maximum number of events to be returned from
				   a single epoll_wait() call */
void
errExit (char *s)
{
  perror (s);
  exit (1);
}

int
main (int argc, char *argv[])
{
  int epfd, ready, fd, s, j, num0penFds;
  struct epoll_event ev;
  struct epoll_event evlist[MAX_EVENTS];
  char buf[MAX_BUF];
  int listenfd, clilen;

  struct sockaddr_in cliaddr, servaddr;

  listenfd = socket (AF_INET, SOCK_STREAM, 0);

  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi (argv[1]));

  if (bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
    perror ("bind");

  listen (listenfd, LISTENQ);



  if (argc < 2 || strcmp (argv[1], "--help") == 0)
    printf ("Uage: %s <port>\n", argv[0]);

  epfd = epoll_create (20);
  if (epfd == -1)
    errExit ("epoll_create");


  ev.events = EPOLLIN;		/* Only interested in input events */
  ev.data.fd = listenfd;
  if (epoll_ctl (epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
    errExit ("epoll_ctl");
  for (;;)
    {
      ready = epoll_wait (epfd, evlist, MAX_EVENTS, -1);
      if (ready == -1)
	{
	  if (errno == EINTR)
	    continue;		/* Restart if interrupted by signal */
	  else
	    errExit ("epoll_wait");
	}
      //printf("nready=%d\n", ready);

      for (j = 0; j < ready; j++)
	{
	  if (evlist[j].events & EPOLLIN)
	    {
	      if (evlist[j].data.fd == listenfd)
		{
		  clilen = sizeof (cliaddr);
		  char ip[128];
		  memset (ip, '\0', 128);
		  int connfd =
		    accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);

		  if (cliaddr.sin_family == AF_INET)
		    {
		      if (inet_ntop (AF_INET, &(cliaddr.sin_addr), ip, 128) ==
			  NULL)
			perror ("Error in inet_ntop\n");
		    }

		  if (cliaddr.sin_family == AF_INET6)
		    {
		      inet_ntop (AF_INET6, &(cliaddr.sin_addr), ip, 128);
		    }

		  printf ("new client: %s, port %d\n", ip,
			  ntohs (cliaddr.sin_port));


		  ev.events = EPOLLIN;	/* Only interested in input events */
		  ev.data.fd = connfd;
		  if (epoll_ctl (epfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
		    errExit ("epoll_ctl");
		}
	      else
		{
		  int s = read (evlist[j].data.fd, buf, MAX_BUF);
		  buf[s] = '\0';
		  if (s == -1)
		    errExit ("read");
		  if (s == 0)
		    {
		      close (evlist[j].data.fd);
		    }
		  if (s > 0)
		    write (evlist[j].data.fd, buf, strlen (buf));

		}
	    }
	}
    }
}
