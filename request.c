#include "connection.h"
#include "request.h"
#include "response.h"


static int request_check_http_version(request_t* req); 
static int request_check_resource(request_t* req);

static int request_handle_request_line(request_t *req);
static int request_handle_headers_field(request_t *req);
static int request_handle_body(request_t *req);

static int response_handle_send_buffer(request_t *req);
static int response_handle_send_file(request_t *req);

//
static int request_handle_general(request_t *req, size_t offset);
static int request_handle_connection(request_t *req, size_t offset);
static int request_handle_content_length(request_t *req, size_t offset);
static int request_handle_transfer_encoding(request_t *req, size_t offset);

static header_fun fun_list[] = {
	/*general header*/
	HEADER_PAIR(    cache_control,  	request_handle_general),
	HEADER_PAIR(    connection,			request_handle_connection),
	HEADER_PAIR(    date,           	request_handle_general),
	HEADER_PAIR(    pragma,         	request_handle_general),//
	HEADER_PAIR(    trailer,        	request_handle_general),//
	HEADER_PAIR(    transfer_encoding, 	request_handle_transfer_encoding),
	HEADER_PAIR(    upgrade,        	request_handle_general),//
	HEADER_PAIR(    via,            	request_handle_general),//
	HEADER_PAIR(    warning,        	request_handle_general),//
	/*request header*/
	HEADER_PAIR(    accept,         	request_handle_general),
	HEADER_PAIR(    accept_charset, 	request_handle_general),
	HEADER_PAIR(    accept_encoding, 	request_handle_general),
	HEADER_PAIR(    accept_language, 	request_handle_general),
	HEADER_PAIR(    authorization, 		request_handle_general),
	HEADER_PAIR(    expect, 			request_handle_general),
	HEADER_PAIR(    from, 				request_handle_general),
	HEADER_PAIR(    host, 				request_handle_general),
	HEADER_PAIR(    ifmatch, 			request_handle_general),
	HEADER_PAIR(    if_modified_since, 	request_handle_general),
	HEADER_PAIR(    if_none_match, 		request_handle_general),
	HEADER_PAIR(    if_range, 			request_handle_general),
	HEADER_PAIR(    if_unmodified_since,request_handle_general),
	HEADER_PAIR(    max_forwards, 		request_handle_general),
	HEADER_PAIR(    proxy_authorization,request_handle_general),
	HEADER_PAIR(    range, 				request_handle_general),
	HEADER_PAIR(    referer, 			request_handle_general),
	HEADER_PAIR(    te, 				request_handle_general),
	HEADER_PAIR(    user_agent, 		request_handle_general),
	/*entity header*/
	HEADER_PAIR(    allow, 				request_handle_general),
	HEADER_PAIR(    content_encoding, 	request_handle_general),
	HEADER_PAIR(    content_language, 	request_handle_general),
	HEADER_PAIR(    content_length, 	request_handle_content_length),
	HEADER_PAIR(    content_location, 	request_handle_general),
	HEADER_PAIR(    content_md5, 		request_handle_general),
	HEADER_PAIR(    content_range, 		request_handle_general),
	HEADER_PAIR(    content_type, 		request_handle_general),
	HEADER_PAIR(    expires, 			request_handle_general),
	HEADER_PAIR(    last_modified, 		request_handle_general),
};
map_t* map_header_handler;
///////////////////////////////////////////////////////////////
void map_header_init()
{
	map_header_handler = map_init();
	int n = sizeof(fun_list) / sizeof(fun_list[0]);
	//map_t* mp, const string_t* str, void* val
	for(int i = 0; i < n; i++)
		map_insert(map_header_handler, &(fun_list[i].name), &(fun_list[i]));
	return ;
}

