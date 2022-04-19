#include <alloca.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include "sockutil.h"

void parentProcess(int sock, char* filename, int i);
void childProcess(int sock);
void printupper(char* buf, struct msghdr msg);

int main(int argc, char ** argv) {
	int socks[4][2];
	int status;

    	if (argc != 2) {
        	fprintf(stderr, "only a single argument is supported\n");
        	return 1;
    	}
	
	/* Create the sockets. The first is for the parent and the
       	second is for the child (though we could reverse that
       	if we liked. */
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, socks[0])) 
        	return 1;
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, socks[1])) 
	       	return 1;
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, socks[2])) 
	       	return 1;
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, socks[3])) 
        	return 1;

	pid_t ret1 = fork();
	pid_t ret2 = fork();
	pid_t ret3 = fork();
	pid_t ret4 = fork();

    	if (ret1 == 0) {
		/* child */
		close(socks[0][0]);
		childProcess(socks[0][1]);
	}
    	else if (ret2 == 0) {
		/* child */
		close(socks[1][0]);
		childProcess(socks[1][1]);
	}
    	else if (ret3 == 0) {
		/* child */
		close(socks[2][0]);
		childProcess(socks[2][1]);
	}
    	else if (ret4 == 0) {
		/* child */
		close(socks[3][0]);
		childProcess(socks[3][1]);
	}
	else {
		/* parent */
		close(socks[0][1]);
		close(socks[1][1]);
		close(socks[2][1]);
		close(socks[3][1]);
    		parentProcess(socks[0][0], argv[1], 0);
    		parentProcess(socks[1][0], argv[1], 1);
    		parentProcess(socks[2][0], argv[1], 2);
    		parentProcess(socks[3][0], argv[1], 3);

    		/* reap the child */
    		wait(&status);

    		if (WEXITSTATUS(status))
		fprintf(stderr, "child failed\n");
	}
    	return 0;
}

void parentProcess(int sock, char* filename, int i) {
	struct iovec vector;
	/* some data to pass w/ the fd */
	struct msghdr msg;
	/* the complete message */
	struct cmsghdr * cmsg;
	/* the control message, which will */
	/* Send the file name down the socket, including the trailing '\0' */
	int fd;
	if ((fd = open(filename, O_RDONLY)) < 0) {
		perror("open");
		return;
	}
	char range[10];
	sprintf(range, "%d-%d", i*4, i*4 + 3);
	vector.iov_base = range;
	vector.iov_len = strlen(range) + 1;
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
		return;
}

void childProcess(int sock) {
	char buf[80];
	/* space to read file name into */
	struct iovec vector; /* file name from the parent */
	struct msghdr msg;
	/* full message */
	struct cmsghdr * cmsg;
	/* control message with the fd */
	int fd;
	/* set up the iovec for the file name */
	vector.iov_base = buf;
	vector.iov_len = 80;
	/* the message we're expecting to receive */
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vector;
	msg.msg_iovlen = 1;
	/* dynamically allocate so we can leave room for the file
	descriptor */
	cmsg = alloca(sizeof(struct cmsghdr) + sizeof(fd));
	cmsg->cmsg_len = sizeof(struct cmsghdr) + sizeof(fd);
	msg.msg_control = cmsg;
	msg.msg_controllen = cmsg->cmsg_len;
	if (!recvmsg(sock, &msg, 0))
		return;
	//printf("got file descriptor for '%s'\n", (char *) vector.iov_base);
	/* grab the file descriptor from the control structure */
	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));	
	char buf1[80];
	read(fd, buf1, 80);
	printupper(buf1, msg);
}

void printupper(char* buf, struct msghdr msg) {
	char *range = msg.msg_iov->iov_base, *x;
	//printf("%s:", range);
	//printf("%s\n", buf);
	for (x = range; *x != '-'; x++);
	*x = '\n';
	x++;
	int start = atoi(range);
	int end = atoi(x);
	int i;
	for (i = start; i <= end; i++) {
		printf("%d : %c\n", getpid(), toupper(buf[i]));
	}
}
