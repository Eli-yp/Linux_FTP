#ifndef FTPSERVE_H
#define FTPSERVE_H

#include "../common/common.h"

void ftpserve_process(int sockcontrol);
int send_response(int sockfd, int rc);
int Setsockopt(int sockfd, int level, int option, const void *val, socklen_t len);
int ftpserve_login(int sockfd);
int ftpserve_recv_cmd(int sock_control, char* cmd, char* filename);
int ftpserve_start_data_conn(int sock_control);
int ftpserve_list(int sock_data, int sock_control);
void ftpserve_get_file(int sock_data, int sock_control, char* filename);
int ftpserve_put_file(int sock_data, int sock_control);

#endif

