//
//  ftp_Commands.h
//
//  Created by Sadafi Yadegari on 2018-03-27.
//  Copyright © 2018 Sadafi Yadegari. All rights reserved.
//

#ifndef ftp_Commands_h
#define ftp_Commands_h

#include <stdio.h>

enum FTP_CMD {
  INVALID = -1,
  USER,
  PASS,
  QUIT,
  CWD,
  CDUP,
  TYPE,
  MODE,
  STRU,
  RETR,
  PASV,
  NLST
};

struct ftp_cmd {
  enum FTP_CMD cmd;
  char* args;
};

struct ftp_cmd* parse_cmd(char *buf, int socket, char* parent);
int handle_username(char* param, int socket);
int handle_login(char* param, int socket);
int handle_commands(struct ftp_cmd* cmd, int socket);


void handle_inc_num_arg(int socket);
void handle_invalid(char* param, int socket);
void handle_not_logged_in(int socket);
int handle_quit(char* param, int socket);
int handle_user(char* param, int socket);
int handle_cwd(char* param, int socket);
int handle_cdup(char* param, int socket, char* parent);
int handle_type(char* param, int socket);
int handle_mode(char* param, int socket);
int handle_retr(char* param, int inputSocket);
int handle_stru(char* param, int socket);
int handle_pasv(char* param, int inputSocket);
int handle_nlst(char* param, int socket);

int retrieveFile(const char* server_fileName, int socket);

#endif /* ftp_Commands_h */
