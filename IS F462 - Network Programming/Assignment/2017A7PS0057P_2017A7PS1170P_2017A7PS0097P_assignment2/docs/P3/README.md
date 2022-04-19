# SynFlood

### Task Requirement:
Write a program [synflood.c](./synflood.c) using raw sockets which works in
the following way:
- It takes server hostname and port on command-line.
- It creates TCP SYN segment and appends ip header and sends to the remote
  host. It uses `IP_HDRINCL` socket option to include IP header.
- It sends TCP SYN segment every 1 second, every time with a different
  source ip and source port. These addresses/port can be random numbers in
  valid range.
- It should display the received replies (SYN+ACK) from the server using
  **[libpcap](https://github.com/the-tcpdump-group/libpcap)** library.
- Program will not send any reply (ACK) segment to web server.


### Dependencies:
- libpcap v1.8.1 (Available on [Ubuntu's Package Repositories](https://
packages.ubuntu.com/search?keywords=libpcap-dev)
- Linux kernal v4.15.0 (or higher)

