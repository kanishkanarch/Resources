#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/un.h>

#define MAXLINE 1000
#define MAX_DATA 1000

int main(int argc, char **argv)
{
	int sockfd, mode;
	char data[MAX_DATA]="";
	struct sockaddr_un servaddr;
	struct msghdr msgh;
	struct iovec iov;

	if(argc != 3 || (atoi(argv[2]) != 0 && atoi(argv[2]) != 1 && atoi(argv[2]) != 2))
	{
		printf("Incorrect Syntax.\nUse: %s <Filename> <Access Mode (0 {O_RDONLY}, 1 {O_WRONLY}, 2 {O_RDWR})>\n",argv[0]);
		exit(1);
	}

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, "f2008387"); /* Connecting client to default path f2008387 where server is also started */

	connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;

	/* Send Filename and Access Mode to server */
	strcat(data, argv[1]);
	strcat(data, "#");
	strcat(data, argv[2]);
	strcat(data, "#");
	iov.iov_base = data;
	iov.iov_len = MAX_DATA;
	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;

	msgh.msg_control = NULL;
	msgh.msg_controllen = 0;

	if(sendmsg(sockfd, &msgh, 0) < 0)
	{
		perror("Error sending message");
		exit(1);	
	}
	else
	{
		printf("Client PID: %d. Sent Filename: %s, Access Mode: %s along with Credentials Successfully to Server\n", getpid(), argv[1], argv[2]);
	}

	if(recvfd(sockfd) == 0)
	{
		printf("Client PID: %d. Received File Descriptor successfully from Server\n", getpid());
	}
	else
	{
		printf("Client PID: %d. Not received File Descriptor from Server\n", getpid());
	}
	sleep(10);
	exit(0);
}

int recvfd(int sock)
{
	char buf[80]; /* space to read file name into */
	struct iovec vector; /* file name from the child */
	struct msghdr msg; /* full message */
	struct cmsghdr * cmsg; /* control message with the fd */
	int fd;
	char line[1000];

	/* set up the iovec for the file name */
	vector.iov_base = buf;
	vector.iov_len = 80;

	/* the message we're expecting to receive */
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vector;
	msg.msg_iovlen = 1;

	/* dynamically allocate so we can leave room for the file descriptor */
	cmsg = alloca(sizeof(struct cmsghdr) + sizeof(fd));
	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
	msg.msg_control = cmsg;
	msg.msg_controllen = cmsg->cmsg_len;

	if (!recvmsg(sock, &msg, 0))
	{
		perror("recvmsg() failed"); 
		return 1;
	}
	if(strcmp("Invalid UserID",(char *) vector.iov_base) == 0)
	{
		printf("Client PID: %d. Error Message Invalid UserID received from Server\n", getpid());
		return 1;
	}
	else
	{
		printf("Client PID: %d. Received File Descriptor for file %s\n", getpid(), (char *) vector.iov_base);
	}

	/* grab the file descriptor from the control structure */
	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));

	//printing first 100Bytes of the request file on to stdout
	read(fd, line, 100);
	printf("Client PID: %d. First 100Bytes of requested file is:\n%s\n", getpid(), line);

	return 0;
}