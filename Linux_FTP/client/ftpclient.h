#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include "../common/common.h"

int read_reply();
void print_reply(int rc);
void ftpclient_login();
int ftpclient_read_command(char* buffer, char* filename);
int ftpclient_list(int data_sock, int sock_control);
int ftpclient_open_conn(int sock_control);
int ftpclient_get_file(int data_sock, char* filename);
int ftpclient_put_file(int data_sock, int sock_control, char* filename, FILE* fd);
int send_response(int sockfd, int rc);

#endif
