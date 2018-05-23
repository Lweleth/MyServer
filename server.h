#ifndef __SERVERH__
#define __SERVERH__

#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "config.h"
#include "util.h"
#include "map.h"
#include "connection.h"
#include "request.h"
#include "response.h"
/*
static void sigint_handler(int signum);
static int add_listen_fd();
static int make_server_socker(uint16_t port, int backlog);
*/
extern struct epoll_event server_events[32767];
extern config_t server_config;
extern int epoll_fd;
extern int listen_fd;
// 


extern int server_start(uint16_t port);
extern int server_shudown();
extern int server_accept(int listen_fd);
/*从命令行获取参数*/
extern int config_parse(int argc, char *argv[], config_t *server_config);

static void sigint_handler(int signum);
static int make_server_socker(uint16_t port, int backlog);






#endif





