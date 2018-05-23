#include "server.h"

config_t server_config = {
	8000,
	60,
	2,
	"NULL",
	-1,
	true,
	NULL,
};
int epoll_fd = -1;
int listen_fd = -1;



int server_shudown()
{
	return close(listen_fd);
}

int server_accept(int listen_fd)
{
	int fd;
	static struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	while(true)
	{
		fd = accept(listen_fd, &addr, &len);
		if(fd != ERROR)//直到返回-1 说明能够建立连接
		{
			connection_accept(fd, &addr);
			printf("ACCEPT CLIENT SUCCESS\n");
			//return ERROR;
		}
		else 
		{
			perror("accept client");
			break;
		}
	}
	return 0;
}


//得到SIGINT退出消息结束进程
void sigint_handler(int signum)
{
	if(signum == SIGINT)
	{
		printf("(PID: %u) shutdown", getpid() );
		print_log(123, "(PID: %u) shutdown", getpid());
		map_mime_free();//
		map_header_clear();//
		err_page_free();
		kill(-getpid(), SIGINT);
		exit(-1);
	}
}

 
int add_listen_fd()
{
	//设置socket为nonblock
	int flag = fcntl(listen_fd, F_GETFL, 0);
	ABORT_ON(flag == ERROR, "fcntl: F_GETFL");
	//设置文件描述标识为ERROR
	ABORT_ON(fcntl(listen_fd, F_SETFL, flag | O_NONBLOCK) == ERROR, "fcntl: FSETFL");
	
	struct epoll_event event;
	event.data.ptr = &listen_fd;
	event.events = EPOLLIN | EPOLLET;//监听读事件
	//EPOLL_CTL_ADD注册listen_fd 到 epoll句柄
	return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
}


int make_server_socker(uint16_t port, int backlog)
{
	int listen_fd;
	listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(listen_fd == ERROR)
		return ERROR;
	
	//开启重用端口或地址... 防止TIME_WAIT状态发生
	int on = 1;
	if(server_config.worker > 1)
		setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));//
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);//任意地址

	//bind一个地址和端口
	if(bind(listen_fd, (struct sockaddr *)&saddr, sizeof(saddr)) != OK) 
		return ERROR;
	//监听队列长度为backlog
	if(listen(listen_fd, backlog) != OK)
		return ERROR;
	return listen_fd;
}

int server_start(uint16_t port)
{
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, SIG_IGN); //SIGPIPE BROKEN 
	map_mime_init();//
	map_header_init();//
	status_table_init();//
	err_page_init();
	listen_fd = make_server_socker(port, 1024);
	ABORT_ON(listen_fd==ERROR, "make_server_socker");

	epoll_fd = epoll_create1(0);
	ABORT_ON(epoll_fd == ERROR, "epoll create");

	ABORT_ON(add_listen_fd() == ERROR, "add listen");
	return OK;
}