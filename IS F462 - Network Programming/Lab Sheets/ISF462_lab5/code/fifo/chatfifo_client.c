#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef struct{
	int dpid;
	int spid;
	char chat[100];
	}Message;


main ()
{
  int i, fd, fd1,n;
  char buf[100];
  char myfifoname[30], fifonameserver[30];
	Message incoming, outgoing;

printf("My pid is %d\n", getpid());


sprintf(myfifoname,"/tmp/%d",getpid());
  i = mkfifo (myfifoname, 0666);
  if (i < 0)
    {
      if (errno == EEXIST)
	{
	  printf ("errno is set as %d\n", errno);
	}
	  perror ("mkfifo");
    }

//sprintf(fifonameserver,"/tmp/server.netproglab4.%s",getenv("LOGNAME"));
  fd1=open("/tmp/chatfifoserver",O_WRONLY);
      outgoing.spid=getpid();
      printf("Enter destination pid and the message: \n");
       scanf("%d\n", &outgoing.dpid);
	fflush(stdout);
	fflush(stdin);
      fgets(buf,100,stdin);
      strcpy(outgoing.chat,buf);
	write(fd1,&outgoing,sizeof(Message));
	printf("Message sent to %d. Waiting for reply...\n", outgoing.dpid);
  
fd = open (myfifoname, O_RDONLY);
  while ((n = read (fd, &incoming, sizeof(Message))) > 0)
    {
      printf ("\nSender:%d, Message:%s\n", incoming.spid,incoming.chat);
	printf("Enter the reply:\n");
	fflush(stdout);
      fgets(buf,100,stdin);
      outgoing.spid=getpid();
      outgoing.dpid=incoming.spid;
      strcpy(outgoing.chat,buf);
	write(fd1,&outgoing,sizeof(Message));
	printf("Message sent to %d. Waiting for reply...\n", outgoing.dpid);
 	
    }

  unlink (myfifoname);

}