void map_header_clear()
{
	map_clear(map_header_handler);
}
///////////////////////////////////////////////////////////////
int request_init(request_t *req, struct connections *con)
{
	if( req == NULL )
		assert(req);
	memset(req, 0, sizeof(request_t));
	req->con = con;
    req->req_buf = buffer_init();
    req->res_buf = buffer_init();
    if(req->req_buf == NULL || req->res_buf == NULL )
        return ERROR;
    parse_archive_init(req->req_buf, &req->parser);

    req->fd = -1;
    req->status = 200;
    req->req_handler = request_handle_request_line;
    req->res_handler = response_handle_send_buffer;
    return OK;
}
// 重用请求？
int request_resue(request_t *req)
{
	buffer_t* rb = req->req_buf;
	buffer_t* sb = req->res_buf;
	buffer_clear(rb);//注意要清空啊
	buffer_clear(sb);
	connection_t* con = req->con;
	memset(req, 0, sizeof(request_t));
	req->con = con;
    req->req_buf = rb;
    req->res_buf = sb;
    parse_archive_init(req->req_buf, &req->parser);

    req->fd = -1;
    req->status = 200;
    req->req_handler = request_handle_request_line;
    req->res_handler = response_handle_send_buffer;
    return OK;
}
int request_check_http_version(request_t* req)
{
	//wrong version
	if((req->parser).version.major != 1 || (req->parser).version.minor > 1)
	{
		// ERR_ON(true, "505 request http version error");
		return ERROR;
	}//default open keep_alive in http1.1
	else if((req->parser).version.major == 1 && (req->parser).version.minor == 1) 
		(req->parser).keep_alive = true;

	return OK;
}
int request_check_resource(request_t* req)
{
	//get relative path and attemp to open file

	char *ref_path = (req->parser).url.ref_path.data;
	int len = (req->parser).url.ref_path.len;
	assert(*ref_path);//
	//if relative path was root, then add prefix.it meannings at now dir, otherwise remove slash.
	if(ref_path[0] == '/' && len == 1)
		ref_path = "./";
	else 
	{
		// ref_path = (char*)malloc(sizeof(char) * len + 2);
		// for(int i = 0; i < len; i++)
		// {
		// 	if((req->parser).url.ref_path.data[i+1]!=' ')
		// 		ref_path[i] = (req->parser).url.ref_path.data[i + 1];
		// 	else {
		// 		ref_path[i] = '\0';
		// 		break;
		// 	}
		// }
		ref_path[len] = '\0';
		ref_path++;
	}
	// //
	// printf("绝对%d-[", (req->parser).url.abs_path.len);
	// string_print(&(req->parser).url.abs_path);
	// printf("]结束\n");
	// printf("相对%d-[", (req->parser).url.ref_path.len);
	// //string_print(&(req->parser).url.ref_path);
	// printf("%s\n", ref_path);
	// printf("]结束\n");
	// // //
	// 	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	// 	for(int i = 0; i < (req->parser).url.abs_path.len; i++)
	// 		printf("%c", (req->parser).url.abs_path.data[i]);
	// 	printf("\n");
	// 	for(int i = 0; i < (req->parser).url.ref_path.len; i++)
	// 		printf("%c", (req->parser).url.ref_path.data[i]);
	// 	printf("\n");
 //   		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int fd = openat(server_config.rootdir_fd, ref_path, O_RDONLY);
	if(fd == -1)
	{
		// printf("[");
		// for(int i = 0; i < strlen(server_config.rootdir); i++)
		// 	printf("%c", server_config.rootdir[i]);
		// printf("]\n");
		// ERR_ON(true, "404 request resource not found");
		// return OK;
    
		return ERROR;
	}
	// printf("~~!!!!!!!!!!!!!!!!!!!!!fddddddddddddddddd%d~~\n", fd);
	// string_print(&(req->parser).url.extension_MIME);
	// printf("~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!~~\n");
	//if file is dir, default make "index.html" as fd.
	struct stat filestat;
	fstat(fd, &filestat);
	if(S_ISDIR(filestat.st_mode))
	{
		int homefd = openat(fd, "index.html", O_RDONLY);
		// printf("%d\n", homefd);
		if(homefd == -1)
		{
			ERR_ON(true , "404");
			//assert(0);
			return ERROR;
		}
		// write(homefd, "123", 4);
		// assert(0);
		string_set(&(req->parser).url.extension_MIME, "html");
		fstat(homefd, &filestat);
		close(fd);
		fd = homefd;
	}
	//update info in connection 
	req->fd = fd;
	req->size = filestat.st_size;
	req->req_handler = request_handle_headers_field;//
	//assert(0);
	return OK;
}

