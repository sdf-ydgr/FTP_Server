#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <memory.h>

#include "dir.h"
#include "usage.h"
#include "ftp_Commands.h"

#define MAXDATASIZE 2000
char* portNumber;
#define BACKLOG 1   // how many pending connections queue will hold
int waiting;
struct ftp_cmd *command;
enum FTP_CMD FTP_CMD;
//extern char* WELCOME_220 = WELCOME_220;
char parent[2000];


void sigchld_handler(int s) {
  (void)s; // quiet unused variable warning
  
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;
  
  while(waitpid(-1, NULL, WNOHANG) > 0);
  
  errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.
int main(int argc, char **argv) {
  
  getcwd(parent, sizeof(parent));
  
  // This is some sample code feel free to delete it
  // This is the main program for the thread version of nc
  
  int i;
  
  // Verify command line arguments
  if (argc != 2) {
    usage(argv[0]);
    return -1;
  }
  
  // Take the input port # and store it in portNumber
  long size = strlen(argv[1]);
  portNumber = (char*) malloc(size);
  strcat(portNumber, argv[1]);
  
  
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP
  
  if ((rv = getaddrinfo(NULL, portNumber, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  
  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }
    
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    
    break;
  }
  
  freeaddrinfo(servinfo); // all done with this structure
  
  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
  
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
  
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  
  printf("server: waiting for connections...\n");
  
  char buf[MAXDATASIZE];
  char client_msg[2000];
  while(1) {  // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }
    
    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);
    int rs = 0;
    char* WELCOME_220 = "220 This FTP server accepts username: homeDir no password required\n";
    if (send(new_fd, WELCOME_220, strlen(WELCOME_220), 0) == -1){
      perror("send");
      close(new_fd);
      exit(0);
    }
    //if (waiting == 0) {
    while ((rs = recv(new_fd, client_msg, MAXDATASIZE-1, 0)) > 0) {
        command = parse_cmd(client_msg, new_fd, parent);
    //  }
      //close(new_fd);
    }
    
   }
  // This is how to call the function in dir.c to get a listing of a directory.
  // It requires a file descriptor, so in your code you would pass in the file descriptor
  // returned for the ftp server's data connection
  printf("Printed %d directory entries\n", listFiles(1, "."));
  free(portNumber);
  free(command);
  
  return 0;
  
}




