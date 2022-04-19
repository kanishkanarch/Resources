#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<strings.h>
#include<pthread.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>
#include <arpa/inet.h> 
#include <netdb.h>

void runRequest(int sockfd);
void doJoin(int sockfd, char *clntname);
void doList(int sockfd);
void doUmsg(int sockfd, char *sendname, char *msg1);
void doBmsg(char *msg1);
void doLeav(int sockfd);

struct client{

	int sockfd;
	char name[50];
	char msg[1000];
};

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_t threads[5];
socklen_t addrlen, len;
int	listenfd, N = 0;
struct client *clients;

void main(int argc, char *argv[]){

	void thread_make(int);	//thread funciton prototype
	int i;
	struct sockaddr_in servaddr;
	
	if(argc != 2){
		printf("Invalid number of arguments!\n");
		exit(0);
	}
	N = atoi(argv[1]);
	clients = (struct client *) malloc(N*sizeof(struct client));
	for(i=0; i<N; ++i){
		clients[i].sockfd = -100;
	}

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");;
	servaddr.sin_port = htons(2000);
	
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenfd, 10);
		
	for(i=0; i<N; ++i){
		thread_make(i);
	}	
	
	while(1){
		pause();
	}
	
	printf("Server closing\n");
	close(listenfd);
	exit(0);
}

void thread_make(int i){
	
	void *thread_main(void *);
	pthread_create(&threads[i], NULL, &thread_main, (void *)i);
	return;		/* main thread returns */
}

void *thread_main(void *arg){
	
	int connfd;
	void runRequest(int);
	socklen_t clilen;
	struct sockaddr *cliaddr;

	cliaddr = malloc(addrlen);

	printf("Thread %d starting\n", (int) arg);
	while(1){
		clilen = addrlen;

		pthread_mutex_lock(&mlock);
		connfd = accept(listenfd, cliaddr, &clilen);
		pthread_mutex_unlock(&mlock);

		runRequest(connfd);      /* process request */
	}
}

void runRequest(int sockfd){

	ssize_t n1;
	char buf[1000];
	while((n1 = read(sockfd, buf, 1000)) > 0){

		printf("Read from client: %s\n", buf);
		char *c, *argVec[10];
		int j = 0, nargs = 0;
		c = buf;						
		argVec[nargs] = c;
		while(*c){
			if(j == 3){
				++c;
				if(*c == ' '){
					*c = '\0';
				}
				argVec[++nargs] = c+1;
			}	
			if(*c == '\r'){
				*c = '\0';
				++c;
				argVec[++nargs] = c+1;
			}
			++c;
			++j;
		}
	/*
		int p;
		for(p=0; p<nargs; ++p){
			printf("argVec %d: %s\n", p, argVec[p]);
		}
	*/
		if(strcmp("JOIN", argVec[0]) == 0){
			doJoin(sockfd, argVec[1]);
		}
	
		if(strcmp("LIST", argVec[0]) == 0){
			doList(sockfd);
		}
	
		if(strcmp("UMSG", argVec[0]) == 0){
			doUmsg(sockfd, argVec[1], argVec[2]);
		}
		
		if(strcmp("BMSG", argVec[0]) == 0){
			doBmsg(argVec[1]);
		}
		if(strcmp("LEAV", argVec[0]) == 0){
			doLeav(sockfd);
			break;
		}
	}
}

void doJoin(int sockfd, char *clntname){

	int i, flag = 0;
	for(i=0; i<N; ++i){
		if(clients[i].sockfd == -100){
			flag = 1;
			clients[i].sockfd = sockfd;
			strcpy(clients[i].name, clntname);
			strcpy(clients[i].msg, " ");
			printf("JOIN done. Client %d - sockfd: %d, name: %s, msg: %s\n", i, clients[i].sockfd, clients[i].name, clients[i].msg);	
			break;
		}
	}
	if(flag != 1){
		printf("Client space full\n");
	}
}

void doList(int sockfd){

	int i, x = 0;
	for(i=0; i<N; ++i){
		if(clients[i].sockfd != -100){
			printf("Client %d - name: %s\n", i, clients[i].name);
			write(sockfd, clients[i].name, 1000);
		}
		else ++x;
	}
	if(x == N){
		printf("No users online!\n");
	}
}

void doUmsg(int sockfd, char *sendname, char *msg1){
	
	int i, flag = 0;
	for(i=0; i<N; ++i){
		if(strcmp(clients[i].name, sendname) == 0){
			printf("Sending msg: %s, size: %d\n", msg1, strlen(msg1));
			write(clients[i].sockfd, msg1, 1000);
			flag = 1;
			break;
		}
	}
	if(flag != 1){
		char *str = "ERROR <not online>\n";
		write(sockfd, str, 1000);
	}	
}

void doBmsg(char *msg1){
	
	int i;
	printf("Sending BULK msg: %s, size: %d\n", msg1, strlen(msg1));
	for(i=0; i<N; ++i){
		if(clients[i].sockfd != -100){
			write(clients[i].sockfd, msg1, 1000);
		}
	}
}

void doLeav(int sockfd){

	int i, flag = 0;
	for(i=0; i<N; ++i){
		if(sockfd == clients[i].sockfd){
			flag = 1;
			clients[i].sockfd = -100;
			strcpy(clients[i].name, " ");
			strcpy(clients[i].msg, " ");
			close(sockfd);
			printf("LEAV done. Removed client at index: %d\n", i);	
			break;
		}
	}
	if(flag != 1){
		printf("Error in removing client!\n");
	}
}
