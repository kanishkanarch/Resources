#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     

#define RCVBUFSIZE 100   

void ExitWithError(char *errorMessage);  
int syntax(char[]);

int main(int argc, char *argv[])
{
    int sock, i, bytesRcvd, totalBytesRcvd;                           
    struct sockaddr_in echoServAddr; 
    unsigned short echoServPort;     
    char *queryArgs[4], *servIP, *c;                
    char echoBuffer[RCVBUFSIZE], query[100];     
    unsigned int echoStringLen;      
	
    if ((argc < 2) || (argc > 3))        {
       fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = "127.0.0.1";             								//Fixing address
    echoServPort = atoi(argv[2]); 									//Setting port
    
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ExitWithError("socket() failed");
  
    memset(&echoServAddr, 0, sizeof(echoServAddr));     
    echoServAddr.sin_family      = AF_INET;                     
    echoServAddr.sin_addr.s_addr = inet_addr("127.0.0.1");   
    echoServAddr.sin_port        = htons(echoServPort); 

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        ExitWithError("connect() failed");

	L1:
	printf("Enter query:\n");
	gets(query);
	
	if( syntax(query) == 0 ){
		
		printf("Syntax incorrect! Usage:\n");
		printf("query <word> <Y/N> [filename]\n");
		goto L1;		
	}
	
	//printf("Query LEN:%d\n", strlen(query));
	
    if (send(sock, query, strlen(query) + 1, 0) != strlen(query) + 1)
        ExitWithError("send() sent a different number of bytes than expected");

	bytesRcvd = 1;
    while ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE, 0)) > 0)
    {
        printf("%s\n", echoBuffer);      
    }

    close(sock);
    exit(0);
}

void ExitWithError(char *msg){

perror(msg);
exit(1);
}

int syntax(char query[]){

	char *queryArgs[4], *c;
	int i = 0;
	
	char q[100];
	strcpy(q, query);
	c = q;
	queryArgs[0] = c;
	while(*c){
		if(*c == 32) {
			*c = '\0';
			
			if(*(c+1) != 32){
			
				if(i > 2) return 0; 
				queryArgs[++i] = c+1;
			}
		}
		++c;
	}
	
	if(i < 2) return 0;
	if(strcmp(queryArgs[0], "query") != 0) return 0;
	if((strcmp(queryArgs[2], "y") != 0) && (strcmp(queryArgs[2], "Y") != 0) && (strcmp(queryArgs[2], "n") != 0) && (strcmp(queryArgs[2], "N") != 0)) return 0;
	if(( (strcmp(queryArgs[2], "n") == 0) || (strcmp(queryArgs[2], "N") == 0) ) && i != 3) return 0;
	
	return 1; 
}
