#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "key.h"
struct my_msgbuf
{
  long mtype;
  char mtext[200];
};

int
main (void)
{
  struct my_msgbuf buf;
  int msqid;
  key_t key;

  if ((key = ftok (MSGQ_PATH, 'B')) == -1)
    {
      perror ("ftok");
      exit (1);
    }

  if ((msqid = msgget (key, IPC_CREAT | 0644)) == -1)
    {
      perror ("msgget");
      exit (1);
    }

  printf ("server: ready to receive messages\n");

  for (;;)
    {
      if (msgrcv (msqid, &(buf.mtype), sizeof (buf), 0, 0) == -1)
	{
	  perror ("msgrcv");
	  exit (1);
	}
      printf ("server: \"%s\"\n", buf.mtext);
    }

  return 0;
}
