#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

main ()
{
  int i;
  int p[2];
  pid_t ret;
  pipe (p);
  ret = fork ();

  if (ret == 0)
    {
      close (1);
      dup (p[1]);
      close (p[0]);
      execlp ("ls", "ls", "-l", (char *) 0);
    }

  if (ret > 0)
    {
      close (0);
      dup (p[0]);
      close (p[1]);
      wait (NULL);
      execlp ("wc", "wc", "-l", (char *) 0);
    }
}