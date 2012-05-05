#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <sys/time.h>
// #include <time.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <signal.h>

#include "project3.h"

static int debug = 0;

int main(int argc, char *argv[])
{
  /**
   Configuration option settings
  */
  int server = 0, client = 0;
  /**
    Default the packet size to 1KB (number given in bytes)
  */
  int port_number = 65000, packets = 100, packet_size = 1 << 10;
  /*
   The IP address to send to from the client
  */
  in_addr_t address;
  
  char c;
  
  while ((c = getopt(argc, argv, "cshdp:b:q:a:")) != -1)
  {
    switch (c)
    {
      case 'c':
        client = 1;
        server = 0;
        break;
      case 's':
        server = 1;
        client = 0;
        break;
      case 'h':
        printf("Usage:\n");
        printf("%s -c -- launch client\n", argv[0]);
        printf("%s -s -- launch server\n", argv[0]);
        printf("%s -d -- enable debugging\n", argv[0]);
        printf("%s -b -- set packet size in bytes\n", argv[0]);
        printf("%s -q -- how many packets to send\n", argv[0]);
        break;
      case 'd':
        printf("Enabling debugging\n");
        debug = 1;
        break;
      case 'p':
        port_number = (int)strtol(optarg, (char **)NULL, 10);
        break;
      case 'b': // packet size
        packet_size = (int)strtol(optarg, (char **)NULL, 10);
        break;
      case 'q': // packets to send
        packets = (int)strtol(optarg, (char **)NULL, 10);
        break;
      case 'a': // ip address of the server
        address = inet_addr(optarg);
        break;
      case '?':
        exit(EXIT_FAILURE);
        break;
      default:
        fprintf(stderr, "No options specified.  Must specify -c or -s for client or server\n");
        exit(EXIT_FAILURE);
        break;
     }
  }
  
  if (server == 0 && client == 0) {
    fprintf(stderr, "Must specify either client or server\n");
    
    exit(EXIT_FAILURE);
  }
  else if (server == 1 && client == 1) {
    fprintf(stderr, "You must only specify either the client or the server\n");
    
    exit(EXIT_FAILURE);
  }
  else if (server == 1 && client == 0) {
    run_server(port_number);
  }
  else if (server == 0 && client == 1) {
    run_client(address, port_number, packets, packet_size);
  }
  
  return EXIT_SUCCESS;
}

/**
 Run the server side of the project on the specified port number (defaults to 65000, overridden with -p 12345)
*/
void run_server(int port_number)
{
  int the_socket = 0, status = 0;
  struct sockaddr_in server, client;
  char receive_buffer[(1 << 16)] = {0};
  
  // if we want TCP, we would change the second argument to SOCK_STREAM
  // ipv6 support is PF_INET6
  the_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  CHECK_SOCKET_STATUS(the_socket);
  
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port_number);
  
  status = bind(the_socket, (struct sockaddr *)&server, sizeof(struct sockaddr));
  CHECK_BIND_STATUS(status);
  
  printf("Socket bound, server is ready to receive packets\n");
  
  int length = 0;
  socklen_t socket_size = sizeof(client);
  
  while(1)
  {
    memset(receive_buffer, '\0', 1 << 16);
    
    length = recvfrom(the_socket, receive_buffer, (sizeof(receive_buffer) - 1), 0, (struct sockaddr *)&client, &socket_size);
    CHECK_RECEIVE_STATUS(length);
    
    if (debug == 1) {
      fprintf(stderr, "Received packet of size %i\n", length);
      fprintf(stderr, "Message received: %s\n", receive_buffer);
    }
    
    /**
     We just want to immediately echo back what we received over the socket
    */
    if (debug == 1) {
      printf("Sending response back to source\n");
    }
    
    sendto(the_socket, receive_buffer, length, 0, (struct sockaddr *)&client, socket_size);
  }
  
  close(the_socket);
}

void run_client(in_addr_t address, int port_number, int packets, int packet_size) {
  if (debug == 1) {
    fprintf(stderr, "Server Address: %s\n", inet_ntoa(*(struct in_addr *)&address));
    fprintf(stderr, "Port: %i\n", port_number);
    fprintf(stderr, "Packets: %i\n", packets);
    fprintf(stderr, "Packet size (in bytes): %i\n", packet_size);
  }
  
  /**
   Metric statistics
  */
  struct timeval packet_sent_time, packet_received_time;
  double total_time_for_packets;
  
  /**
  gettimeofday(&packet_sent_time, 0);
  gettimeofday(&packet_received_time, 0);
  total_time_for_packets = timespecDiff(&packet_received_time, &packet_sent_time);
  printf("Time taken: %f", total_time_for_packets);
  exit(0);
  **/
  
  /**
   Setup the socket
  */
  int socket_size = 0, the_socket = 0, status = 0;
  struct sockaddr_in local, remote, echo;
  char receive_buffer[(1 << 16)] = {0};
  
  socket_size = sizeof(struct sockaddr_in);
  
  // if we want TCP, we would change the second argument to SOCK_STREAM
  // ipv6 support is PF_INET6
  the_socket = socket(PF_INET, SOCK_DGRAM, 17);
  CHECK_SOCKET_STATUS(the_socket);
  
  /**
   The socket to connect to the server
  */
  remote.sin_family = PF_INET;
  remote.sin_addr.s_addr = address;
  remote.sin_port = htons(port_number);
  
  /**
   The local socket settings
  */
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port = htons(0);
  
  status = bind(the_socket, (struct sockaddr *)&local, socket_size);
  CHECK_BIND_STATUS(status);
  
  int length = (packet_size - 1);
  socklen_t server_size = sizeof(remote);
  
  /**
   Send the packets
  */
  for (register int i = 0; i < packets; i++) {
    if (debug == 1) {
      fprintf(stderr, "Sending packet %i of size %i bytes\n", i, packet_size);
    }
    
    memset(receive_buffer, '\0', 1 << 16);
    
    for (register int i = 0; i < length; i++) {
      receive_buffer[i] = 'b'; // generate "random" payload data
    }
    
    /**
     Track the time immediately before we send the payload
    */
    gettimeofday(&packet_sent_time, 0);
    sendto(the_socket, receive_buffer, length + 1, 0, (struct sockaddr *)&remote, server_size);
    
    memset(receive_buffer, '\0', 1 << 16); // clear out the receive buffer
    
    if (debug == 1) {
      fprintf(stderr, "Waiting to receive\n");
    }
    
    recvfrom(the_socket, receive_buffer, length + 1, 0, (struct sockaddr *)&echo, (socklen_t *)socket_size);
    //CHECK_RECEIVE_STATUS(length);
    gettimeofday(&packet_received_time, 0);
    total_time_for_packets += timespecDiff(&packet_received_time, &packet_sent_time);
    
    if (debug == 1) {
      fprintf(stderr, "Received message: %s\n", receive_buffer);
    }
  }
  
  if (debug == 1) {
    fprintf(stderr, "sending a receiving all done\n");
  }
  
  printf("Total time taken to send and receive: %f\n\n", total_time_for_packets);
}

/**
 Determine the difference in time to tell how long it took round trip for a packet
*/
double timespecDiff(struct timeval *end, struct timeval *start)
{
 return (double)(((end->tv_sec * SECONDS_TO_NANOSECONDS) + end->tv_usec) -
          ((start->tv_sec * SECONDS_TO_NANOSECONDS) + start->tv_usec)) / SECONDS_TO_NANOSECONDS;
}
