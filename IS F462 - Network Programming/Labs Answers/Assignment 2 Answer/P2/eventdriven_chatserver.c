#include<sys/epoll.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/msg.h>
#include<unistd.h>
#include<strings.h>
#include<sys/select.h>
#include<errno.h>
#include<signal.h>
#include<string.h>
#include<pthread.h>
#include<netinet/in.h>
#include<arpa/inet.h> 
#include<netdb.h>

typedef struct msg_buf{
	long mtype;
	char mtext[100];
} msg_buf;

struct client{
	int sockfd;
	char name[50];
	char msg[1000];
};

void doJoin(int sockfd, char *clntname);
void doList(int sockfd);
void doUmsg(int sockfd, char *sendname, char *msg1);
void doBmsg(char *msg1);
void doLeav(int sockfd);

int msqid, N = 100, epfd;
struct client *clients;
pthread_t thread1;
struct epoll_event ev;
struct epoll_event evlist[100];
 
void main(int argc, char *argv[]){

	int listenfd, clilen, i, j, ready;
	struct sockaddr_in cliaddr, servaddr;
	
	//Creating the clients list
	clients = (struct client *) malloc(N*sizeof(struct client));
	for(i=0; i<N; ++i){
		clients[i].sockfd = -100;
	}

	//Creating message queue
//	key_t key = ftok("/Desktop/", 'B');
	if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0666)) == -1){
		perror("msgget");
	}
	printf("IN MAIN MSQID: %d\n", msqid);

	//Creating second thread for processing
	void *thread_main(void *);
	pthread_create(&thread1, NULL, &thread_main, (void *)0);
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(listenfd, F_SETFL, O_NONBLOCK);  		//set to non-blocking

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(atoi(argv[1]));

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0){
		printf("Error in bind\n");
		exit(1);
	}
	listen(listenfd, 10);

	epfd = epoll_create(100);
	if (epfd == -1){
		printf("Error in epoll_create\n");
		exit(1);
	}
	ev.events = EPOLLIN;		/* Only interested in input events */
	ev.data.fd = listenfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1){
		printf("Error in epoll_ctl\n");
		exit(1);
	}
	
	printf("Entering while(1)\n");
	while(1){
	
		ready = epoll_wait(epfd, evlist, 100, -1);
		if (ready == -1){
			printf("Error in epoll_wait\n");
			exit(1);
		}
		
		for(j = 0; j < ready; j++){
	
			if(evlist[j].events & EPOLLIN){
	
				if(evlist[j].data.fd == listenfd){

					clilen = sizeof(cliaddr);
					char ip[128];

					memset(ip, '\0', 128);
					int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
					if(cliaddr.sin_family == AF_INET){
						if(inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, 128) == NULL){
							printf("Error in inet_ntop\n");
							exit(1);
						}
					}
					printf("New client ip: %s, port %d\n", ip, ntohs(cliaddr.sin_port));

					ev.events = EPOLLIN;	/* Only interested in input events */
					ev.data.fd = connfd;
					if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) == -1){
						printf("Error in epoll_ctl for connfd\n");
					}
				}
				else{
					msg_buf msg123;
					msg123.mtype = 1;
					
					sprintf(&msg123.mtext, "%d;READ", evlist[j].data.fd);
					ev.events = EPOLLOUT;
					ev.data.fd = evlist[j].data.fd;
					if (epoll_ctl(epfd, EPOLL_CTL_MOD, evlist[j].data.fd, &ev) == -1){
						printf("Error in epoll_ctl for connfd\n");
					}	
					
					if(msgsnd(msqid, &msg123, strlen(msg123.mtext), 0) == -1){
						//printf("Error in msgsnd\n");
						perror("msgsnd");
					}
				}
			}
		}
	}	//end of while(1)
}

