#ifndef __REQUESTH__
#define __REQUESTH__

#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

#include "map.h"
#include "connection.h" // pointer to fa
#include "parse.h"
#include "util.h"
#include "buffer.h"

#define HEADER_PAIR(name, method) {SSTRING(#name), offsetof(request_headers_t, name), method}
//struct connections;

typedef struct request_info {
	struct connections 		*con;    //bottom of conn ection
	buffer_t				*req_buf;// request(in) buffer 
	buffer_t				*res_buf;// response(out) buffer
	header_parser_t			 parser;
	int 					 fd;	 //resource file descriptor?
	int 					 size;
	int 					 status; //response status code
	//
	int (*req_handler) (struct request_info *);//handler in
	int (*res_handler) (struct request_info *);//handler out
}request_t ;
typedef int (*header_handler_fun)(request_t *, size_t);

typedef struct {
	string_t 			name;
	size_t 				offset;	//this pair's position in  'request_headers_t'
	header_handler_fun	fun;//pointer of method-function
}header_fun;


/////
extern map_t* map_header_handler;
extern void map_header_init();
extern void map_header_clear();
/////
extern int request_init(request_t *req, struct connections *con);
extern int request_resue(request_t *req);
extern int request_clear(request_t *req);

extern int request_handle(struct connections *con);
extern int response_handle(struct connections *con);


#endif
