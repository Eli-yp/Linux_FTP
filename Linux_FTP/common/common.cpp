#include "common.h"

void sys_err(const char* str)
{
	perror(str);
	exit(-1);
}

int Socket(int domain, int type, int protocol)
{
	int n;
	if((n = socket(domain, type, protocol)) == 0){
		sys_err("socket error");
	}

	return n;
}



int Bind(int fd, struct sockaddr *addr, socklen_t len)
{
	int n;
	if((n = bind(fd, addr, len)) == -1){
		sys_err("bind error");
	}
	return n;
}


int Listen(int fd, int n)
{
	int ret;
	if((ret = listen(fd, n)) == -1){
		sys_err("listen error");
	}
	return ret;
}

/*
 * accept是一个慢速系统调用，故判断出错的时候要精确一点
 */
int Accept(int fd, struct sockaddr* addr, socklen_t* addr_len)
{
	int n;
again:
	if((n = accept(fd, addr, addr_len)) == -1){
		if((errno == ECONNABORTED) || (errno == EINTR))
			goto again;
		else
			sys_err("accept error");
	}

	return n;
}

int Connect(int fd, struct sockaddr* addr, socklen_t len)
{
	int n;
	if((n == connect(fd, addr, len)) == -1){
		sys_err("connect error");
	}

	return n;
}


int Close(int fd)
{
	int n;
	if((n = close(fd)) == -1){
		sys_err("close error");
	}

	return n;
}


//发送响应码到sockfd, 正确返回0,错误返回-1
int send_response(int sockfd, int rc)
{
    int conv = htonl(rc);
    if(send(sockfd, &conv, sizeof(conv), 0) < 0)
    {
        perror("send error");
        return -1;
    }
    return 0;
}

//端口复用
int Setsockopt(int sockfd, int level, int option, const void *val, socklen_t len)
{
    int n;
    if((n = setsockopt(sockfd, level, option, val, len)) == -1){
        sys_err("setsockopt error");
    }
    return n;
}

//去除字符串中的换行符
void trimstr(char* str, int n)
{
    for(int i = 0; i < n; ++i)
        if(str[i] == '\n')
            str[i] = 0;
}


