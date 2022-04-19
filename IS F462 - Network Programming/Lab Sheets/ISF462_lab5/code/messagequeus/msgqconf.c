#include <stdio.h>
#include <sys/msg.h>

int
main ()
{
  int msgid, ret;
  key_t key;
  struct msqid_ds buf;
  key = ftok ("msgqconf.c", 'A');
  msgid = msgget (key, IPC_CREAT|0620);
  /* Check successful completion of msgget */
  if (msgid >= 0)
    {
      ret = msgctl (msgid, IPC_STAT, &buf);
      buf.msg_qbytes = 4096;
      ret = msgctl (msgid, IPC_SET, &buf);
      if (ret == 0)
	{
	  printf ("Size successfully changed for queuec %d.\n", msgid);
	}
	else
		perror("msgctl:");
    }
	else
		perror("msgget:");
  return 0;
}
