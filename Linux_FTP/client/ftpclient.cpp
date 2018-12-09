#include "ftpclient.h"
int sock_control;

int main(int argc, char *argv[]) {
    int data_sock;
    int retcode;
    struct sockaddr_in servaddr;
    char buffer[MAXSIZE];
    char cmd[5];    //用来保存命令
    char filename[256];     //用来保存用户名

    //命令行参数合法性检测
    if (argc != 2) {
        cout << "usage: ./ftpclient serve_IP" << endl;
        exit(0);
    }

    //创建套接字
    sock_control = Socket(AF_INET, SOCK_STREAM, 0);

    //绑定服务器的地址
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVE_PORT);
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr.s_addr);


    //建立数据连接
    Connect(sock_control, (struct sockaddr *) &servaddr, sizeof(servaddr));

    //连接成功，打印信息：
    cout << "connect to " << argv[1] << " successful!" << endl;
    print_reply(read_reply());      //打印服务器的欢迎码

    //获取用户的名字和密码
    ftpclient_login();

    while (1) {
        //循环，直到用户输入quit

        //得到用户输入的命令,保存到buffer里面,把文件名保存在filename里面
        if (ftpclient_read_command(buffer, filename) < 0) {
            cout << "Invalid command" << endl;
            continue;       //跳过本次循环，处理下一个命令
        }

        //发送命令到服务器
        if (send(sock_control, buffer, sizeof(buffer), 0) < 0) {
            close(sock_control);
            exit(1);
        }

        retcode = read_reply();     //读取服务器响应，看服务器是否支持该命令

        //221表示为退出命令
        if (retcode == 221) {
            print_reply(221);
            break;  //退出
        }

        //502表示为无效命令
        if (retcode == 502) {
            cout << "Invalid command" << endl;
            continue;
        }

        //200表示为list命令
        if (retcode == 200) {
            //打开数据连接
            if ((data_sock = ftpclient_open_conn(sock_control)) < 0)
                sys_err("Error opening socket for data connection");

            //执行命令ls
            ftpclient_list(data_sock, sock_control);
            close(data_sock);   //关闭数据连接，继续执行下一个命令
        }

            //300表示为　get file命令
        else if (retcode == 300) {
            //打开数据连接
            if ((data_sock = ftpclient_open_conn(sock_control)) < 0)
                sys_err("Error opening socket for data connection");

            //先从服务器读应答码看看要获取的文件是否有效
            if (read_reply() == 550) {  //550表示文件无效
                print_reply(550);
                close(data_sock);   //关闭数据连接
                continue;   //执行下一个命令
            }

            //若文件有效，则执行get file命令
            ftpclient_get_file(data_sock, filename);
            //文件传输结束后，服务器会发来一个应答码．对应答码作出应答，表示文件传输完成
            print_reply(read_reply());
            close(data_sock);       //关闭连接，执行下一个命令
        }

            //400表示为put file命令
        else if(retcode == 400)
        {
            //打开数据连接
            if ((data_sock = ftpclient_open_conn(sock_control)) < 0)
                sys_err("Error opening socket for data connection");

            FILE* fd = nullptr;
            fd = fopen(filename, "r");  //打开文件

            //若文件不存在
            if(!fd)
            {   //发送消息250告知服务器文件不可用
                send_response(sock_control, 250);
                cout << "File unavailable" << endl;
            }

            else
            {
                //发送350表示文件存在可以上传
                send_response(sock_control, 350);
                ftpclient_put_file(data_sock, sock_control, filename, fd);
                close(data_sock);   //关闭连接，执行下一个命令
            }
        }
    }

    close(sock_control);
    return 0;
}


//接受服务器响应，正确返回状态码，错误返回-1
int read_reply()
{
    int retcode = 0;
    //从服务器读取一个消息，放在retcode里面，
    if(recv(sock_control, &retcode, sizeof(retcode), 0) < 0)
    {
        perror("client: error reading message from server");
        return -1;
    }
    return ntohl(retcode);
}

void print_reply(int rc)
{
    switch(rc)
    {
        case 220:
            cout << "Welcome! server is ready!" << endl;
            break;
        case 221:
            cout << "Goodbye!" << endl;
            break;
        case 226:
            cout << "Request file action successful" << endl;
            break;
        case 550:
            cout << "File unavailabe" << endl;
            break;
    }
}

//获取登录信息，发送到服务器验证
void ftpclient_login()
{
    char buf[MAXSIZE];
    memset(buf, 0, sizeof(buf));

    //获取用户名
    cout << "Name: ";
    fflush(stdout);     //清理标准输入流
    //从标准输入读入用户名
    if((read(STDIN_FILENO, buf, sizeof(buf))) == -1) {
        sys_err("recv error");
    }
    //将用户名发送到服务器
    if(send(sock_control, buf, sizeof(buf) ,0) == -1)
        sys_err("send error");

    //等待应答码331，说明用户名正确，可以继续输入密码
    int wait;
    recv(sock_control, &wait, sizeof(wait), 0);

    //获取密码
    memset(buf, 0, sizeof(buf));    //清空缓存
    cout << "PassWord: ";
    fflush(stdout);
    //从标准输入读入密码
    if((read(STDIN_FILENO, buf, sizeof(buf))) == -1) {
        sys_err("recv error");
    }
    //发送到服务器
    if(send(sock_control, buf, sizeof(buf) ,0) == -1)
        sys_err("send error");

    //接受服务器反馈回来的信息
    int retcode = read_reply();
    switch (retcode)
    {
        case 230:
            cout << "login successful!" << endl;
            break;
        case 430:
            cout << "Invalid username/password." << endl;
            exit(0);
        default:
            perror("error reading message from server");
            exit(1);
    }
}

