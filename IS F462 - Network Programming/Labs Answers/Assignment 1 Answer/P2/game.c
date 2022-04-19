#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<signal.h>
#include<unistd.h>
#include<error.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include<stddef.h>
#include<linux/prctl.h>
#include<limits.h>
#include<sys/types.h> 
#include<sys/ipc.h>

struct msgbuf{
	long mtype;
	int x;
};


int main(int argc, char *argv[]){
	if(argc != 2){
		printf("Incorrect no of arguments");
		exit(1);
	}

	prctl(PR_SET_PDEATHSIG, SIGHUP);

	struct msgbuf buf;
	int size,msgid;
	int flag = 1;
	int done=0;
	int N=atoi(argv[1]);
	int count=32767;
	pid_t next;
	char cwd[1024],path[1024];

	if (getcwd(cwd, sizeof(cwd)) != NULL)
		sprintf(path, "%s/parent.c", cwd);
	else{
		perror("getcwd() error");
		exit(0);
	}
	
	key_t key;
	key = ftok(path,'p');
	msgid = msgget(key, 0666|IPC_CREAT));
	size = sizeof(struct msgbuf) - sizeof(long);

	msgrcv(msgid, &buf, sizeof(buf), getpid(), 0);
	next = buf.x;

	key = ftok(path,'q');
	msgid = msgget(key, 0666|IPC_CREAT);
	size = sizeof(struct msgbuf) - sizeof(long);

	while(1){
		msgrcv(msgid, &buf, sizeof(buf), getpid(), 0);
		if(flag){
			if(count == buf.x){
				printf("i am a truthful process %d, only the truth that always wins\n",getpid());
				done = 1;
			}
			else if(buf.x == 0){
				printf("i am a foolish process %d, defeated\n",getpid());
				buf.x = N;
				flag = 0;
			}
			else if(buf.x == 0){
				buf.x = N;
			}
			else{
				buf.x--;
				count = buf.x;
			}
		}
		if(done == 1){
			buf.mtype = getppid();
			msgsnd(msgid,&buf,size,0);
			exit(0);
		}
		else{
			buf.mtype = next;
			msgsnd(msgid,&buf,size,0);
		}
	}
}
