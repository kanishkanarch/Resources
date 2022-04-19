#include<stdio.h>
#include<signal.h>
#include<stdlib.h>
#include<unistd.h>

void int_handler(int signo);
void quit_handler(int signo);

void main() {
	int x, i = 0;
	sigset_t blockSet, prevMask, pending;
	sigemptyset(&blockSet);
	for (i = 1; i < 32; i++) {
		sigaddset(&blockSet, i);
	}
	if (sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1) {printf("sigprocmask1"); exit(0);}
	
	while(1) {
		scanf("%d", &x);
		if(x == 0) break;
	}

	if (sigpending(&pending) == -1) {printf("sigpending"); exit(0);}
	for (i = 1; i < 32; i++) {
		if (sigismember(&pending, i)) printf("Signal %d\n", i); 
	}
	
	//printf("Entering Signals");
	signal(SIGINT, int_handler);	
	signal(SIGQUIT, quit_handler);
	//printf("Entering unblock");
	if (sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1) {printf("sigprocmask2"); exit(0);}
	//printf("Entering while");
	while(1);
}

void int_handler(int signo) {
	int status;
	char s[80];
	printf("Enter a command(only 1 word):");
	gets(s);
	
	pid_t ret;

	ret = fork();	
	if (ret < 0) {printf("Error!!!"); exit(0);}
	else if (ret == 0) {
		execlp(s,s,(char *) 0);
	}
	else {
		wait(&status);
	}
}

void quit_handler(int signo) {
	int status;

	printf("%d\n", wait(&status));
}