int request_handle_request_line(request_t *req)// first 
{
	//parse request line
	// printf("2$$$$$\n");fflush(stdout);
	int stat = parse_request_line(req->req_buf, &req->parser);
	// 	printf("*********************************************\n%s %d\n", __FUNCTION__, __LINE__);
	// // string_print(&con->req_info.parser.url_str);
 // 	printf("[%s] %lu\n*********************************************\n", req->parser.next_pos, req->req_buf->len);
	if(stat == AGAIN)
	{
		// printf("@\n");
		return AGAIN;
	}
	else if(stat == ERROR)
	{
		ERR_ON(stat == ERROR, "400 parse request line faild");
		// return OK;

		assert(0);
		return response_build_error(req, 400);
	}
	// printf("3$$$$$\n");fflush(stdout);
	//check version
	if(request_check_http_version(req) == ERROR)
		response_build_error(req, 505);
	//check uri
	if(request_check_resource(req) == ERROR)
		response_build_error(req, 404);
	
	return OK;
}

int request_handle_headers_field(request_t *req)// then
{
	int flag = 0;
	int stat;
	header_fun *headfun;
	// printf("4$$$$$\n");fflush(stdout);
	while(true)
	{
		if(flag == 1)
			break;
		stat = parse_header_line(req->req_buf, &req->parser);
		//printf("!%d\n", stat);
		switch(stat)
		{
			case AGAIN:
				return AGAIN;
			case EMPTY_LINE:
				flag = 1;
				break;
			case OK:
				headfun = map_find(map_header_handler, &req->parser.header_key);
				if( headfun == NULL || headfun == ERROR)
					break;
				if(headfun->fun != NULL)
				{
					//string_print(&headfun->name);
					//printf("%p\n", headfun->fun);
					stat = headfun->fun(req, headfun->offset);
					if(stat != OK)
						return OK;
				}
				break;
					printf("----------\n");
		}
	}
	req->req_handler = request_handle_body;
	// printf("5$$$$$\n");fflush(stdout);
	return OK;
}

int request_handle_body(request_t *req)//final
{
	int stat;
	// printf("6$$$$$\n");fflush(stdout);
	switch(req->parser.transfer_encoding)
	{
		case TE_IDENTITY:
			stat = parse_header_body_identity(req->req_buf, &req->parser);
			break;
		case TE_CHUNKED:
			//parse_header_body_chunked();	
		default:
			stat = ERROR;
			break;
	}
	// printf("7$$$$$\n");fflush(stdout);
	// printf("body stat:%d\n", stat);
	switch(stat)
	{
		case AGAIN:
			return AGAIN;
		case OK://parse complete.
			//pipelining?
			//listen out and stop listen in
			connection_open_out(epoll_fd, req->con);
			connection_close_in(epoll_fd, req->con);
			req->req_handler = NULL;
			/////response
			response_build(req);
			return OK;
		default:
			ERR_ON(true, "501 request ");
			return response_build_error(req, 501);

	}
	// printf("8$$$$$\n");fflush(stdout);
	assert(0);
	return OK;
}
//从请求中recv数据
int request_recv(request_t *req)
{
	char buf[2048];
	int len;
	while(true)
	{
		len = recv(req->con->fd, buf, sizeof(buf), 0);
		//printf("\n%s\n len:%d\n", req->req_buf->data, len);
		if(len == 0)
		{
			buffer_clear(req->req_buf);
			return OK;
		}
		if(len == ERROR)
		{
			if(errno != EAGAIN)
			{
				print_log(1, "recv: %d", errno);
				return ERROR;
			}
			else return AGAIN;
		}
		buffer_cat(req->req_buf, buf, len);
	}
	return AGAIN;
}
//core
int request_handle(struct connections *con)
{
	request_t* req = &(con->req_info);
	//mark
	//int stat = request_recv(req);
	int stat = buffer_recv(req->req_buf, con->fd);
	if(stat == ERROR || stat == OK)//when client close, recv OK
		return ERROR;
	while(true)// loop to change function to deal with
	{

		stat = req->req_handler(req);
		if(stat != OK || req->req_handler == NULL)
			break;
	}
	return stat;

}

////////////////////////////////////////////////////////////////////
int request_handle_general(request_t *req, size_t offset)
{
	//trap  指针加1会移动对应类型所占的字节数..要先转换
	size_t ad = &((req->parser).req_headers);
	string_t *headers = (ad + offset);
	//assert(0);
	//printf("%p, %p\n",&(req->parser).req_headers.host, headers);
	*headers = (req->parser).header_val;
	return OK;
}

