#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 50
main (int argc, char* argv[])
{
	if (argc != 2) {printf("Enter only one argument\n Exiting!!!\n"); exit(0);}
	int i, status;
	char buf[BUFSIZE];
	int p[6][2];
	pid_t ret[5];
	for (i = 0; i < 6; i++)	pipe (p[i]);
	ret[0] = fork ();
	if (ret[0] == 0) {							//C1
      		if (close(p[0][1] == -1) && close(p[1][0] == -1)) {
			printf("Error in Closing write and read!!!");
			exit(0);
		}
		read(p[0][0], buf, BUFSIZE);
		i = 0;
		while (buf[i]) {		
			buf[i] = toupper(buf[i]);
			i++;
		}
		write(p[1][1], buf, BUFSIZE);
		printf("name:C1\tpid:%d\tstring:%s\n", getpid(), buf);
    		exit(1);
    	}
  	else {										//P
		write(p[0][1], argv[1], strlen(argv[1]));
		ret[1] = fork();
		if (ret[1] == 0) {							//C2
			if (close(p[1][1] == -1) && close(p[2][0] == -1)) {
				printf("Error in Closing write and read!!!");
				exit(0);
			}
			read(p[1][0], buf, BUFSIZE);
			i = 0;
			while (buf[i] != '\0') {		
				buf[i] = (buf[i + 1]);
				i++;
			}
			write(p[2][1], buf, BUFSIZE);
			printf("name:C2\tpid:%d\tstring:%s\n", getpid(), buf);
    			exit(1);
		}
		else {									//P
			ret[2] = fork();
			if (ret[2] == 0) {						//C3
				if (close(p[2][1] == -1) && close(p[3][0] == -1)) {
					printf("Error in Closing write and read!!!");
					exit(0);
				}
				read(p[2][0], buf, BUFSIZE);
				i = 0;
				while (buf[i + 1] != '\0') i++;
				buf[i] = '\0';
				write(p[3][1], buf, BUFSIZE);
				printf("name:C3\tpid:%d\tstring:%s\n", getpid(), buf);
    				exit(1);
			}
			else {
				ret[3] = fork();							//P
				if (ret[3] == 0) {							//C4
					if (close(p[3][1] == -1) && close(p[4][0] == -1)) {
						printf("Error in Closing write and read!!!");
						exit(0);
					}
					read(p[3][0], buf, BUFSIZE);
					i = 0;
					while (buf[i] != '\0') {		
						buf[i] = (buf[i + 1]);
						i++;
					}
					write(p[4][1], buf, BUFSIZE);
					printf("name:C4\tpid:%d\tstring:%s\n", getpid(), buf);
		    			exit(1);
				}
				else {
					ret[4] = fork();							//P
					if (ret[4] == 0) {							//C4
						if (close(p[4][1] == -1) && close(p[5][0] == -1)) {
							printf("Error in Closing write and read!!!");
							exit(0);
						}
						read(p[4][0], buf, BUFSIZE);
						i = 0;
						while (buf[i + 1] != '\0') i++;
						buf[i] = '\0';
						write(p[5][1], buf, BUFSIZE);
						printf("name:C5\tpid:%d\tstring:%s\n", getpid(), buf);
		    				exit(1);
					}
					else {
						wait(&status);
						if (close(p[5][1] == -1) && close(p[1][0] == -1)) {
							printf("Error in Closing write and read!!!");
							exit(0);
						}
						read(p[5][0], buf, BUFSIZE);
						printf("name:P\tpid:%d\tstring:%s\n", getpid(), buf);
 
					}
				}
			}
   		}
    	}
  	exit (0);
}
