//
//  ftp_Commands.c
//
//  Created by Sadafi Yadegari on 2018-03-27.
//  Copyright © 2018 Sadafi Yadegari. All rights reserved.
//
#include <fcntl.h>
#include <stdarg.h>
#include <sys/uio.h>

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
#include <sys/uio.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/stat.h>
#include "ftp_Commands.h"
#include "dir.h"
#include <inttypes.h>

#include <string.h>

#include <netdb.h>
#include <time.h>
#include "ftp_Commands.h"
#include "dir.h"
//#include "file_retrieval.h"

//extern int waiting = 0;
int isLoggedIn = 0, isAscci = 0, isInStream = 0, setToFileStructure = 0, isInPassiveMode = 0; // flag variables
struct sockaddr_in pasvMode_adr; // pasv server
int dataSock_fd;
int newdataSock_fd = -1;
int passiveModePort; // socket and port for the pasv server
#define PASSIVE_BACKLOG 1 // how many pending passive connections queue will hold
#define PORT_LOWER_RANGE 1024 // lower range on the port number
#define PORT_UPPER_RANGE 65536 // upper range on the port number
#define BACKLOG 1   // how many pending connections queue will hold

#define FILE_ARRIVED


struct ftp_cmd* parse_cmd(char* buf, int socket, char* parent) {
  struct ftp_cmd* stub = malloc(sizeof(struct ftp_cmd));
  
  
  char command[100];
  char param[1000];

  sscanf(buf, "%s %s", command, param);
  
