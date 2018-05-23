#ifndef __BUFFERH__
#define __BUFFERH__ 

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include "s_string.h"
#include "util.h"

#define BUFFER_SIZE (1024 << 1)

//使用数组可实现环形
typedef struct {
	int len;
	int spr;
	int offest;
	//char buf[0];//C99 std
	char data[BUFFER_SIZE + 1];
} buffer_t;


static inline size_t buffer_remain(buffer_t *buf)
{
	return buf->len - buf->offest;
}
extern buffer_t* buffer_new(size_t len);
extern void buffer_free(buffer_t *buf);
extern void buffer_clear(buffer_t *buf);
extern void buffer_print(const buffer_t *buf);

extern buffer_t* buffer_cat(buffer_t *buf,char* str, size_t len);
extern buffer_t* buffer_cat_cstr(buffer_t *buf,char *str);
extern buffer_t* buffer_init();

extern int buffer_recv(buffer_t* buf, int fd);
extern int buffer_send(buffer_t* buf, int fd);

extern char* buffer_now_end(buffer_t *buf);

extern char* buffer_now_begin(buffer_t *buf);


#endif