int request_handle_connection(request_t *req, size_t offset)
{
	request_handle_general(req, offset);
	string_t str = req->parser.req_headers.connection;
	if(string_ncase_equal(&str, &SSTRING("keep-alive")))
		req->parser.keep_alive = true;
	else if(string_ncase_equal(&str, &SSTRING("close")))
		req->parser.keep_alive = false;
	else {// error connction:"val"
		printf("[");

		string_print(&str);
	printf("]\n");
		ERR_ON(true, "400 error header connection opt val");
		//assert(0);
		// return OK; 
		return response_build_error(req, 400);
	}
	return OK;
}
int request_handle_content_length(request_t *req, size_t offset)
{
	request_handle_general(req, offset);
	string_t str = req->parser.req_headers.content_length;
	int len = atoi(str.data);
	if(len <= 0)
	{
		ERR_ON(true, "400 error header content_length opt val");
		assert(0);
		// return OK;
		return response_build_error(req, 400);
	}
	req->parser.content_length = len;
	return OK;
}
int request_handle_transfer_encoding(request_t *req, size_t offset)
{
	request_handle_general(req, offset);
	string_t str = req->parser.req_headers.transfer_encoding;
	if (string_ncase_equal(&str, &SSTRING("chunked")))// not support
	{
		req->parser.transfer_encoding = TE_CHUNKED;
		return response_build_error(req, 501);
	}
	else if (string_ncase_equal(&str, &SSTRING("compress"))) 
	{
		req->parser.transfer_encoding = TE_COMPRESS;
		return response_build_error(req, 501);
	} 
	else if (string_ncase_equal(&str, &SSTRING("deflate"))) 
	{
		req->parser.transfer_encoding = TE_DEFLATE;
		return response_build_error(req, 501);
   	} 
    else if (string_ncase_equal(&str, &SSTRING("gzip"))) 
    {
		req->parser.transfer_encoding = TE_GZIP;
		return response_build_error(req, 501);

    }
    else if (string_ncase_equal(&str, &SSTRING("identity"))) 
    {
    	req->parser.transfer_encoding = TE_IDENTITY;
    	return response_build_error(req, 501);
    }
	else{
		// printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		return response_build_error(req, 400);
	} 
	return OK;
}



////////////////////////////////////////////////////
static int response_send(request_t *req)
{
    int len = 0;
    buffer_t *b = req->res_buf;
    char *buf_beg;
    while (true)
    {
        buf_beg = b->data + req->parser.buffer_sent;
        len = send(req->con->fd, buf_beg, buffer_now_end(b) - buf_beg, 0);
        if (len == 0)//send complete, clear buffer
        {
            buffer_clear(b);
            req->parser.buffer_sent = 0;
            return OK;
        }
        else if (len < 0)
        {
            if (errno == EAGAIN)
                return AGAIN;
            print_log(7, "send: %s", strerror(errno));
            return ERROR;
        }
        req->parser.buffer_sent += len;
    }
    return OK;
}
int response_handle(struct connections *con)
{
    request_t *req = &con->req_info;
    int stat;
    while(true)
    {
    	stat = req->res_handler(req);
    	if(stat != OK || req->parser.response_done == true)
    		break;
    }
    if (req->parser.response_done)   // response done
    {
        if (req->parser.keep_alive)
        {
            request_resue(req);
            connection_close_out(epoll_fd, con);
            connection_open_in(epoll_fd, con);
        }
        else
            return ERROR; // make connection expired
    }
    return stat;
}

int response_handle_send_buffer(request_t *req)//send response reporter
{
    //int stat = response_send(req);
    int stat = buffer_send(req->res_buf, req->con->fd);
    if (stat != OK)
    {
        return stat;
    }
    else
    {
        if (req->fd != -1)
        {
            req->res_handler = response_handle_send_file;
            return OK;
        }
        req->parser.response_done = true;
        connection_close_out(epoll_fd, req->con);
        return OK;
    }
    return OK;
}

int response_handle_send_file(request_t *req)//send static resource
{
    int len;
    connection_t *con = req->con;
    while (true)
    {
        // zero copy, make it faster
        len = sendfile(con->fd, req->fd, NULL, req->size);
        if (len == 0)
        {
            req->parser.response_done = true;
            close(req->fd);
            return OK;
        }
        else if (len < 0)
        {
            if (errno == EAGAIN)
            {
                return AGAIN;
            }
            print_log(7, "sendfile: %s", strerror(errno));
            return ERROR;
        }
    }
    return OK;
}






