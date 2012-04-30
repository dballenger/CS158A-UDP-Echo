#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <string.h>

#include <time.h>

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
  int the_socket = 0, connection_socket = 0, status = 0;
  struct sockaddr_in server, client;
  char receive_buffer[(1 << 16)] = {0};
  
  // if we want TCP, we would change the second argument to SOCK_STREAM
  // ipv6 support is PF_INET6
  the_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port_number);
  
  if(the_socket == -1)
  {
    switch(errno)
    {
      case EPROTONOSUPPORT:
        fprintf(stderr, "Protocol type not supported within this domain.\n");
      break;
      
      case EMFILE:
        fprintf(stderr, "Per-process descriptor table is full.\n");
      break;
      
      case ENFILE:
        fprintf(stderr, "System file table is full.\n");
      break;
      
      case EACCES:
        fprintf(stderr, "Permission to create socket of PF_INET type and SOCK_DGRAM protocol denied.\n");
      break;
      
      case ENOBUFS:
        fprintf(stderr, "Insufficient buffer space available.  Free sufficient system resources first.\n");
      break;
    }
    
    exit(EXIT_FAILURE);
  }
  
  status = bind(the_socket, (struct sockaddr *)&server, sizeof(struct sockaddr));
  
  if(status != 0)
  {
    switch(errno)
    {
      case EAGAIN:
        fprintf(stderr, "Kernel resources needed temporarily unavailable.\n");
      break;
      
      case EBADF:
        fprintf(stderr, "Socket was not a valid descriptor.\n"); //should never get this one
      break;
      
      case ENOTSOCK:
        fprintf(stderr, "Socket variable not a valid socket.\n"); //never get this either
      break;
      
      case EADDRNOTAVAIL:
        fprintf(stderr, "The given address is not available on this machine.\n");
      break;
      
      case EADDRINUSE:
        fprintf(stderr, "Address is already in use.\n");
      break;
      
      case EACCES:
        fprintf(stderr, "Permission to bind to socket denied.\n");
      break;
      
      case EFAULT:
        fprintf(stderr, "Given addr argument not valid part of user address space.\n");
      break;
    }
    
    exit(EXIT_FAILURE);
  }
  
  printf("Socket bound, server is ready to receive packets\n");
  
  int length = 0;
  socklen_t socket_size = sizeof(client);
  
  while(1)
  {
    memset(receive_buffer, '\0', 1 << 16);
    
    length = recvfrom(the_socket, receive_buffer, (sizeof(receive_buffer) - 1), 0, (struct sockaddr *)&client, &socket_size);
    
    if (debug == 1) {
      fprintf(stderr, "Received packet of size %i\n", length);
      fprintf(stderr, "Message received: %s\n", receive_buffer);
    }
    
    if(length == -1)
    {
      switch(errno)
      {
        case EBADF:
          fprintf(stderr, "Socket var not a valid descriptor\n");
        break;
        
        case ECONNRESET:
          fprintf(stderr, "Connection reset\n");
        break;
        
        case ENOTCONN:
          fprintf(stderr, "Socket not connected\n");
        break;
        
        case ENOTSOCK:
          fprintf(stderr, "Given socket var does not refer to a socket\n");
        break;
        
        case EAGAIN:
          fprintf(stderr, "Socket marked as non-blocked, recv operation would block\n");
        break;
        
        case EINTR:
          fprintf(stderr, "Recv interrupted before any data came\n");
        break;
        
        case EFAULT:
          fprintf(stderr, "Recv buffer pointer points outside processes' address space\n");
        break;
      }
      exit(EXIT_FAILURE); // maybe should be continue 
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
   Setup the socket
  */
  int socket_size = 0, the_socket = 0, status = 0;
  struct sockaddr_in local, remote, echo;
  char receive_buffer[(1 << 16)] = {0};
  
  socket_size = sizeof(struct sockaddr_in);
  
  // if we want TCP, we would change the second argument to SOCK_STREAM
  // ipv6 support is PF_INET6
  the_socket = socket(PF_INET, SOCK_DGRAM, 17);
  
  remote.sin_family = PF_INET;
  remote.sin_addr.s_addr = address;
  remote.sin_port = htons(port_number);
  
  if(the_socket == -1)
  {
    switch(errno)
    {
      case EPROTONOSUPPORT:
        fprintf(stderr, "Protocol type not supported within this domain.\n");
      break;
      
      case EMFILE:
        fprintf(stderr, "Per-process descriptor table is full.\n");
      break;
      
      case ENFILE:
        fprintf(stderr, "System file table is full.\n");
      break;
      
      case EACCES:
        fprintf(stderr, "Permission to create socket of PF_INET type and SOCK_DGRAM protocol denied.\n");
      break;
      
      case ENOBUFS:
        fprintf(stderr, "Insufficient buffer space available.  Free sufficient system resources first.\n");
      break;
    }
    
    exit(EXIT_FAILURE);
  }
  
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port = htons(0);
  
  status = bind(the_socket, (struct sockaddr *)&local, socket_size);
  
  if(status != 0)
  {
    switch(errno)
    {
      case EAGAIN:
        fprintf(stderr, "Kernel resources needed temporarily unavailable.\n");
      break;
      
      case EBADF:
        fprintf(stderr, "Socket was not a valid descriptor.\n"); //should never get this one
      break;
      
      case ENOTSOCK:
        fprintf(stderr, "Socket variable not a valid socket.\n"); //never get this either
      break;
      
      case EADDRNOTAVAIL:
        fprintf(stderr, "The given address is not available on this machine.\n");
      break;
      
      case EADDRINUSE:
        fprintf(stderr, "Address is already in use.\n");
      break;
      
      case EACCES:
        fprintf(stderr, "Permission to bind to socket denied.\n");
      break;
      
      case EFAULT:
        fprintf(stderr, "Given addr argument not valid part of user address space.\n");
      break;
    }
    
    exit(EXIT_FAILURE);
  }
  
  int length = 0;
  socklen_t server_size = sizeof(remote);
  
  /**
   Send the packets
  */
  for (register int i = 0; i < packets; i++) {
    if (debug == 1) {
      fprintf(stderr, "Sending packet %i of size %i bytes\n", i, packet_size);
    }
    
    memset(receive_buffer, '\0', 1 << 16);
    
    for (register int i = 0; i < (packet_size - 1); i++) {
      receive_buffer[i] = 'b'; // generate "random" payload data
    }
    
    sendto(the_socket, receive_buffer, packet_size, 0, (struct sockaddr *)&remote, socket_size);
    
    memset(receive_buffer, '\0', 1 << 16); // clear out the receive buffer
    
    if (debug == 1) {
      fprintf(stderr, "Waiting to receive\n");
    }
    
    recvfrom(the_socket, receive_buffer, 1 << 16, 0, (struct sockaddr *)&echo, (socklen_t *)socket_size);
    
    if (debug == 1) {
      fprintf(stderr, "Received message: %s\n", receive_buffer);
    }
  }
  
  if (debug == 1) {
    fprintf(stderr, "sending a receiving all done\n");
  }
}