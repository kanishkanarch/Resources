//HEADER FILES
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<stdlib.h>
#include<setjmp.h>
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

//GLOBAL VARIABLES
int shellReturn = 0, waitAction = 1, ijid=0;
static jmp_buf env;

//STRUCT TYPE FOR JOBS
typedef struct jobs{
	pid_t pid;
	int jid;
	int status;
} job;
job jobs[1000];

//FUNCTION PROTOTYPES
void add(pid_t pid);
void rem(pid_t pid);
void print();
void childHandler(int signum);
void execSigIntMain(int);
void execSigTSTPMain(int signo);

//MAIN FUNCTION
void main(int argc, char  *argv[]){

	//Blocking signals at shell command line
	sigset_t blockSet, prev, pending;
	sigemptyset(&blockSet);
	sigaddset(&blockSet, SIGINT);
	sigaddset(&blockSet, SIGTSTP);
	sigaddset(&blockSet, SIGKILL);
	if(sigprocmask(SIG_BLOCK, &blockSet, &prev) == -1){
		printf("Error in sigprocmask()!\n");
		exit(0);
	}

	//Setting signal handlers
	if(signal(SIGINT, execSigIntMain) == SIG_ERR){
		printf("Error in SIGINT signal()!\n");
		exit(0);
	}
	if(signal(SIGTSTP, execSigIntMain) == SIG_ERR){
		printf("Error in SIGTSTP signal()!\n");
		exit(0);
	}

	pid_t childPid;

	while(1){
		//Resetting variables
		shellReturn = 0;
		waitAction = 1;

		int i=0, status, err, inputSize;
		char command[]="/bin/", input[100];
		char *c, *argVec[10], *envVec[] = { "GREET=salut", "BYE=adieu", NULL };

		printf("Shell>");
		gets(input);

		if(exitStatus(input) == 1) break;									//for 'exit' command entered
		if(strcmp(input, "jobs") == 0) {print(); continue;}					//for 'jobs' command
		if(strcmp(input, "clear") == 0) {clearTerminal(); continue;}		//for 'clear'command
		
		inputSize = strlen(input);
		if(input[inputSize-1] == '&'){
			waitAction = 0;
			input[inputSize-1] = '\0';
		}

		c = input;						
		argVec[0] = c;
		while(*c){						//splitting command into arguments
			if(*c == 32) {
				*c = '\0';
				argVec[++i] = c+1;
			}
			++c;
		}
		argVec[++i] = NULL;
		strcat(command, argVec[0]);

		if(strcmp(argVec[0], "kill") == 0){							//for 'kill <jid>' command
			kill(jobs[atoi(argVec[1])-1].pid, SIGKILL);
			continue;
		}
		if(strcmp(argVec[0], "fg") == 0){							//for 'fg <jid>' command
			kill(jobs[atoi(argVec[1])-1].pid, SIGSTOP);
			kill(jobs[atoi(argVec[1])-1].pid, SIGCONT);
			rem(jobs[atoi(argVec[1])-1].pid);
			wait(&status);			
			continue;
		}
		if(strcmp(argVec[0], "bg") == 0){							//for 'bg <jid>' command
			kill(jobs[atoi(argVec[1])-1].pid, SIGCONT);
			rem(jobs[atoi(argVec[1])-1].pid);
			wait(&status);			
			continue;
		}

		signal(SIGCHLD, childHandler);
		switch(childPid = fork()){
			case -1: 
				printf("Child process not created!\n");
				break;
	
			case 0:
				if(waitAction == 1){
					sigprocmask(SIG_UNBLOCK, &blockSet, &prev);			//Unblocking signals for foreground processes
				}
				else{
					signal(SIGCHLD, SIG_IGN);							//Ignoring SIGCHLD for background processes
				}

				err = execve(command, argVec, envVec);	
				if(err == -1) {
					printf("%s: command not found!\n", command);
					exit(1);
				}
				break;
		
			default:		
				sigprocmask(SIG_BLOCK, &blockSet, &prev);				//Blocking signals in parent process (at shell command line)
				signal(SIGCHLD,childHandler);
				add(childPid);											//adding job to jobs array		
				if(waitAction == 1)
				{
					 waitpid(childPid, &status, 0);
					 rem(childPid);
				 }
		}	//end of switch()
	}	//end of while(1)
	exit(0);
}	//end of main()

int exitStatus(char a[]){

	if(strcmp(a, "exit") == 0) return 1;
	return 0;	
}

int clearTerminal(){
	
	int i;
	for(i=0; i<200; ++i){
		printf("\n");
	}
	return 1;
}

void execSigIntMain(int signo){

	shellReturn = 1;
}

void execSigTSTPMain(int signo){

	shellReturn = 1;
}

void add(pid_t pid){

	jobs[ijid].pid = pid;
	jobs[ijid].jid = ijid+1;
	jobs[ijid].status = 1;
	++ijid;
}

void rem(pid_t pid){

	int i;
	for(i=0; i<ijid; ++i){
		if(jobs[i].pid == pid){
			jobs[i].jid = 0;
		}
	}
}

void print() {

	int i;
	for(i=0; i<ijid; ++i){
		if(jobs[i].jid != 0){
			printf("[%d]\t%d\tRunning\n", jobs[i].jid, jobs[i].pid);
		}
	}
}

void childHandler(int signum)
{
    pid_t childPid;
    int childStatus;

    while ((childPid = waitpid( -1, &childStatus, WNOHANG)) > 0){
        if (WIFEXITED(childStatus)){
            WEXITSTATUS(childStatus);
            rem(childPid);
			printf("Process Id:%d Status:%d\n", childPid, childStatus);
        }
        else
            if (WIFSTOPPED(childStatus)){
                WSTOPSIG(childStatus);
				rem(childPid);
				printf("Process Id:%d Status:%d\n", childPid, childStatus);		
            }
            else{
                if (WIFSIGNALED(childStatus)){
                    WTERMSIG(childStatus);
					rem(childPid);
					printf("Process Id:%d Status:%d\n", childPid, childStatus);		
                }
                else{
                    perror("waitpid");
                }
			}
	}
}


