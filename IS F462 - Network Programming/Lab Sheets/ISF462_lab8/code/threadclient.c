#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>

void	*copyto(void *);

static int	sockfd;		/* global for both threads to access */
static FILE	*fp;

void
str_cli(FILE *fp_arg, int sockfd_arg)
{
	char		recvline[1000];
	pthread_t	tid;

	sockfd = sockfd_arg;	/* copy arguments to externals */
	fp = fp_arg;
	pthread_create(&tid, NULL, copyto, NULL);

	while (read(sockfd, recvline, 1000) > 0)
		fputs(recvline, stdout);
}

void *copyto(void *arg)
{
	char	sendline[1000];
        while (fgets(sendline, 1000, fp) != NULL)
		write(sockfd, sendline, strlen(sendline));

	shutdown(sockfd, SHUT_WR);	/* EOF on stdin, send FIN */

	return(NULL);
}


int main(int argc,char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(atoi(argv[2]));
	servaddr.sin_addr.s_addr=inet_addr(argv[1]);
	if((connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)))<0)
	printf("connect error\n");
	str_cli(stdin,sockfd);
	exit(0);
}
