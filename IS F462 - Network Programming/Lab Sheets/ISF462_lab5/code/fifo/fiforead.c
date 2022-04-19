#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>




main ()
{
  int i, fd, n;
  char buf[100];

  i = mkfifo ("fifo", 0666);
  if (i < 0)
    {
      if (errno == EEXIST)
	{
	  printf ("errno is set as %d\n", errno);
	  perror ("mkfifo");	//it prints the れrrno description
	}
    }

  fd = open ("fifo", O_RDONLY);

  while ((n = read (fd, buf, 100)) > 0)
    {
      buf[n] = '\0';
      printf ("%s\n", buf);
    }

  unlink ("fifo");

}