//解析用户输入的命令
int ftpclient_read_command(char *buf, char* filename)
{
    memset(buf, 0, sizeof(buf));
    memset(filename, 0, sizeof(filename));

    char command[256];  //用来保存读入的命令
    memset(command, 0, sizeof(command));
    char *arg = nullptr;    //用来获取用户输入的命令的前几个表示命令的字符
    cout << "ftpclient> ";  //输入提示符
    fflush(stdout);
    //从标准输入读入命令,保存在command里面
    if(read(STDIN_FILENO, command, sizeof(command)) < 0)
        sys_err("read error");

    //如果直接输入的是回车，则回去失败，表示无效的命令
    if(strcmp(command, "\n") == 0)
        return -1;

    trimstr(command, sizeof(command));      //去除command里面的换行符
    arg = strtok(command, " ");     //获取表示命令的那几个字符
    if(strcmp(arg, "ls") == 0)      //用户输入ls
        strcpy(buf, "LIST");
    else if(strcmp(arg, "get") == 0)    //用户输入get　file
    {
        strcpy(buf, "DOWN");
        arg = strtok(NULL, " ");    //若为get file，还要记得保存file的名字
        if (arg != NULL) {
            strcpy(filename, arg);  //把文件名保存起来
            strcat(buf, " ");   //先添加一个空格
            strncat(buf, arg, strlen(arg)); //再把文件名添加上去,也发到服务器那边去
        }
    }
    else if(strcmp(arg, "put") == 0)    //用户输入put file
    {
        strcpy(buf, "PUSH");
        arg = strtok(NULL, " ");    //若为put file，还要记得保存file的名字
        if (arg != NULL) {
            strcpy(filename, arg);   //只需要把文件名保存起来，但先不发送到服务器那边去
        }
    }
    else if(strcmp(arg, "quit") == 0)   //用户输入quit
        strcpy(buf, "QUIT");
    else
        return -1;  //返回-1表示失败

    return 0;   //返回0表示成功
}

//打开数据连接,将服务器和客户端重现建立一个新的连接
int ftpclient_open_conn(int sock_control)
{
    int sock_listen;
    int opt = 1;
    struct sockaddr_in sock_addr;

    //创建套接字
    if((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return -1;
    }

    //设置本地套接字地址
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(CLIENT_PROT);
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //端口复用
    if(setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1)
    {
        close(sock_listen);
        perror("setsockopt error");
        return -1;
    }

    //绑定本地套接字地址到套接字
    if(bind(sock_listen, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0)
    {
        close(sock_listen);
        perror("bind error");
        return -1;
    }
    //监听
    if(listen(sock_listen, 5) < 0)
    {
        close(sock_listen);
        perror("listen error");
        return -1;
    }

    //在控制连接上发送一个ACK确认
    int ack = 1;
    if((send(sock_control, (char*)&ack, sizeof(ack), 0)) < 0)
    {
        cout << "Client: ack wirte error" << endl;
        exit(1);
    }

    struct sockaddr_in sock_conn_addr;
    socklen_t len = sizeof(sock_conn_addr);
    //等待连接
    int sock_conn = accept(sock_listen, (struct sockaddr*)&sock_conn_addr, &len);

    if(sock_conn < 0)
    {
        perror("accept error");
        return -1;
    }

    close(sock_listen);
    return sock_conn;
}

//实现list命令
int ftpclient_list(int sock_data, int sock_control)
{
    char buf[MAXSIZE];
    size_t num_recv;
    int tmp = 0;
    memset(buf, 0, sizeof(buf));

    //等待服务器启动的信息
    if(recv(sock_control, &tmp, sizeof(tmp), 0) < 0)
    {
        perror("client: error reading message from server");
        return -1;
    }

    //接受服务器传来的数据
    while((num_recv = recv(sock_data, buf, sizeof(buf), 0)) > 0)
    {
        cout << buf;        //把ls的信息输入来
        memset(buf, 0, sizeof(buf));    //把缓冲区清空，以免造成溢出．
    }
    if(num_recv < 0)
        perror("recv error");

    //等待服务器完成的消息
    if(recv(sock_control, &tmp, sizeof(tmp), 0) < 0)
    {
        perror("client: error reading message from server");
        return -1;
    }

    return 0;
}

//实现get <filename>命令
int ftpclient_get_file(int data_sock, char* filename)
{
    char data[MAXSIZE];
//    memset(data, 0, sizeof(data));
    int size;
    FILE* fd = nullptr;

    fd = fopen(filename, "w");  //创建并打开名字为filename的文件

    //将服务器传来的数据（文件内容）写入本地建立的文件
    while((size = recv(data_sock, data, MAXSIZE, 0)) > 0)
        fwrite(data, 1, size, fd);

    if(size < 0)
        perror("recv error");

    fclose(fd);
    return 0;
}

//实现put <filename>命令
int ftpclient_put_file(int data_sock, int sock_control, char* filename, FILE* fd)
{
    //要先把文件名传过去
    char buf[256];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, filename);
    if(send(sock_control, buf, sizeof(buf), 0) < 0)
        sys_err("send error");

    char data[MAXSIZE];
    memset(data, 0, sizeof(data));
    size_t num_read;
    do {
        num_read = fread(data, 1, MAXSIZE, fd); //读文件内容
        if (num_read < 0)
            cout << "error in fread" << endl;

        if (send(data_sock, data, num_read, 0) < 0)   //发送数据（文件内容）
            perror("send error");
    } while (num_read > 0);

    cout << "Upload file action successful." << endl;
    fclose(fd);
}