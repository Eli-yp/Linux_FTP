#include "ftpserve.h"

//主函数入口
int main(int argc, char *argv[])
{
    int sock_listen, sock_control, port, pid;
    int opt = 1;
    struct sockaddr_in serve_addr, client_addr;
    char client_ip_buf[4096];

    //(1)调用socket创建套接字
    sock_listen = Socket(AF_INET, SOCK_STREAM, 0);

    //端口复用
    Setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    //初始化serve_addr;
    memset(&serve_addr, 0, sizeof(serve_addr));
    serve_addr.sin_family = AF_INET;    //用ip4
    serve_addr.sin_port = htons(SERVE_PORT);  //
    serve_addr.sin_addr.s_addr = htonl(INADDR_ANY); //绑定ip

    //（２）调用bind绑定ip地址和端口号
    Bind(sock_listen, (struct sockaddr*)&serve_addr, sizeof(serve_addr));

    //(3)调用listen,设置能同时连接的上限数
    Listen(sock_listen, 5);

    //循环接受不同的客户机请求
    while(1) {
        //调用accept,阻塞等待客户端发起请求
        socklen_t len = sizeof(client_addr);
        sock_control = Accept(sock_listen, (struct sockaddr *) &client_addr, &len);

        /* //自己测试的时候可以用来答应客户端的地址和端口，
        //打印客户端的IP地址和端口号,这样可以知道谁连接过来了，当然，这个可以不要
        cout << "client IP: " << inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip_buf,sizeof(client_ip_buf)) << " ";
        cout << "client port: " << ntohs(client_addr.sin_port) << endl;
        */

        //创建子进程处理客户端请求
        if ((pid = fork()) < 0) {
            perror("Error forking child process");
        }

        //子进程调用ftpserve_process函数与客户端交互
        else if (pid == 0)
        {
            close(sock_listen);  //关闭监听套接字
            ftpserve_process(sock_control); //处理用户请求
            close(sock_control); //用户请求处理完毕，关闭控制套接字
            exit(0);
        }

        close(sock_control); //父进程关闭控制套接字
    }

    close(sock_listen);
    return 0;
}

//处理客户端请求
void ftpserve_process(int sock_control) {
    int sock_data;  //用来和客户端连接
    char cmd[5];    //用来保存用户输入的命令
    char filename[MAXSIZE]; //用来保存用户要获得的文件名

    send_response(sock_control, 220);   //220代表欢饮码

    //用户认证
    if (ftpserve_login(sock_control) == 1)   //认证成功
        send_response(sock_control, 230);
    else {
        send_response(sock_control, 430);    //认证失败
        exit(-1);
    }

    //处理用户请求
    while (1) {
        //接受用户传来的命令，并解析，获得命令和参数
        int rc = ftpserve_recv_cmd(sock_control, cmd, filename);

        if ((rc < 0) || (rc == 221)) //用户输入命令"QUIT"
            break;

        //用户输入命令list
        if (rc == 200) {
            //和客户端建立连接
            if ((sock_data = ftpserve_start_data_conn(sock_control)) < 0) {
                //若没连接成功，说明客户端已经关闭了．
                close(sock_control);
                exit(1);
            }

            //执行list命令
            ftpserve_list(sock_data, sock_control);
            close(sock_data);   //关闭和客户端的连接
        }

            //用户输入命令get filename
        else if (rc == 300) {
            //先和客户建立连接
            if ((sock_data = ftpserve_start_data_conn(sock_control)) < 0) {
                //若没连接成功，说明客户端已经关闭了．
                close(sock_control);
                exit(1);
            }
            //执行get file命令
            ftpserve_get_file(sock_data, sock_control, filename);
            close(sock_data);   //关闭连接

        }

            //用户输入put file命令
        else if (rc == 400) {

            //先和客户端建立连接
            if ((sock_data = ftpserve_start_data_conn(sock_control)) < 0) {
                //若没连接成功，说明客户端已经关闭了．
                close(sock_control);
                exit(1);
            }

            //接受由客户端传来的参数，看看客户端要上传的文件是否存在
            int retcode = 0;
            if (recv(sock_control, &retcode, sizeof(retcode), 0) < 0) {
                perror("error reading message from client");
            }
            int rc = ntohl(retcode);

            if (rc == 250)   //说明文件不存在，关闭连接，执行下一个命令
            {
                close(sock_data);
                continue;
            }
            //否则，执行put命令
            ftpserve_put_file(sock_data, sock_control);
            close(sock_data);   //关闭连接，执行下一个命令
        }
    }
}

//用户登录
int ftpserve_login(int sock_control)
{
    char buf[MAXSIZE];
    char user[MAXSIZE];     //保存用户输入的用户名
    char pass[MAXSIZE];     //保存用户输入的密码
    char username[] = "anonymous"; //保存自己设置的用户名,(匿名登录)
    char passname[] = ""; //保存密码
    memset(buf, 0, sizeof(buf));
    memset(user, 0, sizeof(user));
    memset(pass, 0, sizeof(pass));

    //获得客户端传来的用户名
    if( (recv(sock_control, buf, sizeof(buf), 0)) == -1)
        sys_err("recv error");
    int i = 0;
    int n = 0;
    while(buf[i] != '\n')
        user[n++] = buf[i++];


    //用户名正确，通知用户输入密码
    send_response(sock_control, 331);

    memset(buf, 0, sizeof(buf));    //清空缓存

    //获得用户传来的密码
    if( (recv(sock_control, buf, sizeof(buf), 0)) == -1)
        sys_err("recv error");
    i = 0;
    n = 0;
    while(buf[i] != '\n')
        pass[n++] = buf[i++];

    //比较客户端传来的用户和密码和服务器端设置的是否一样
    if(strcmp(user, username) == 0 && strcmp(pass, passname) == 0)
        return 1;
    else
        return 0;
}

