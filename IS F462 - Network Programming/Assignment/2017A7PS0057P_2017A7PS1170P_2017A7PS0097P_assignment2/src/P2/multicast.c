#include <netinet/in.h>
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close 
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h> 
#include <sys/file.h>
#include <sys/time.h>
#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

char* group;
int port, count, fd;
bool five;
struct sockaddr_in dst;
timer_t* t[2];
struct itimerspec its[2];
struct sigevent te[2];
#define MSGBUFSIZE 256

typedef struct packet
{
	char hello[6];
	char time[20];
	bool bye;
	pid_t ogsender;
    pid_t lastsender;
}packet;

packet* sendit;

void die(char *s)
{
	perror(s);
	exit(1);
}

char* GetCurrentTime()
{
    char *tim = (char *)malloc(sizeof(char)*20);
    //int rc;
    time_t curr;
    struct tm* timeptr;
    struct timeval tv;

    curr = time(NULL);
    timeptr = localtime(&curr);

    gettimeofday(&tv, NULL);

    /*rc = */strftime(tim, 20, "%H:%M:%S", timeptr);

    char ms[8];
    sprintf(ms, ".%06ld", tv.tv_usec);
    strcat(tim, ms);
    return tim;
}
static void handler2(int sig, siginfo_t *si, void *uc)
{	
	char* temp = GetCurrentTime();
	for(int i = 0 ; i < 20 ; i++)
		sendit->time[i] = temp[i];
	printf("\nbye + %s\n" , sendit->time);
	sendit->bye = 1;
	int tempr = sendto(fd, sendit, sizeof(packet), 0, (const struct sockaddr *) &dst, sizeof(dst));
	if(tempr == -1)
		die("send");
	exit(0);
}
static void handler(int sig, siginfo_t *si, void *uc)
{
	timer_t *tidp;
    tidp = si->si_value.sival_ptr;

    if(*tidp == *t[0])//15s timeout send hello+time()
	{
		printf(YELLOW"Sending customary \"hello\" packet...\n"RESET);
		char* temp = GetCurrentTime();
		for(int i = 0 ; i < 20 ; i++)
			sendit->time[i] = temp[i];
        sendit->ogsender = getpid();
        sendit->lastsender = getpid();
        sendit->bye = 0;
        int tempr = sendto(fd, sendit, sizeof(packet), 0, (const struct sockaddr *) &dst, sizeof(dst));
		if(tempr == -1)
			die("send");
		if(timer_settime(*t[0], 0, &its[0], NULL) == -1)//reset the timer for 15s
	        die("timer");
		if(timer_settime(*t[1], 0, &its[1], NULL) == -1)//set the timer for 5s
	            die("timer");
	    five = 1;
/*        print_pkt(temp[i]);*/
	}
	else if(*tidp == *t[1])//5s timeout print count 
    {
        printf(GREEN"count : %d\n", count);
        printf(RESET);
    	five = 0;
    	count = 0;
    }
    else
        die("Wrong timer");
    
return;
}

int main(int argc, char *argv[])
{
    if (argc != 3) 
    {
       printf("Command line args should be multicast group and port\n");
       printf("(e.g.`listener 239.255.255.250 1900`)\n");
       return 1;
    }

    group = argv[1]; // e.g. 239.255.255.250 for SSDP
    port = atoi(argv[2]); // 0 if error, which is an invalid port

    // create what looks like an ordinary UDP socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) 
    {
        perror("socket");
        return 1;
    }

    // allow multiple sockets to use the same PORT number
    u_int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0)
    	die("Reusing ADDR failed");

    // set up destination address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // differs from sender
    addr.sin_port = htons(port);

    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr(group);
    dst.sin_port = htons(port);

    // bind to receive address
    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) 
    	die("bind");

    // use setsockopt() to request that the kernel join a multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0)
        die("add mem");
    /*u_char loop = 0;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP,  &loop, sizeof(loop)) < 0)
        die("loop");*/


    //signal handler for SIGALRM
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) == -1)
        die("signal handler");

    //signal handler for SIGINT
    struct sigaction sa2;
    sa2.sa_flags = SA_SIGINFO;
    sa2.sa_sigaction = handler2;
    sigemptyset(&sa2.sa_mask);
    if (sigaction(SIGINT, &sa2, NULL) == -1)
        die("signal handler");

    //two timers; 0 for the 15s interval and the 1 for the 5s interval

	for(int i = 0 ; i < 2 ; i++)
	{
		t[i] = (timer_t*)malloc(sizeof(timer_t));
		te[i].sigev_notify = SIGEV_SIGNAL;
	    te[i].sigev_signo = SIGALRM;
	    te[i].sigev_value.sival_ptr = t[i];
	    timer_create(CLOCK_REALTIME, &te[i], t[i]);
	    its[i].it_interval.tv_sec = 0;
		its[i].it_interval.tv_nsec = 0;
		its[i].it_value.tv_nsec = 0;
	}
	//periodic timer to send hello every 15s
	its[0].it_value.tv_sec = 15;

	//timer to count replies received within 5s
	its[1].it_value.tv_sec = 5;

	if(timer_settime(*t[0], 0, &its[0], NULL) == -1)//reset the timer for 15s
	            die("timer");

	
	sendit = (packet* )malloc(sizeof(packet));
	strcpy(sendit->hello, "hello");
	packet* receive = (packet* )malloc(sizeof(packet));
	count = 0;
    // now just enter a read-print loop
    unsigned int addrlen = sizeof(addr);
    while (1) 
    {
        int nbytes = recvfrom(fd, receive, sizeof(packet), 0, (struct sockaddr*)&addr, &addrlen);
        if (nbytes < 0)
        	continue;
        
        if(receive->bye == 1)
        {
        	printf("Message Received from IP -> %s PORT -> %d\n" , inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
			printf("bye + %s\n" , receive->time);
            printf("......................................\n");
            continue;
        }
        if(five == 0)
        {
	        if(receive->lastsender == receive->ogsender)
        	{	
		        printf("Message Received from IP -> %s PORT -> %d\n" , inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		     	printf("echoing...\n");
				printf("%s + %s\n" , receive->hello , receive->time);
                printf("......................................\n");
                receive->lastsender = getpid();
/*                printf("new receive->lastsender : %d\n", receive->lastsender);*/
		     	nbytes = sendto(fd, receive, sizeof(packet), 0, (const struct sockaddr *) &dst, sizeof(dst));
		     	if (nbytes < 0) 
	            	continue;
	        }
        }
        else//if the 5s timer is on
        {
        	if(receive->ogsender == getpid() && receive->ogsender != receive->lastsender)//it is a reply if send time == receive time, incerement count
        		count++;
        	else if(receive->ogsender == receive->lastsender && receive->ogsender != getpid())
        	{
                printf("Message Received from IP -> %s PORT -> %d\n" , inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		     	printf("echoing...\n");
				printf("%s + %s\n" , receive->hello , receive->time);
                printf("......................................\n");
                receive->lastsender = getpid();
                /*printf("new receive->lastsender : %d\n", receive->lastsender);*/
                nbytes = sendto(fd, receive, sizeof(packet), 0, (const struct sockaddr *) &dst, sizeof(dst));
                if (nbytes < 0) 
                    continue;
        	}
        }
    }
    return 0;
}
