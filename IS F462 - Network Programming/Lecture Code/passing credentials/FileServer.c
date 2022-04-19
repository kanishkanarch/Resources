#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/syscall.h>

#define MAX_DATA 1000

void* processclient(void *); /* Each thread executes this function */
int isvaliduid(long); /* To check if uid of the client is valid or not  */
int sendfd(char *, char *, int); /* Open the requested file and send the file descriptor to the client */

int main()
{
	int listenfd, connfd, choice, uid;
	socklen_t addrlen, len;
	pthread_t tid1;
	struct sockaddr	*cliaddr;
	struct sockaddr_un servaddr;

	printf("1. Enter list of allowed Client UID's manually\n2. Generate list of all UID's on this computer automatically\nEnter Choice: ");
	scanf("%d", &choice);
	if(choice == 1)
	{
		FILE *fp;
		fp = fopen("uidlist.txt","w");
		printf("Enter the allowed Client UID's\n");
		scanf("%d", &uid);
		while(uid != -1)
		{
			fprintf(fp, "%d\n", uid);
			scanf("%d", &uid);
		}
		fclose(fp);
	}
	else
	{	
		FILE *fp;
		/* Clear the uidlist.txt file of previous entries */
		fp = fopen("uidlist.txt","w");
		fclose(fp);
		/* Generate list of all userid's on this computer and store them in uidlist.txt file for validating clients later */
		system("awk -F: '$6 ~ /home/ {print $3}' /etc/passwd >> uidlist.txt");
	}

	listenfd=socket(AF_LOCAL, SOCK_STREAM, 0);

	unlink("f2008387");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family=AF_LOCAL;
	strcpy(servaddr.sun_path, "f2008387"); /* Setting server path to default path f2008387 in the current directory */

	if(bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind() failed");
		exit(1);
	}

	printf("Server PID: %d. Started Server at default path f2008387\n", getpid());

	listen(listenfd,10);

	cliaddr = malloc(addrlen);

	for (;;)
	{
		len = addrlen;
		if((connfd = accept(listenfd, (struct sockaddr *) cliaddr, &len)) < 0)
		{
			perror("accept() failed");
			continue;
		}
		printf("Server PID: %d. Received new request\n", getpid());
		pthread_create(&tid1, NULL, &processclient, (void *)&connfd);
	}
}

