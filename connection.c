#include "connection.h"


//priority queue 
connection_t *heap_connections[MAX_CONNECTIONS + 10] = {0};
int totalCon = 0;

void swapCon(int a, int b)
{
	heap_connections[a]->idx = b;
	heap_connections[b]->idx = a;
	connection_t *tmp = heap_connections[a];
	heap_connections[a] = heap_connections[b];
	heap_connections[b] = tmp;
	return ;
}


int insert(connection_t* a)
{
	if (totalCon >= MAX_CONNECTIONS)
		return ERROR;
	int idx = ++totalCon;
	a->idx = totalCon;
	heap_connections[totalCon] = a;
	while(idx >> 1)
	{
		time_t nw = heap_connections[idx]->active_time;
		time_t pre = heap_connections[idx >> 1]->active_time;
		if(nw < pre)
			swapCon(idx, idx >> 1), idx >>= 1;
		else 
			break;
	}
	return 0;
}

int ShiftDown(int idx)
{
	if(idx > totalCon)
		return ERROR;
	int pos;
	time_t lson, rson, nw, min;
	while((idx << 1) <= totalCon)
	{
		nw = heap_connections[idx]->active_time;
		lson = heap_connections[idx << 1]->active_time;
		if((idx << 1 | 1) <= totalCon)
			rson = heap_connections[idx << 1 | 1]->active_time;
		else rson = lson;

		if(lson <= rson)
			min = lson, pos = idx << 1;
		else 
			min = rson, pos = idx << 1 | 1;
		if(nw > min)
			swapCon(idx, pos), idx = pos;
		else
			break;
	}
	return OK;
}

int delete(int idx)
{
	if(heap_connections[idx] == NULL)
		return OK;
	//free idx
	swapCon(idx, totalCon);
	heap_connections[totalCon] = NULL;
	totalCon--;
	return ShiftDown(idx);
}

connection_t* getTop()
{
	return heap_connections[1];
}

int build()
{
	int x = 1;
	while((x << 1) <= totalCon)
		x <<= 1;
	while(x > 0)
	{
		//printf("~%d\n", x);
		x >>= 1;
		for(int i = x; i < (x << 1) ; i++)
			ShiftDown(i);
	}
	return 0;
}
static void heap_print() {
  connection_t *c;
  int i;
  printf("----------------heap---------------\n");
  for (i = 1; i <= totalCon; i++) {
    c = heap_connections[i];
    printf("[%2d] %p fd: %2d heap_idx: %2d active_time: %lu\n", i, c, c->fd,
           c->idx, c->active_time);
  }
  printf("----------------heap---------------\n");
}
/////////////////////////////////////
connection_t *connection_init()
{
	connection_t* p;
	p = malloc(sizeof(connection_t));
	if(p != NULL && request_init(&p->req_info, p) == ERROR)//忘了初始化request了...
	{
		free(p);
		p = NULL;
	}
	return p;
}

void give(connection_t* con, int i, int x)
{
	con->idx = i;
	con->active_time = x;
}
int connection_close(connection_t* con)
{
	if(con == NULL)
		return OK;
	
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, con->fd, NULL);
	close(con->fd);
	if(delete(con->idx) == ERROR)
		return ERROR;
	buffer_free(con->req_info.req_buf);
	buffer_free(con->req_info.res_buf);
	free(con);
	return OK;
}


connection_t *connection_accept(int fd, struct sockaddr_in *addr)
{
	if(totalCon >= MAX_CONNECTIONS)
		return NULL;
	connection_t *con = connection_init();
	if(con == NULL)
		return NULL;
	con->fd = fd;
	con->addr = *addr;
	con->active_time = time(0);
	insert(con);//
	//设置nodelay 对应小数据
	int on = 1;
	setsockopt(con->fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
	///
	//设置文件描述标识非阻塞 /////////////////////
	int flag = fcntl(fd, F_GETFL, 0);
	ABORT_ON(flag == ERROR, "fcntl: F_GETFL");
	ABORT_ON(fcntl(con->fd, F_SETFL, flag | O_NONBLOCK) == ERROR, "fcntl: FSETFL noblock");
	//设置监听事件
	con->event.data.ptr = con;//此后epoll_wait直接返回该结构体的指针
	con->event.events = EPOLLIN | EPOLLET;//边缘触发
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, con->fd, &con->event) == ERROR)
	{
		ERR_ON(false, "EPOLL_CTL ADD FAILED");
		connection_close(con);
		return NULL;
	}
	return con;
}

// check top timeout connection
void connection_check()
{
	for (int i = 1; i <= totalCon; i++)
	{
		connection_t* con = heap_connections[i];
		if(time(0) - con->active_time >= server_config.timeout)
		{
			print_log(1, "close timeout connection %p total_Live:%d\n", con, totalCon);			
			connection_close(con);
		}
		else break;
	}
	//heap_print();
}

int connection_open_in(int epoll_fd, connection_t *con)
{
	if(con->event.events & EPOLLIN)//already open
		return OK;
	con->event.events |= EPOLLIN;
	return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->fd, &con->event);
}
int connection_open_out(int epoll_fd, connection_t *con)
{
	if(con->event.events & EPOLLOUT)//already open
		return OK;
	con->event.events |= EPOLLOUT;
	return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->fd, &con->event);
}
int connection_close_in(int epoll_fd, connection_t *con)
{
	if((con->event.events & EPOLLIN) == 0)//already close
		return OK;
	con->event.events &= (~EPOLLIN);
	return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->fd, &con->event);
}
int connection_close_out(int epoll_fd, connection_t *con)
{
	if((con->event.events & EPOLLOUT) == 0)//already close
		return OK;
	con->event.events &= (~EPOLLOUT);
	return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, con->fd, &con->event);
}
/*
int main()
{
	int cnt = 1;
	totalCon = 13;
	for(int i = 15; i >= 3; i--)
	{
		heap_connections[cnt++] = connection_init();
		give(heap_connections[cnt - 1], cnt - 1, i);
	}
	build();
	while(totalCon)
	{
		for(int i = 1; i <= totalCon; i++)
			printf("idx %d:%ld\n", heap_connections[i]->idx,heap_connections[i]->active_time);
		printf("----------------------------\n");
		delete(1);
	}

}*/