  if (strcasecmp(command, "USER") == 0) {
    
    stub->cmd = USER;
    if (handle_user(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "QUIT") == 0) {
    
    stub->cmd = QUIT;
    if (handle_quit(param, socket) == 0) stub->args = param; // more arguments than neeeded
    
  } else if (strcasecmp(command, "CWD") == 0) {
  
    stub->cmd = CWD;
    if (handle_cwd(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "CDUP") == 0) {
    
    stub->cmd = CDUP;
    if (handle_cdup(param, socket, parent) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "TYPE") == 0) {
    
    stub->cmd = TYPE;
    if (handle_type(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "MODE") == 0) {
    
    stub->cmd = MODE;
    if (handle_mode(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "STRU") == 0) {
    
    stub->cmd = STRU;
    if (handle_stru(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "RETR") == 0) {
    
    stub->cmd = RETR;
    if (handle_retr(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "PASV") == 0) {
    
    stub->cmd = PASV;
    if (handle_pasv(param, socket) > 0) stub->args = param;
    
  } else if (strcasecmp(command, "NLST") == 0) {
    stub->cmd = NLST;
    if (handle_nlst(param, socket) > 0) stub->args = param;
    
  } else {
    handle_invalid(param, socket);
  }
  return stub;
}

// handles quit, returns 1 if successful, 0 otheriwse
int handle_quit(char* param, int socket) {
  if (strcasecmp(param, "") == 0) {
    handle_inc_num_arg(socket);
    return 0;
  } else {
    
    // setting checks back to initial values
    isLoggedIn = 0;
    isAscci = 0;
    isInStream = 0;
    setToFileStructure = 0;
    isInPassiveMode = 0;
    char* response = "Terminating connection.\n";
    send(socket, response, strlen(response), 0);
    fflush(stdout);
    close(socket);
    close(dataSock_fd);
    dataSock_fd = 0;
    close(newdataSock_fd);
    newdataSock_fd = -1;
    return 1;
  }
}

// handles incorrect number of arguments
void handle_inc_num_arg(int socket) {
  char* response = "501 Syntax error in parameters or arguments.\n";
  send(socket, response, strlen(response), 0);
}

// hanldes invalid commands
void handle_invalid(char* param, int socket) {
  char* response = "500 Syntax error, command unrecognized.\n";
  send(socket, response, strlen(response), 0);
}

// handles user, returns 1 if login was successful, false otherwise
int handle_user(char* param, int socket) {
  if (strcasecmp(param, "") == 0) {
    handle_inc_num_arg(socket);
    return 0;
  } if (isLoggedIn) {
    char* response = "430 Can't change from homeDir.\n";
    send(socket, response, strlen(response), 0);
    isLoggedIn = 1;
    return 0;
  } else {
    // new login
    if (strcasecmp(param, "homeDir") == 0) {
      char* login_success = "230 Login successful.\n";
      send(socket, login_success, strlen(login_success), 0);
      isLoggedIn = 1;
      return 1;
    } else {
      char* login_failed = "430 This server only supports the username: homeDir.\n";
      send(socket, login_failed, strlen(login_failed), 0);
      isLoggedIn = 0;
      return 0;
    }
  }
}

// handles cwd cases, returns 1 if successful, 0 otherwise
int handle_cwd(char* param, int socket) {
  if (strcasecmp(param, "") == 0) {
    handle_inc_num_arg(socket);
    return 0;
  } else if (isLoggedIn == 0) {
    handle_not_logged_in(socket);
    return 0;
  } else if ((strcasecmp(param,"./") != 0) &&
             (strcasecmp(param,"../") != 0) &&
             (strcasecmp(param,"..") != 0) &&
             (strcasecmp(param,".") != 0) &&
             (chdir((char *) param) == 0)) {
    char* cwd_success = "200 Working directory changed.\n";
    send(socket, cwd_success, strlen(cwd_success), 0);
    return 1;
  } else {
    char* cwd_fail = "550 Failed to change path.\n";
    send(socket, cwd_fail, strlen(cwd_fail), 0);
    return 0;
  }
}

// handles not logged in cases
void handle_not_logged_in(int socket) {
  char* response = "530 Not logged in.\n";
  send(socket, response, strlen(response), 0);
}

// handles cdup cases, returns 1 if successful, 0 otherwise
int handle_cdup(char* param, int socket, char* parent) {
  // TODO: Move parent Directory up in the code, so it doesn't update every time
 // char parentDictory[1024];
  //(parentDictory, sizeof(parentDictory));
  chdir((char*) parent);
  char* message257 = "257 changed to parent directory. \n";
  send(socket, message257, strlen(message257), 0);
  return 1;
}

// handles type cases, returns 1 if successful, 0 otherwise
int handle_type(char* param, int socket) {
  if(!isLoggedIn) {
    handle_not_logged_in(socket);
    return 0;
  } else {
    if(strcasecmp(param, "A") == 0){
      if (isAscci) {
        char* same_response = "200 Type is already ASCII.\n";
        send(socket, same_response, strlen(same_response), 0);
        return 0;
      } else {
        isAscci = 1;
        char* change_response = "200 Type changed to ASCII.\n";
        send(socket, change_response, strlen(change_response), 0);
        return 1;
      }
    } else if (strcasecmp(param, "I") == 0) {
      if (isAscci){
        isAscci = 0;
        char* change_response = "200 Type changed to Image.\n";
        send(socket, change_response, strlen(change_response), 0);
        return 1;
      } else {
        char* same_response = "200 Type is Image already.\n";
        send(socket, same_response, strlen(same_response), 0);
        return 0;
      }
    } else {
      char* unaccepted_response = "504 Only Type A and Type I are supported.\n";
      send(socket, unaccepted_response, strlen(unaccepted_response), 0);
      return 0;
    }
  }
}

// handles mode cases, returns 1 if successful, 0 otherwise
int handle_mode(char* param, int socket) {
  if (!isLoggedIn) {
    handle_not_logged_in(socket);
    return 0;
  } else {
    if (strcasecmp(param, "S") != 0) {
      char* not_allowed_response = "504 Only Stream mode is permitted.\n";
      send(socket, not_allowed_response, strlen(not_allowed_response), 0);
      return 0;
    } else if (isInStream == 0) {
      isInStream = 1;
      send(socket, "200 Stream mode activated.\n", strlen("200 Entering Stream mode.\n"), 0);
      return 1;
    } else {
      send(socket, "200 Stream mode is already activated.\n", strlen("200 Stream mode is already activated.\n"), 0);
      return 0;
    }
  }
}

// handles stru cases, returns 1 if successful, 0 otherwise
int handle_stru(char* param, int socket) {
  if(!isLoggedIn){
    handle_not_logged_in(socket);
    return 0;
  } else {
    if (strcasecmp(param, "F") != 0) {
      char* not_allowed_response = "504 File Structure is the only Data Structure supported.\n";
      send(socket, not_allowed_response, strlen(not_allowed_response), 0);
      return 0;
    } else {
      if (setToFileStructure) {
        send(socket, "200 Already set to File structure.\n",
             strlen("200 Already set to File structure.\n"), 0);
        return 0;
      } else {
        setToFileStructure = 1;
        char* structure_change_response = "200 Data Structure is now set to the File Structure.\n";
        send(socket, structure_change_response, strlen(structure_change_response), 0);
        return 1;
      }
    }
  }
}

// handles retr cases, returns 1 if successful, 0 otherwise
int handle_retr(char* param, int inputSocket) {
  //File* file;
  
  if (isInPassiveMode == 0 ) {
    // 425  Can't open data connection.
    char* notInPasvMode = " 425 You must enter passive mode first. \n";
    send(inputSocket, notInPasvMode, strlen(notInPasvMode), 0);
    return 0;
  }
  // if in passive mode but RETR command doesn't have an argumetn, return error
  else if (param == NULL && (isInPassiveMode == 1)) {
    char* RETR_syntax_error = "501 Syntax error. Please specify a file name. \n";
    send(inputSocket, RETR_syntax_error, strlen(RETR_syntax_error), 0);
    return 0;
  }
  else if (isInPassiveMode == 1){
    
    listen(dataSock_fd, PASSIVE_BACKLOG);
    socklen_t sin_size1 = sizeof pasvMode_adr;
    newdataSock_fd = accept(dataSock_fd, (struct sockaddr *)&pasvMode_adr, (unsigned int*)&sin_size1);
    int retrievedFile = retrieveFile(param, newdataSock_fd);
    if (retrievedFile >= 0) {
      char* RETR_success = "200 File transfer successful. \n";
      send(inputSocket, RETR_success, strlen(RETR_success), 0);
    }
    isInPassiveMode = 0;
    close(newdataSock_fd);
    newdataSock_fd = -1;
    close(dataSock_fd);
    dataSock_fd = 0;
    return 1;
  }
  return 0;
}



int handle_pasv(char* param, int inputSocket) {
  if (isLoggedIn == 0) {
    handle_not_logged_in(inputSocket);
    return 0;
  } else if (isInPassiveMode > 0) {
    // we're already in passive mode
    char already_in_passive_mode_response [100];
    snprintf(already_in_passive_mode_response,
             sizeof already_in_passive_mode_response,
             "%s%d%s%d%s%d%s%d%s%d%s%d%s", "227 Already in passive mode: ",
             (pasvMode_adr.sin_addr.s_addr & 0xff), ", ",
             ((pasvMode_adr.sin_addr.s_addr >> 8) & 0xff), ", ",
             ((pasvMode_adr.sin_addr.s_addr >> 16) & 0xff), ", ",
             ((pasvMode_adr.sin_addr.s_addr >> 24) & 0xff), ", ",
             (passiveModePort >> 8), ", ",
             (passiveModePort & 0xff), "\n");
    send(inputSocket, already_in_passive_mode_response,
         strlen(already_in_passive_mode_response), 0);
    isInPassiveMode = 1;
    return isInPassiveMode;
  } else if (isInPassiveMode == 0) {
    
    passiveModePort = PORT_LOWER_RANGE + (rand()% (PORT_UPPER_RANGE - PORT_LOWER_RANGE));
    if ((PORT_LOWER_RANGE <= passiveModePort) && (passiveModePort <= PORT_UPPER_RANGE)) {
      socklen_t sin_size = sizeof pasvMode_adr;
      getsockname(inputSocket, (struct sockaddr *)&pasvMode_adr, &sin_size);
      char success_response [100];
      snprintf(success_response, sizeof success_response,
               "%s%d%s%d%s%d%s%d%s%d%s%d%s", "227 Passive mode activated: ",
               (pasvMode_adr.sin_addr.s_addr & 0xff), ", ",
               ((pasvMode_adr.sin_addr.s_addr >> 8) & 0xff), ", ",
               ((pasvMode_adr.sin_addr.s_addr >> 16) & 0xff), ", ",
               ((pasvMode_adr.sin_addr.s_addr >> 24) & 0xff), ", ",
               (passiveModePort >> 8), ", ",
               (passiveModePort & 0xff), "\n");
      send(inputSocket, success_response, strlen(success_response), 0);
    }
    do {
    // create a passive connection first
    
      pasvMode_adr.sin_family = AF_INET;
      pasvMode_adr.sin_addr.s_addr = INADDR_ANY;
      dataSock_fd = socket(AF_INET, SOCK_STREAM, 0);
      pasvMode_adr.sin_port = htons(passiveModePort);
      socklen_t sin_size1 = sizeof pasvMode_adr;
      if ((bind(dataSock_fd, (struct sockaddr *)&pasvMode_adr, sin_size1))>= 0){
            listen(dataSock_fd, PASSIVE_BACKLOG);
            isInPassiveMode = 1; // now in passive mode
            break;
      }
    } while (sleep(2));
    
    if (isInPassiveMode < 0) {
      send(inputSocket, "500 Couldn't enter passive mode.\n", strlen("500 Couldn't enter passive mode.\n"), 0);
    }
    return isInPassiveMode;
  }
  return isInPassiveMode;
}

// handles nlst cases, returns 1 if successful, 0 otherwise
int handle_nlst(char* param, int inputSocket) {
  // if command = NLST but not yet in passive mode, ask user to enter pasv mode first
  if (isInPassiveMode == 0 ) {
    // 425  Can't open data connection.
    char* notInPasvMode = " 425 You must enter passive mode first. \n";
    send(inputSocket, notInPasvMode, strlen(notInPasvMode), 0);
    return 0;
  }
  // if in passive mode but NLST command has an argumetn, return error
  else if (param != NULL && (param[0] == '\0') && (isInPassiveMode == 1)) {
    char* NLST_syntax_error = "501 Syntax error. Can't take an argument for this command. \n";
    send(inputSocket, NLST_syntax_error, strlen(NLST_syntax_error), 0);
    //isInPassiveMode = 0;
    return 0;
  }
  else if (    isInPassiveMode == 1){
    char* sending_dir_list = "150 Here comes the directory listing. \n";
    send(inputSocket, sending_dir_list, strlen(sending_dir_list), 0);
    listen(dataSock_fd, PASSIVE_BACKLOG);
    socklen_t sin_size = sizeof pasvMode_adr;
    newdataSock_fd = accept(dataSock_fd, (struct sockaddr *)&pasvMode_adr, (unsigned int*)&sin_size);
    char dir_content [1024];
    isInPassiveMode = 0;
    getcwd(dir_content, sizeof dir_content);
    listFiles(newdataSock_fd , dir_content);
    char* dir_list_sent = "226 Directory send OK. \n";
    send(inputSocket, dir_list_sent, strlen(dir_list_sent), 0);
    //isInPassiveMode = 0;
    close(newdataSock_fd);
    newdataSock_fd = -1;
    close(dataSock_fd);
    dataSock_fd = 0;
    return 1;
  }
  return 0;
}


int retrieveFile(const char* server_fileName, int socket) {
  int fileFD;
  int newFile;
  FILE* server_file;
  server_file = fopen(server_fileName, "rb");
  int fileSize = fseek(server_file, 0, SEEK_SET) + 1;
  fseek(server_file, (long) 0, SEEK_SET);
  char cwd[1024];
  const char* dataBuf[fileSize];
  if (server_fileName == NULL) {
    printf("File %s failed to open\n", server_fileName);
    return -1;
  }
  int i;
  for (i = fileSize; i > 0; i--) {
    fread(dataBuf, 1, fileSize-1,  server_file);
    newFile = send (socket, dataBuf, i , 0);
  }
  int closeFile =fclose(server_file);
  return closeFile;
}
