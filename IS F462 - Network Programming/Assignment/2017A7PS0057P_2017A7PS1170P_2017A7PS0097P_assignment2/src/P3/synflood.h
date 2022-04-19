#define __USE_MISC

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pcap/pcap.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SUSPENSE_TIME 1
#define DEFAULT_ATTACK_TIME 1
#define ETHERNET_HEADERS_LEN 14
#define HOSTNAME_BUFFER_LENGTH 256
#define PACKET_BUFFER_LEN sizeof(struct iphdr) + sizeof(struct tcphdr)
#define FILTER_EXPR_TEMPLATE "tcp and (tcp[tcpflags] & tcp-syn == tcp-syn or tcp[tcpflags] & tcp-ack == tcp-ack) and host %s and port %d"
#define LINKTYPE_ETHERNET DLT_EN10MB

#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_ACK  0x10

typedef struct {
  uint32_t saddr;       /* The source IP address (spoofed) */
  uint32_t daddr;       /* The destination IP address. */
  uint8_t  rsvd;        /* These bytes are not used and are just buffer. */
  uint8_t  proto;       /* The protocol as in the IP headers - 0x6 */
  uint16_t seglen;      /* The computed length of the TCP segment. */
  struct tcphdr thdr;   /* The actual tcp headers - not a part of the pseudo header itself but it makes memory handling easier with this here. */
} tcp_pseudo_header_t;

/* Function declarations */
void vlog(const char format_string[], ...);
void die(const char format_string[], ...);
void hexDump(void *ptr, ssize_t len);
void getOptions(int argc, char *argv[], char hostname[HOSTNAME_BUFFER_LENGTH],
                unsigned short int *port, struct sockaddr_in *host_addr,
                unsigned int *attack_time, bool *no_sniff);
unsigned short int validatePort(const char *inp);
bool validateHostname(const char *inp, char hostname[HOSTNAME_BUFFER_LENGTH],
                      unsigned short int, struct sockaddr_in *host_addr);
unsigned int validateAttackTime(char *inp);
void resolveHostName(char *hostname, unsigned short int port, struct sockaddr_in *addr);
int getRawSocket();
void synflood(char *hostname, unsigned int port, struct sockaddr_in host_addr);
struct in_addr getCurrentIpAddr();
in_addr_t getSpoofedIpAddr();
uint16_t getSpoofedPortNumber();
void setIpHeaders(struct iphdr *ip_headers, struct in_addr *hin_addr);
void setTcpHeaders(struct tcphdr *tcp_headers, in_port_t port);
uint16_t pseudoHeaderTcpChecksum(struct iphdr *ip_headers, struct tcphdr *tcp_headers);
void sniff(char *hostname, int port);
