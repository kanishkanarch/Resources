#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

void str_echo(int sockfd)
{
	ssize_t n;
	char buf[1000];
	while((n=read(sockfd,buf,1000))>0)
	{
	write(sockfd,buf,n);
	printf("%s\n",buf);
	}
}

static void	*doit(void *);		/* each thread executes this function */

int
main(int argc, char **argv)
{
	int				listenfd, connfd;
	socklen_t		addrlen, len;
	pthread_t tid1;
	struct sockaddr	*cliaddr;
	struct sockaddr_in servaddr;
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(atoi(argv[1]));
	bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
	listen(listenfd,10);

	cliaddr = malloc(addrlen);

	for ( ; ; ) {
		len = addrlen;
		connfd = accept(listenfd, cliaddr, &len);
		pthread_create(&tid1, NULL, &doit, (void *) connfd);
	}
}

static void *doit(void *arg)
{
	pthread_detach(pthread_self());
	str_echo((int) arg);	/* same function as before */
	close((int) arg);		/* we are done with connected socket */
	return(NULL);
}

