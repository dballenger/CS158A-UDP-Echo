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

#include <signal.h> // not needed at this time

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
    This is the IP address of the server
  */
  in_addr_t address;
  
  char c;
  
  /**
   Parse the command line arguments to determine what we should do (server / client, etc)
  */
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
        printf("%s -a -- IP address of the server\n", argv[0]);
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
        fprintf(stderr, "No options specified.  Run %s -h for help and the options", argv[0]);
        exit(EXIT_FAILURE);
        break;
     }
  }
  
  if (server == 0 && client == 0) {
    fprintf(stderr, "You must specify either client or server mode.  Run %s -h for help and the options", argv[0]);
    
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
  
  /**
    Configure the socket for the server, we want to listen on any IP, and specify which port (changable with CLI arg)
  */
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port_number);
  
  /**
    Bind the socket to the port to tell the OS to send us all data that comes in UDP on port `port_number` to our app.
  */
  status = bind(the_socket, (struct sockaddr *)&server, sizeof(struct sockaddr));
  CHECK_BIND_STATUS(status);
  
  DEBUG("Socket bound, server is ready to receive packets");
  
  int length = 0;
  socklen_t socket_size = sizeof(client);
  
  while(1)
  {
    bzero(receive_buffer, 1 << 16);
    
    /**
     Receive in data from the socket, the contents of the UDP message received will be played into receive_buffer
    */
    length = recvfrom(the_socket, receive_buffer, (sizeof(receive_buffer) - 1), 0, (struct sockaddr *)&client, &socket_size);
    CHECK_RECEIVE_STATUS(length);
    DEBUG("Got a datagram from %s port %d of length %d\n Message Received: %s", inet_ntoa(client.sin_addr), ntohs(client.sin_port), length, receive_buffer);
    
    /**
     We just want to immediately echo back what we received over the socket
    */
    DEBUG("Sending response back to source\n");
    sendto(the_socket, receive_buffer, length, 0, (struct sockaddr *)&client, socket_size);
  }
  
  /**
   In reality, this won't ever get reached currently
  */
  close(the_socket);
}

void run_client(in_addr_t address, int port_number, int packets, int packet_size) {
  /**
   print out server and packet configuration
  */
  DEBUG("Server Address: %s", inet_ntoa(*(struct in_addr *)&address));
  DEBUG("Port: %i", port_number);
  DEBUG("Packets: %i", packets);
  DEBUG("Packet size (in bytes): %i", packet_size);
  
  /**
   Metric statistics
  */
  struct timeval packet_sent_time, packet_received_time;
  double total_time_for_packets = 0.0;
  
  /**
   Setup the socket
  */
  int socket_size = 0, the_socket = 0, received_length = 0;
  struct sockaddr_in remote;
  char receive_buffer[128000];
  
  /**
   Initialize the receive buffer
   This currently can attempt to go outside of our stack/address space
  */
  bzero(receive_buffer, packet_size + 1);
  
  socket_size = sizeof(struct sockaddr_in);
  
  // if we want TCP, we would change the second argument to SOCK_STREAM
  // ipv6 support is PF_INET6
  the_socket = socket(PF_INET, SOCK_DGRAM, 0);
  CHECK_SOCKET_STATUS(the_socket);
  
  /**
   Configure the socket to connect to the server
  */
  remote.sin_family = PF_INET;
  remote.sin_addr.s_addr = address; /* the address passed in on the command line */
  remote.sin_port = htons(port_number); /* port number default or passed in on command line */
  
  // we want to leave one byte for the null at the end of the buffer
  int length = (packet_size - 1);
  socklen_t server_size = sizeof(remote);
  
  /**
   Send the packets
   
   This operates such that every packet is ACK'd before the next one is sent.
   This loop does _NOT_ handle the case of a packet being lost (will hang the simulation waiting)
  */
  for (register int i = 0; i < packets; i++) {
    DEBUG("Sending packet %i of size %i bytes", i, packet_size);
    
    /**
     Zero out the buffer
    */
    bzero(receive_buffer, packet_size);
    for (register int i = 0; i < length; i++) {
      receive_buffer[i] = '1'; // generate "random" payload data
    }
    
    /**
     Track the time immediately before we send the payload
    */
    gettimeofday(&packet_sent_time, 0);
    /**
     Send the frame to the server
    */
    sendto(the_socket, receive_buffer, packet_size, 0, (struct sockaddr *)&remote, server_size);
    /**
     Zero out the buffer in preparation for the reply
    */
    bzero(receive_buffer, packet_size);
    
    DEBUG("Waiting to receive");
    
    /**
     Listen for the response
    */
    received_length = recvfrom(the_socket, receive_buffer, packet_size, MSG_WAITALL, NULL, NULL);
    CHECK_RECEIVE_STATUS(received_length);
    
    /**
     Track the receive time immediately after the packet shows up
    */
    gettimeofday(&packet_received_time, 0);
    /**
     Add in the latency from this packet to our total latency of all packets
    */
    total_time_for_packets += timespecDiff(&packet_received_time, &packet_sent_time);
    
    DEBUG("Received message: %s\nRound trip delay: %f\n", receive_buffer, timespecDiff(&packet_received_time, &packet_sent_time));
  }
  
  DEBUG("sending and receiving all done");
  
  printf("Total time taken to send and receive %i packets: %f\n\n", packets, total_time_for_packets);
}

/**
 Determine the difference in time to tell how long it took round trip for a packet
*/
double timespecDiff(struct timeval *end, struct timeval *start)
{
 return (double)(((end->tv_sec * SECONDS_TO_NANOSECONDS) + end->tv_usec) -
          ((start->tv_sec * SECONDS_TO_NANOSECONDS) + start->tv_usec)) / SECONDS_TO_NANOSECONDS;
}
