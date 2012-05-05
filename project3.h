void run_server(int);
void run_client(in_addr_t, int, int, int);
double timespecDiff(struct timeval *timeA_p, struct timeval *timeB_p);

#define SECONDS_TO_NANOSECONDS 1000000000

#define CHECK_BIND_STATUS(status) if (status != 0) \
{ \
  switch(errno) \
  { \
    case EAGAIN: \
      fprintf(stderr, "Kernel resources needed temporarily unavailable.\n"); \
    break; \
    \
    case EBADF: \
      fprintf(stderr, "Socket was not a valid descriptor.\n"); \
    break; \
    \
    case ENOTSOCK: \
      fprintf(stderr, "Socket variable not a valid socket.\n"); \
    break; \
    \
    case EADDRNOTAVAIL: \
      fprintf(stderr, "The given address is not available on this machine.\n"); \
    break; \
    \
    case EADDRINUSE: \
      fprintf(stderr, "Address is already in use.\n"); \
    break; \
    \
    case EACCES: \
      fprintf(stderr, "Permission to bind to socket denied.\n"); \
    break; \
    \
    case EFAULT: \
      fprintf(stderr, "Given addr argument not valid part of user address space.\n"); \
    break; \
  } \
  \
  exit(EXIT_FAILURE); \
}

#define CHECK_SOCKET_STATUS(status) if(status == -1) \
{ \
  switch(errno) \
  { \
    case EPROTONOSUPPORT: \
      fprintf(stderr, "Protocol type not supported within this domain.\n"); \
    break; \
    \
    case EMFILE: \
      fprintf(stderr, "Per-process descriptor table is full.\n"); \
    break; \
    \
    case ENFILE: \
      fprintf(stderr, "System file table is full.\n"); \
    break; \
    \
    case EACCES: \
      fprintf(stderr, "Permission to create socket of PF_INET type and SOCK_DGRAM protocol denied.\n"); \
    break; \
    \
    case ENOBUFS: \
      fprintf(stderr, "Insufficient buffer space available.  Free sufficient system resources first.\n"); \
    break; \
  } \
  \
  exit(EXIT_FAILURE); \
}

#define CHECK_RECEIVE_STATUS(status) if(status == -1) \
{ \
  switch(errno) \
  { \
    case EBADF: \
      fprintf(stderr, "Socket var not a valid descriptor\n"); \
    break; \
    \
    case ECONNRESET: \
      fprintf(stderr, "Connection reset\n"); \
    break; \
    \
    case ENOTCONN: \
      fprintf(stderr, "Socket not connected\n"); \
    break; \
    \
    case ENOTSOCK: \
      fprintf(stderr, "Given socket var does not refer to a socket\n"); \
    break; \
    \
    case EAGAIN: \
      fprintf(stderr, "Socket marked as non-blocked, recv operation would block\n"); \
    break; \
    \
    case EINTR: \
      fprintf(stderr, "Recv interrupted before any data came\n"); \
    break; \
    \
    case EFAULT: \
      fprintf(stderr, "Recv buffer pointer points outside processes' address space\n"); \
    break; \
  } \
  \
  exit(EXIT_FAILURE); \
}
