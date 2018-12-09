#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fstream>  
#include <cstring>

using namespace std;

#define SERVE_PORT 8000
#define CLIENT_PROT 9000
#define MAXSIZE 1024

void sys_err(const char* str);
int Socket (int domain, int type, int protocol);
int Bind(int fd, struct sockaddr* addr, socklen_t len);
int Listen(int fd, int n);
int Accept(int fd, struct sockaddr* addr, socklen_t * len);
int Connect(int fd, struct sockaddr* addr, socklen_t len);
int Close(int fd);
int send_response(int sockfd, int rc);
int Setsockopt(int sockfd, int level, int option, const void *val, socklen_t len);
void trimstr(char* str, int n);

#endif
