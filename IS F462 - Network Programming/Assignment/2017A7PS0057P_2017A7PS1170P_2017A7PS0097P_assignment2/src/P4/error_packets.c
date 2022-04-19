/* The article https://www.binarytides.com/packet-sniffer-code-in-c-using-linux-sockets-bsd-part-2/ 
was used to help in understanding how raw socket work and how can it be used to capture and decode packets.
*/
#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<stdio.h> //For standard things
#include<stdlib.h>    //malloc
#include<string.h>    //strlen
 
#include<netinet/ip_icmp.h>   //Provides declarations for icmp header
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h>    //Provides declarations for ip header
#include<netinet/if_ether.h>  //For ETH_P_ALL
#include<net/ethernet.h>  //For ether_header
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>
  
FILE *output;
struct sockaddr_in s,d;
void die(char *s)
{
    perror(s);
    exit(1);
}

void print_port_info(unsigned char *buf , int size)
{
    int iphdrlen;
    struct iphdr *iph = (struct iphdr *)(buf);
    iphdrlen = iph->ihl*4;
    struct udphdr *udph = (struct udphdr*)(buf + iphdrlen);
    fprintf(output , "   |-Source Port      : %d\n" , ntohs(udph->source));
    fprintf(output , "   |-Destination Port : %d\n" , ntohs(udph->dest));
}

void print_ip_info(unsigned char* Buffer, int Size)
{   
//    unsigned short iphdrlen;
    struct iphdr *iph = (struct iphdr *)(Buffer);
  //  iphdrlen =iph->ihl*4;
    memset(&s, 0, sizeof(s));
    s.sin_addr.s_addr = iph->saddr;
    memset(&d, 0, sizeof(d));
    d.sin_addr.s_addr = iph->daddr;
    fprintf(output , "\n");
    fprintf(output , "   |-Source IP        : %s\n",inet_ntoa(s.sin_addr));
    fprintf(output , "   |-Destination IP   : %s\n",inet_ntoa(d.sin_addr));
    fprintf(output , "   |-Protocol         : %d\n", iph->protocol);    
}

void packet_process(unsigned char* buffer, int size)
{
    //Get the IP Header part of this packet , excluding the ethernet header
    unsigned short iphdrlen;
    struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ihl * 4;
    struct icmphdr *icmph = (struct icmphdr *)(buffer + iphdrlen  + sizeof(struct ethhdr));
    int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof(icmph);
    if(iph->protocol == 1)
    {
        memset(&s, 0, sizeof(s));
        s.sin_addr.s_addr = iph->saddr;
    	fprintf(output ,"\nPacket dropped at router  : %s", inet_ntoa(s.sin_addr));
    	fprintf(output, "\nDetails of packet dropped ->");
    	print_ip_info(buffer + header_size , iphdrlen);
    	print_port_info(buffer + header_size , iphdrlen);  
    	fprintf(output , "\n.................................................................");
    }
    return;
}

int main()
{
    int s_size , data;
    struct sockaddr addr;
         
    unsigned char *buffer = (unsigned char *) malloc(256); 
     
    //output=fopen("log.txt","w");
    output = stdout;
    if(output==NULL) 
        die("file open");
    
    printf("Starting...\n");
     
    int sockfd = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL)) ;
    //setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , "eth0" , strlen("eth0")+ 1 );
     
    if(sockfd < 0)
       die("socket");
    
    while(1)
    {
        s_size = sizeof(addr);
        //Receive a packet
        data = recvfrom(sockfd , buffer , 256 , 0 , &addr , (socklen_t*)&s_size);
        if(data < 0)
            die("Recvfrom error , failed to get packets\n");
        
        //Now process the packet
        packet_process(buffer , data);
    }
    close(sockfd);
    printf("Finished");
    return 0;
}

//##########################################################################
