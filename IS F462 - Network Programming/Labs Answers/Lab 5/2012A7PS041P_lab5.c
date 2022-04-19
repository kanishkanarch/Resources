#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define key (2000)

void main (int argc, char* argv[])
{
	int semid, i, j, flag = 1, status = 0;
	pid_t ret, wpid;
	struct sembuf sb;
	int n = 0;
	if (argc == 2)	n = atoi(argv[1]);
	else{
		printf("Invalid Command!!! RETRY\n");
		exit(-1);
	}
	int arg;

	semid = semget (key, 1, IPC_CREAT | 0666);

	if (semid >= 0)
	{
		/*set sem value*/
		i = semctl (semid, 0, SETVAL, n);
		printf("Initial Semaphore Value:\t%d\n", semctl(semid, 0, GETVAL));

		for (i = 0; i < n; i++)
		{
			switch(ret = fork())
			{
			case -1:
				exit(-1);
			case 0:
				sb.sem_num = 0;
				sb.sem_op = -1;
				sb.sem_flg = 0;
				/* Acquire the semaphore */
				if (semop (semid, &sb, 1) == -1)
				{
					printf ("Child: semop failed.\n");
					exit (-1);
				}
				arg = semctl(semid, 0, GETVAL);
				printf ("Child%d Acquired Semaphore\nChild%d pid: %d\t\tSemaphore Value: %d\n", i, i, getpid(), arg);
				//sleep(3);
				exit(0);
				break;
			default:
				break;
			}
		}
		while((wpid=wait(&status)) > 0);
		arg = semctl(semid, 0, GETVAL);
		printf ("Parent Acquired Semaphore\nParent pid: %d\tSemaphore Value: %d\n", getpid(), arg);
		exit(0);
	}
	else
		perror ("semget:");

	return;
}
