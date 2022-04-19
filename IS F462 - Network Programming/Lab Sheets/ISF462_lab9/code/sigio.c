
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/fcntl.h>

#define LISTENQ 15
#define MAXLINE 10 

char buf[MAXLINE];


int listenfd;			//global var so that signal handlers can access them.
int connfd;
static void
sigioListenHandler (int sig, siginfo_t * si, void *ucontext)
{
  printf ("signal no:%d, for fd:%d,event code:%d,  event band:%ld\n",
	  si->si_signo, (int) si->si_fd, (int) si->si_code,
	  (long) si->si_band);
  fflush (stdout);
  if (si->si_code == POLL_IN)
    {
      struct sockaddr_in cliaddr;
      int clilen = sizeof (cliaddr);
      char ip[128];
      int n = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
inet_ntop (AF_INET, &(cliaddr.sin_addr), ip, 128);
      printf ("Connection accepted from %s:%d, connfd is %d\n",
	      ip,
	      ntohs (cliaddr.sin_port), n);
      if (n > 0)
	connfd = n;
      fcntl (connfd, F_SETOWN, getpid ());
      int flags = fcntl (connfd, F_GETFL);	/* Get current flags */
      fcntl (connfd, F_SETFL, flags | O_ASYNC | O_NONBLOCK);
      fcntl (connfd, F_SETSIG, SIGRTMIN + 2);
    }
  if (sig == SIGIO)
    printf ("Real time signalQ overflow");

}


static void
sigioConnHandler (int sig, siginfo_t * si, void *ucontext)
{
  printf ("signal no:%d, for fd:%d, event code:%d,  event band:%ld\n",
	  si->si_signo, (int) si->si_fd, (int) si->si_code,
	  (long) si->si_band);
  fflush (stdout);
  if (si->si_code == POLL_OUT)
    {
      //output possible
      printf ("POLL_OUT event occured\n");
    }
  if (si->si_code == POLL_IN)
    {				//input available
      printf ("POLL_IN event occured\n");
      int n = read (si->si_fd, buf, MAXLINE);
      if (n == 0)
	{
	  close (si->si_fd);
	  printf ("EOF read. Socket %d closed\n", si->si_fd);
	}
      else if (n > 0)
	{
	  buf[n] = '\0';
	  printf ("Data from connfd %d:len %d: %s \n", si->si_fd,n, buf);
	  int k;
	  if (write (si->si_fd, buf, n) < 0)
	    {
	      perror ("write");
	    }
	}
      else
	{
	  //printf ("Socket %d error\n", si->si_fd);
	  //perror ("socket");
	}
    }
  if (si->si_code == POLL_ERR)
    {
      int err;
      int errlen = sizeof (int);
      getsockopt (si->si_fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
      if (err > 0)
	printf ("error on socket %d : %s", si->si_fd, strerror (err));
    }
  fflush (stdout);
}



main (int argc, char **argv)
{

  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;
  struct sigaction sa, sa1;
  memset (&sa, '\0', sizeof (sa));
  memset (&sa, '\0', sizeof (sa1));
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = &sigioListenHandler;	//for accepting new conn
  sigaction (SIGIO, &sa, NULL);
  sigaction (SIGRTMIN + 1, &sa, NULL);

  sigemptyset (&sa1.sa_mask);
  sa1.sa_flags = SA_SIGINFO;
  sa1.sa_sigaction = &sigioConnHandler;	//for reading data
  sigaction (SIGRTMIN + 2, &sa1, NULL);

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi (argv[1]));
  bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr));
  listen (listenfd, LISTENQ);

  fcntl (listenfd, F_SETOWN, getpid ());
  int flags = fcntl (listenfd, F_GETFL);	/* Get current flags */
  fcntl (listenfd, F_SETFL, flags | O_ASYNC | O_NONBLOCK);	//set signal driven IO
  fcntl (listenfd, F_SETSIG, SIGRTMIN + 1);	//replace SIGIO with realtime signal

  for (;;)
    {
      pause ();
    }

}
