#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<signal.h>
#include<unistd.h>
#include<error.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include<stddef.h>
#include<sys/types.h> 
#include<sys/ipc.h>

struct msgbuf{
	long mtype;
	int x;
};

int fork_child(int i, int N);
void userHandler(){
	return;
}

int main(int argc, char *argv[]){
	if(argc != 3){
		printf("Incorrect no of arguments");
		exit(1);
	}

	int i,size,msgid,msgid2,
	int N=atoi(argv[1]);
	int K=atoi(argv[2]);
	pid_t *pids;
	pids = malloc((N+1)*sizeof(pid_t));
	struct msgbuf buf;
	char cwd[1024],path[1024];

	if (getcwd(cwd, sizeof(cwd)) != NULL)
		sprintf(path, "%s/parent.c", cwd);
	else{
		perror("getcwd() error");
		exit(1);
	}

	key_t key;
	key = ftok(path,'q');
	msgid = msgget(k, 0666|IPC_CREAT);
	key = ftok(path,'p');
	msgid2 = msgget(k, 0666|IPC_CREAT);
	size = sizeof(struct msgbuf) - sizeof(long);

	for(i=0; i<N; i++){
		pids[i] = fork();
		if(pids[i] == 0){
			signal(SIGUSR1, userHandler);
			pause();
			char number[10];
			sprintf(number,"%d",N);
			execlp("./game","./game", number, NULL);
			exit(0);
		}
	}
	pids[N] = pids[0];

	sleep(1);
	
	for(i=0; i<N; i++){
		kill(pids[i], SIGUSR1);
	}

	size = sizeof(struct msgbuf) - sizeof(long);
	for(i=0;i<N;i++){
		buf.mtype = pids[i];
		buf.x = pids[i+1];
		msgsnd(msgid2,&buf,size,0)
	}
	
	buf.mtype = pids[0];
	buf.x = K;
	msgsnd(msgid,&buf,size,0)

	msgrcv(msgid, &buf, sizeof(buf), getpid(), 0);

	msgctl(msgid2, IPC_RMID, NULL);
	msgctl(msgid, IPC_RMID, NULL);

	exit(0);
}

int fork_child(int i, int N){	
	
}