void *thread_main(void *arg){

	//printf("In thread_main\n");	
	
	size_t max = 100;
	int x = 0;
	//printf("MSQID: %d\n", msqid);
	while(1) {
		msg_buf msg1;
		msg1.mtype = 1;
		strcpy(msg1.mtext, "");
		if(msgrcv(msqid, &msg1, max, 1, 0) == -1){
			perror("msgrcv");
		}
		printf("%dMSG from msgq: %s\n", ++x, msg1.mtext);
		int k = 0;
		char *c, *argVec[4];
		argVec[0] = msg1.mtext;
		for (c = msg1.mtext; *c; c++) {
			if (*c == ';') {
				*c = '\0';
				argVec[++k] = c + 1;
			}
			if (k > 2) break;
		}
		argVec[++k] = NULL;
	
		int fd = atoi(argVec[0]);

		if (strcmp("READ",argVec[1]) == 0) {
			char command[1000];
			int s = read(fd, command, 1000);
			if (s > 0) {
				msg_buf msg2;
				msg2.mtype = 1;
				sprintf(&msg2.mtext, "%d;PROC;%s", fd, command);
					
				if(msgsnd(msqid, &msg2, strlen(msg2.mtext), 0) == -1){
					//printf("Error in msgsnd\n");
					perror("msgsnd");
				}
			}
		}
		if (strcmp("PROC",argVec[1]) == 0) {
			char *c = argVec[2], *procVec[10];
			int j = 0, nargs = 0;
			procVec[nargs] = c;
			while(*c){
				if(j == 3){
					++c;
					if(*c == ' '){
						*c = '\0';
					}
					procVec[++nargs] = c + 1;
				}	
				if(*c == '\r'){
					*c = '\0';
					++c;
					if (*(c + 1))
						procVec[++nargs] = c + 1;
				}
				++c;
				++j;
			}
			procVec[++nargs] = NULL;
			
			int p;
			for(p=0; procVec[p] != NULL; ++p){
				printf("procVec %d: %s\n", p, procVec[p]);
			}

			if(strcmp("JOIN", procVec[0]) == 0){
				
				doJoin(fd, procVec[1]);
			}
	
			if(strcmp("LIST", procVec[0]) == 0){
				doList(fd);
			}
	
			if(strcmp("UMSG", procVec[0]) == 0){
				doUmsg(fd, procVec[1], procVec[2]);
			}
		
			if(strcmp("BMSG", procVec[0]) == 0){
				doBmsg(procVec[1]);
			}
			if(strcmp("LEAV", procVec[0]) == 0){
				doLeav(fd);
			}
			else{
				ev.events = EPOLLIN;
				ev.data.fd = fd;
				if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1){
					printf("Error in epoll_ctl for connfd\n");
				}
				printf("Changd to epoll_in, fd: %d\n", fd);
			}
		}
		if (strcmp("WRIT",argVec[1]) == 0) {
			write(fd, argVec[2], 1000);
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
			strcpy(clients[i].msg, "");
			printf("JOIN done. Client %d - sockfd: %d, name: %s\n", i, clients[i].sockfd, clients[i].name);
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
			
			msg_buf msg1;
			msg1.mtype = 1;
			sprintf(&msg1.mtext, "%d;WRIT;%s", sockfd, clients[i].name);
			
			if(msgsnd(msqid, &msg1, strlen(msg1.mtext), 0) == -1){
				perror("msgsnd");
			}
			//write(sockfd, clients[i].name, 1000);
		}
		else ++x;
	}
	if(x == N){
		printf("No users online!\n");
	}
}

void doUmsg(int sockfd, char *sendname, char msgtosend[]){
	
	int i, flag = 0;
	for(i=0; i<N; ++i){
		if(strcmp(clients[i].name, sendname) == 0){
			//printf("Sending msg: %s, size: %d\n", msgtosend, strlen(msgtosend));
			msg_buf msg2;
			msg2.mtype = 1;
			sprintf(&msg2.mtext, "%d;WRIT;%s", clients[i].sockfd, msgtosend);
			
			//printf("S1: %s\n", msg2.mtext);
			if(msgsnd(msqid, &msg2, strlen(msg2.mtext), 0) == -1){
				perror("msgsnd");
			}
			//write(clients[i].sockfd, msg1, 1000);
			flag = 1;
			break;
		}
	}
	if(flag != 1){
		char *str = "ERROR <not online>\n";
		write(sockfd, str, 1000);
	}	
}

void doBmsg(char *msgtosend){
	
	int i;
	printf("Sending BULK msg: %s, size: %d\n", msgtosend, strlen(msgtosend));
	for(i=0; i<N; ++i){
		if(clients[i].sockfd != -100){
			msg_buf msg2;
			msg2.mtype = 1;
			sprintf(&msg2.mtext, "%d;WRIT;%s", clients[i].sockfd, msgtosend);
			
			if(msgsnd(msqid, &msg2, strlen(msg2.mtext), 0) == -1){
				perror("msgsnd");
			}
			//write(clients[i].sockfd, msg1, 1000);
		}
	}
}

void doLeav(int sockfd){

	int i, flag = 0;
	for(i=0; i<N; ++i){
		if(sockfd == clients[i].sockfd){
			flag = 1;
			clients[i].sockfd = -100;
			strcpy(clients[i].name, "");
			strcpy(clients[i].msg, "");
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL) == -1){
				printf("Error in epoll_ctl for connfd\n");
			}
			close(sockfd);
			printf("LEAV done. Removed client at index: %d\n", i);	
			break;
		}
	}
	if(flag != 1){
		printf("Error in removing client!\n");
	}
}