void* processclient(void *arg)
{
	pthread_detach(pthread_self());
	printf("Thread PID: %d. Processing Client inside new thread\n", (int)syscall(SYS_gettid));

	struct msghdr msgh;
	struct iovec iov;
	struct ucred *ucredp, ucred;
	int lfd, sfd, optval, opt;
	char data[MAX_DATA]="", *filename=NULL, *mode=NULL;
	ssize_t nr;
	union {
		struct cmsghdr cmh;
		char   control[CMSG_SPACE(sizeof(struct ucred))]; /* Space large enough to hold a ucred structure */
	} control_un;

	struct cmsghdr *cmhp=NULL;
	socklen_t len;

	optval = 1;

	/* Set SO_PASSCRED socket option for receving credentials of other processes */
	if (setsockopt(*(int *)arg, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1)
	{
		perror("setsockopt() failed");
		exit(1);
	}

	/* Set 'control_un' to describe ancillary data that we want to receive */
	control_un.cmh.cmsg_len = CMSG_LEN(sizeof(struct ucred));
	control_un.cmh.cmsg_level = SOL_SOCKET;
	control_un.cmh.cmsg_type = SCM_CREDENTIALS;

	/* Set 'msgh' fields to describe 'control_un' */
	msgh.msg_control = control_un.control;
	msgh.msg_controllen = sizeof(control_un.control);

	/* Set fields of 'msgh' to point to buffer used to receive (real) data read by recvmsg() */
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	iov.iov_base = data;
	iov.iov_len = MAX_DATA;

	msgh.msg_name = NULL;
	msgh.msg_namelen = 0;

	/* Receive real plus ancillary data */
	nr = recvmsg(*(int *)arg, &msgh, 0);
	if (nr == -1)
	{
		perror("recvmsg() failed");
		exit(1);
	}
	else if (nr > 0)
	{
		filename = strtok(data, "#");
		mode = strtok(NULL, "#");
		printf("Thread PID: %d. Received Filename: %s and Mode: %s\n", (int)syscall(SYS_gettid), filename, mode);
	}

	/* Extract credentials information from received ancillary data */
	cmhp = CMSG_FIRSTHDR(&msgh);
	if (cmhp == NULL || cmhp->cmsg_len != CMSG_LEN(sizeof(struct ucred)))
	{
		perror("bad cmsg header / message length");
		exit(1);
	}
	if (cmhp->cmsg_level != SOL_SOCKET)
	{
		perror("cmsg_level != SOL_SOCKET");
		exit(1);
	}
	if (cmhp->cmsg_type != SCM_CREDENTIALS)
	{
		perror("cmsg_type != SCM_CREDENTIALS");
		exit(1);
	}
	ucredp = (struct ucred *) CMSG_DATA(cmhp);

	printf("Thread PID: %d. Received Credentials pid: %ld, uid: %ld, gid: %ld\n", (int)syscall(SYS_gettid), (long) ucredp->pid, (long) ucredp->uid, (long) ucredp->gid);

	if(isvaliduid(ucredp->uid))
	{
		//send file descriptor to client
		printf("Thread PID: %d. Uid of Client is Valid uid\n", (int)syscall(SYS_gettid));
		if(sendfd(filename, mode, *(int *)arg) == 0)
		{
			printf("Thread PID: %d. sendfd() successful. Sent requested File Descriptor to Client successfully\n", (int)syscall(SYS_gettid));
		}
		else
		{
			printf("Thread PID: %d. sendfd() unsuccessful. File Descriptor not sent to Client\n", (int)syscall(SYS_gettid));
			exit(1);
		}
	}
	else
	{
		//send Invalid UserID errormsg
		printf("Thread PID: %d. Uid of Client is Invalid uid\n", (int)syscall(SYS_gettid));
		send(*(int *)arg,"Invalid UserID",15,0);
		exit(1);
	}
	sleep(10);
	recvmsg(*(int *)arg, &msgh, 0);
	close((int) arg); /* we are done with connected socket */
	return(NULL);
}

int isvaliduid(long uid)
{
	FILE *fp;
	long tempuid;
	fp = fopen("uidlist.txt","r");
	while(!feof(fp))
	{
		fscanf(fp,"%ld",&tempuid);
		if(tempuid == uid) return 1;
	}
	return 0;
}

int sendfd(char *filename, char *mode, int sock)
{
	int fd;
	struct iovec vector;
	struct msghdr msg;
	struct cmsghdr * cmsg;

	/* Open the file whose descriptor will be passed */
	if ((fd = open(filename, atoi(mode))) < 0) {
		perror("open() failed");
		return 1;
	}
	else
	{
		printf("Thread PID: %d. Opened the File requested by the Client\n", (int)syscall(SYS_gettid));
	}

	/* Send the file name down the socket, including the trailing '\0' */
	vector.iov_base = filename;
	vector.iov_len = strlen(filename) + 1;

	/* Put together the first part of the message. Include the file name iovec */
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vector;
	msg.msg_iovlen = 1;

	/* Now for the control message. We have to allocate room for the file descriptor. */
	cmsg = alloca(sizeof(struct cmsghdr) + sizeof(fd));
	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	/* copy the file descriptor onto the end of the control message */
	memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

	msg.msg_control = cmsg;
	msg.msg_controllen = cmsg->cmsg_len;

	if (sendmsg(sock, &msg, 0) != vector.iov_len)
	{
		perror("sendmsg() failed");
		return 1;
	}

	return 0;
}