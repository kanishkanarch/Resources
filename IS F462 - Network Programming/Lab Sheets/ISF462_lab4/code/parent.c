#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

void err_sys(char* str)
{
	perror(str);
		exit(-1);
		}

#define MAXLINE 80
  int
main (void)
{
  char line[MAXLINE];
  FILE *fpin;

  if ((fpin = popen ("./filter", "r")) == NULL)
    err_sys ("popen error");
  for (;;)
    {
      fputs ("prompt> ", stdout);
      fflush (stdout);
      if (fgets (line, MAXLINE, fpin) == NULL)	/* read from pipe */
	break;
      if (fputs (line, stdout) == EOF)
	err_sys ("fputs error to pipe");
    }
  if (pclose (fpin) == -1)
    err_sys ("pclose error");
  putchar ('\n');
  exit (0);
}
