#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h> 
#include <sys/types.h>
#include "map.h"
#include "server.h"
#include "connection.h"
#define DEBUG
//#include "untility.h"

/*用于命令行下提示信息*/
static void usage(const char* exec) 
{
	printf("Usage: %s -r html_root_dir [-p port]"
		   "[-t timeout] [-w worker_num] [-d (debug mode)]\n", exec
		);
}
struct epoll_event server_events[32767];

int main(int argc, char *argv[])
{
	int stat = 0;
	if(argc < 1 || (stat = config_parse(argc, argv, &server_config)) != 0)
	{
		printf("%d\n",stat);
		usage(argv[0]);
		exit(1);
	}
	/*  */
	if(server_config.debug)
		goto work;
	int nworker = 0;
	while(true)
	{
		if(nworker >= server_config.worker)
		{
			int stat;
			waitpid(-1, &stat, 0);
			if(WIFEXITED(stat))
				raise(SIGINT);
			print_log(5, "worker exit");
			raise(SIGINT);
		}
		pid_t pid = fork();
		ABORT_ON(pid == -1, "fork");
		if(pid == 0)
			break;
		nworker++;
	}

work:;
	int nfds;
	
	server_start(server_config.port);
	printf("listen_fd = %d\n",  listen_fd);
	printf("pid: %d\n",  getpid());
	while(true)
	{
		nfds = epoll_wait(epoll_fd, server_events, 10240, 10);
		if (nfds == ERROR) 
	     	ERR_ON(errno != EINTR, "epoll_wait");

		if(nfds > 0)
			printf("nfds: %d\n", nfds);
		for(int i = 0; i < nfds; i++)
		{
			int fd = *((int *)server_events[i].data.ptr);
			int ffd = server_events[i].data.fd;
			// printf("fd = %d\n", fd);
			// printf("ffd = %d \n", ffd);
			if( fd == listen_fd)
			{
				printf("~~~accept con~~~\n");
				server_accept(fd);
				printf("~~~accept cpl~~~\n");
			}
			else 
			{
				printf("~~~recv!~~~\n");
				connection_t* con = server_events[i].data.ptr;
				printf("in:%d\n", (con)->event.events & EPOLLIN);
				int stat;
				if(con->active_time + server_config.timeout < time(0))
					continue;
				if(((con)->event.events & EPOLLIN))
				{
					stat = request_handle(con);
					printf("stat: %d\n", stat);
					if(stat == ERROR)
					{
						con->active_time = 0;
						ShiftDown(con->idx);
					}
					else 
					{	
						con->active_time = time(NULL);
						int idx = con->idx;
						while(idx >> 1)
						{
							time_t nw = heap_connections[idx]->active_time;
							time_t pre = heap_connections[idx >> 1]->active_time;
							if(nw < pre)
								swapCon(idx, idx >> 1), idx >>= 1;
							else 
								break;
						}
					}
					printf("~~~recv finished!~~~\n");
				}
				/*if(stat == -1)
				{
					connection_close_in(epoll_fd, con);
					connection_open_out(epoll_fd, con);
				}
				*/				
				printf("out:%d\n", (con)->event.events & EPOLLOUT);
				if(((con)->event.events & EPOLLOUT))
				{
					stat = response_handle(con);
					//char ss[]="404040401212131";
					//stat = send(con->fd, ss, sizeof(ss), 0);
					printf("~~~send~~~\n");fflush(stdout);
					printf("stat: %d\n", stat);
					if(stat == ERROR)
					{
						con->active_time = 0;
						ShiftDown(con->idx);
					}
					else 
					{
						con->active_time = time(NULL);
						int idx = con->idx;
						while(idx >> 1)
						{
							time_t nw = heap_connections[idx]->active_time;
							time_t pre = heap_connections[idx >> 1]->active_time;
							if(nw < pre)
								swapCon(idx, idx >> 1), idx >>= 1;
							else 
								break;
						}
					}
					printf("~~~send finished~~~\n");fflush(stdout);
					//PIPE 
					// con->event.events &= (~EPOLLOUT);
					// epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->fd, &con->event);
				}
				printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@2\n");
				//string_print(&con->req_info.parser.url.extension_MIME);
				printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@2\n");
			}
		}
		connection_check();
	}
	close(epoll_fd);
	server_shutdown();
	return 0;
}	