//接受用户的命令并解析
int ftpserve_recv_cmd(int sock_control, char* cmd, char* filename)
{
    int rc = 200;
    char buffer[MAXSIZE];
    memset(buffer, 0, sizeof(buffer));
    memset(cmd, 0, sizeof(cmd));
    memset(filename, 0, sizeof(filename));

    //接受客户端的命令
    if((recv(sock_control, buffer, sizeof(buffer), 0)) < 0)
        sys_err("recv error");

    //解析出用户的命令
    strncpy(cmd, buffer, 4);    //从buffer里面复制４个字节到cmd里面
    char* fname = buffer + 5;   //获取文件的名字
    strcpy(filename, fname);    //保存到filename里面去

    //说明用户输入的是quit，退出命令
    if(strcmp(cmd, "QUIT") == 0)
        rc = 221;

    else if(strcmp(cmd, "LIST") == 0)   //说明用户输入的是ls
        rc = 200;
    else if(strcmp(cmd, "DOWN") == 0)   //说明用户输入的是get file
        rc = 300;
    else if(strcmp(cmd, "PUSH") == 0)   //说明用户输入的是put file
        rc = 400;
    else
        rc = 502;   //无效的命令

    //把反馈返回给客户端．
    send_response(sock_control, rc);

    return rc;
}


//和客户端实现数据连接, 成功返回数据连接的套接字，失败返回-1
int ftpserve_start_data_conn(int sock_control)
{
    char buf[1024];
    int wait;
    int sock_data;

    //接受客户端发送过来的ACK确认
    if(recv(sock_control, &wait, sizeof(wait), 0) < 0)
    {
        perror("Error while waiting");
        return -1;
    }

    //创建和客户端的连接
    memset(buf, 0, sizeof(buf));
    struct sockaddr_in client_addr, dest_addr;
    sock_data = Socket(AF_INET, SOCK_STREAM, 0);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PROT);
    socklen_t  len = sizeof(client_addr);
    //获得客户端的地址
    getpeername(sock_control, (struct sockaddr*)&dest_addr, &len);
    //将客户端的地址保存在buf里面
    inet_ntop(AF_INET, &dest_addr.sin_addr, buf, sizeof(buf));
    inet_pton(AF_INET, buf, &client_addr.sin_addr.s_addr);
    if(connect(sock_data, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0)
        return -1;

    return sock_data;

}


//执行命令list．发送当前所在目录的目录项列表，然后关闭数据连接
//正确返回0，错误返回-1
int ftpserve_list(int sock_data, int sock_control)
{
    char buf[MAXSIZE];
    memset(buf, 0, sizeof(buf));
    size_t num_read;
    FILE* fd;

    //利用系统调用函数system执行命令，并重定向到隐藏.tmp.txt文件中
    int rs = system("ls -l | tail -n+2 > .tmp.txt");
    if(rs < 0)
        exit(1);

    fd = fopen(".tmp.txt", "r");
    if(!fd)
        exit(1);

    //定位到文件的开始处
    fseek(fd, SEEK_SET, 0);

    send_response(sock_control, 1); //告诉客户端，服务端已经启动好了

    //通过数据连接，发送.tmp.txt文件的内容
    while((num_read = fread(buf, 1, sizeof(buf), fd)) > 0)
    {
        if(send(sock_data, buf, num_read, 0) < 0)
            perror("send error");

        //这里记得把缓冲区清空，
        memset(buf, 0, sizeof(buf));
    }

    fclose(fd);

    send_response(sock_control, 226);       //给客户端发送应答码，关闭数据连接，请求的文件操作成功
    return 0;

}

//执行命令get filename．发送指定的文件到客户端．
//控制信息交互通过控制套接字，处理无效的或者不存在的文件名
void ftpserve_get_file(int sock_data, int sock_control, char* filename)
{
    FILE* fd = nullptr;
    char data[MAXSIZE];
    size_t num_read;

    fd = fopen(filename, "r");  //打开指定的文件

    //若文件不存在
    if(!fd)
        send_response(sock_control, 550);   //发送错误码，指出文件不存在．

    else
    {
        send_response(sock_control, 150);   //发送ok，说明要下载的文件存在
        do{
            num_read = fread(data, 1, MAXSIZE, fd); //读文件内容
            if(num_read < 0)
                cout << "error  in fread" << endl;

            if(send(sock_data, data, num_read, 0) < 0)  //发送文件内容到客户端
                perror("error sending file");

        }while(num_read > 0);   //一直读到文件结束

        send_response(sock_control, 226);   //文件读取完毕以后给客户端发送一个消息

        fclose(fd);
    }

}


//执行命令put file.接受从客户端传来的文件，并保存到服务器的本地
int ftpserve_put_file(int sock_data, int sock_control)
{
    char filename[256];
    char buf[256];
    memset(buf, 0, sizeof(buf));
    memset(filename, 0, sizeof(filename));
    //接受客户端传过来的文件名
    if(recv(sock_control, buf, sizeof(buf), 0) < 0)
        sys_err("recv file name error");
    strcpy(filename, buf);  //把文件名保存给filename

    //  自己测试的时候可以在服务器那段打印下文件名字．这样可以看看是不是你要传上去的文件
    //cout << filename << endl;

    char data[MAXSIZE];
    memset(data, 0, sizeof(data));
    int size;
    FILE* fd = nullptr;
    fd = fopen(filename, "w");  //创建并打开名字为filename的文件

    //将客户端传来的数据（文件内容）写入到本地建立的文件
    while((size = recv(sock_data, data, MAXSIZE, 0)) > 0)
        fwrite(data, 1, size, fd);

    if(size < 0)
        perror("error");

    fclose(fd);
}