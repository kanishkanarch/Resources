#include <stdio.h>
#include <string.h>
#define MAXSIZE 100

main ()
{
  char buf[MAXSIZE];
  while (fgets (buf, MAXSIZE, stdin) != NULL)
    {
      if (strstr (buf, "@") > 0)
	if (strstr (buf, ".") > 0)
	  printf ("1\n");
	else
	  printf ("-2\n");
      else
	printf ("-3\n");
    }
}
