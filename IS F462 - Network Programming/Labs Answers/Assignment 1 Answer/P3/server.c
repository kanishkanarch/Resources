//HEADER FILES
#include<sys/socket.h>
#include <arpa/inet.h> 
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//MACROS
#define BUF_SIZE 200
#define RCV_SIZE 100
#define BACKLOG 5

//FUNCTION PROTOTYPES
void ExitWithErrors(char *);
void HandleTCPClient(int, char *);

//MAIN FUNCTION
void main(int argc, char *argv[]){

	socklen_t clntAddrLen;
	struct sockaddr_in addr;
	int sfd, cfd;
	ssize_t numRead;
	char prevQuery[500];
	unsigned short server_port;							

	server_port = atoi(argv[1]);						//Setting port

	memset(&addr, 0, sizeof(addr));
	sfd 					= socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	addr.sin_family 		= AF_INET;
	addr.sin_addr.s_addr 	= inet_addr("127.0.0.1");
	addr.sin_port 			= htons(server_port);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        ExitWithErrors("bind() failed");
    if (listen(sfd, BACKLOG) < 0)
        ExitWithErrors("listen() failed");

	while(1){
		printf("Waiting for client...\n");
        clntAddrLen = sizeof(addr);
        if ((cfd = accept(sfd, (struct sockaddr *) &addr, &clntAddrLen)) < 0)
            ExitWithErrors("accept() failed");
        printf("Handling client %s\n", inet_ntoa(addr.sin_addr));
        HandleTCPClient(cfd, prevQuery);
    }
}	//end of main()

void HandleTCPClient(int cfd, char *prev){

	char echoBuffer[RCV_SIZE], sendstr[RCV_SIZE];        
	int recvMsgSize, i;                    
	char *c, *queryArgs[4];
	FILE *fp;
		
	if ((recvMsgSize = recv(cfd, echoBuffer, RCV_SIZE, 0)) < 0)
		ExitWithErrors("recv() failed");

	i = 0;
	c = echoBuffer;
	queryArgs[0] = c;
	while(*c){
		if(*c == 32) {
			*c = '\0';
			queryArgs[++i] = c+1;
		}
		++c;
	}

	char str[RCV_SIZE];
	if(!strcmp(queryArgs[2], "y") || !strcmp(queryArgs[2], "Y")){
		strcpy(str, prev);
		strcat(str, "|grep ");
		strcat(str, queryArgs[1]);
	}
	else{
		strcpy(str, "grep ");
		strcat(str, queryArgs[1]);
		strcat(str, " ");
		strcat(str, queryArgs[3]);
	}

	fp = popen(str, "r");
	while(fgets(sendstr, RCV_SIZE, fp) != NULL){

		strcpy(prev, str);	
		if (send(cfd, sendstr, RCV_SIZE, 0) != RCV_SIZE)
	    	ExitWithErrors("send() failed");
	}

	pclose(fp);
	close(cfd);
}

void ExitWithErrors(char *msg){

perror(msg);
exit(1);
}

