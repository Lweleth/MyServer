#ifndef __CONFIGH__
#define __CONFIGH__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <malloc.h>
#include <assert.h>
#include <sys/stat.h>

#include "MyCJson/myCJson.h"
#include "util.h"

typedef struct config_server
{
	uint16_t 	port;      //65535
	int 		timeout;
	uint32_t    worker;    //core of work
	char 	   *rootdir;   //root addr
	int 		rootdir_fd;//fd of rootaddr
	bool 		debug;
	char       *text;
} config_t;


extern void config_init(config_t* server_config);
//从命令行获取参数
extern int config_parse(int argc, char *argv[], config_t *server_config);
//从文件
extern int config_load(config_t* server_config);

#endif
