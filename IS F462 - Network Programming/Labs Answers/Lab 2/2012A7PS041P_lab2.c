#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

main (){
	int status;
	pid_t ret[6];

	printf("process\t\tpid\tppid\n");
	printf("This\t\t%d\t%d\n", getpid(), getppid());

	ret[0] = fork();

	if (ret[0] == 0) {
		printf("is\t\t%d\t%d\n", getpid(), getppid());

		ret[2] = fork();

		if (ret[2] == 0) {
			printf("prog\t\t%d\t%d\n", getpid(), getppid());
			exit(0);
		}
		else {

			ret[3] = fork();

			if (ret[3] == 0) {
				printf("ramming\t\t%d\t%d\n", getpid(), getppid());
				exit(0);
			}
			else {
				wait(&status);
			}
		}
		exit(0);
	}
	else {
		ret[1] = fork();

		if (ret[1] == 0) {
			printf("network\t\t%d\t%d\n", getpid(), getppid());

			ret[4] = fork();

			if (ret[4] == 0) {
				printf("lab\t\t%d\t%d\n", getpid(), getppid());
				exit(0);
			}
			else {

				ret[5] = fork();

				if (ret[5] == 0) {
					printf("1\t\t%d\t%d\n", getpid(), getppid());
					exit(0);
				}
				else {
					wait(&status);
				}
			}
			exit(0);
		}
		else {
			wait(&status);
		}
	}
	exit(0);
}
