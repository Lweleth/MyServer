#ifndef __CONNECTIONH__
#define __CONNECTIONH__

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#include "request.h"
#include "server.h"
#include "config.h"
#include "util.h"
#include "buffer.h"


#define MAX_CONNECTIONS (1 << 15)

typedef struct connections {
	int 				fd; //connection's opposite descritor
	struct epoll_event	event;// listened epoll event type
	struct sockaddr_in 	addr;// socket address IPV4
	time_t 				active_time;
	int					idx;// this connection's indice in heap
	request_t			req_info;
}connection_t;


//////priroty queue
extern int insert(connection_t* a);
extern int delete(int idx);
extern int ShiftDown(int idx);
extern connection_t* getTop();
extern void swapCon(int a, int b);
extern int build();
extern void give(connection_t* con, int i, int x);
extern connection_t *heap_connections[MAX_CONNECTIONS + 10];
extern int totalCon;
//////
//PIPE OPERATIONS
extern int connection_open_in(int epoll_fd, connection_t *con);
extern int connection_open_out(int epoll_fd, connection_t *con);
extern int connection_close_in(int epoll_fd, connection_t *con);
extern int connection_close_out(int epoll_fd, connection_t *con);

extern connection_t *connection_init();
extern int connection_close(connection_t* con);

extern connection_t *connection_accept(int fd, struct sockaddr_in *addr);
extern void connection_check();
#endif
