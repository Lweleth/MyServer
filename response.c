#define _GNU_SOURCE
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "buffer.h"
#include "map.h"
#include "util.h"
#include "connection.h"
#include "request.h"
#include "response.h"
#include "server.h"
#include "s_string.h"

#define ERR_HEADER_MAX_LEN (1024 << 1)


#define MIME_PAIR(mime, desc) { SSTRING(#mime), SSTRING(desc) }

static string_t mime_list[][2] =
{
    MIME_PAIR(word, "application/msword"),
    MIME_PAIR(pdf,  "application/pdf"),
    MIME_PAIR(zip,  "application/zip"),
    MIME_PAIR(js,   "application/javascript"),
    MIME_PAIR(gif,  "image/gif"),
    MIME_PAIR(jpeg, "image/jpeg"),
    MIME_PAIR(jpg,  "image/jpeg"),
    MIME_PAIR(png,  "image/png"),
    MIME_PAIR(css,  "text/css"),
    MIME_PAIR(html, "text/html"),
    MIME_PAIR(htm,  "text/html"),
    MIME_PAIR(txt,  "text/plain"),
    MIME_PAIR(xml,  "text/xml"),
    MIME_PAIR(svg,  "image/svg+xml"),
    MIME_PAIR(mp4,  "video/mp4"),
};


static map_t mime_map;
void map_mime_init()
{
    size_t len = sizeof(mime_list) / sizeof(mime_list[0]);
    map_init(&mime_map);
    for (int i = 0; i < len; i++)
    {
        map_insert(&mime_map, &mime_list[i][0], &mime_list[i][1]);
    }
}

void map_mime_free()
{
    map_clear(&mime_map);
}

static const char *status_table[512];
void status_table_init()
{
    memset(status_table, 0, sizeof(status_table));
#define XX(num, name, string) status_table[num] = #num " " #string;
    HTTP_STATUS_MAP(XX);
#undef XX
}
///////////////////////////////////////////////////////////
static err_page_t err_page;

int err_page_init()
{
    err_page_t *ep = &err_page;
    // open error.html
    ep->fd = openat(server_config.rootdir_fd, "error.html", O_RDONLY);
    ABORT_ON(ep->fd == ERROR, "openat");
    struct stat st;
    fstat(ep->fd, &st);
    ep->raw_size = st.st_size;

    ep->ren_page = buffer_init(ep->raw_size + ERR_HEADER_MAX_LEN);
    ABORT_ON(ep->ren_page == NULL, "buffer_init");

    ep->raw_page = mmap(NULL, ep->raw_size, PROT_READ, MAP_SHARED, ep->fd, 0);
    ABORT_ON(ep->raw_page == NULL, "mmap");
    return OK;
}

void err_page_free()
{
    err_page_t *ep = &err_page;
    buffer_free(ep->ren_page);
    // munmap
    munmap((void *)ep->raw_page, ep->raw_size);
    close(ep->fd);
}
///////////////////////////////////////////////////////////
void response_build_status_line(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    if (r->parser.version.minor == 1)
        r->res_buf = buffer_cat_cstr(b, "HTTP/1.1 ");
    else
        r->res_buf = buffer_cat_cstr(b, "HTTP/1.0 ");
    // status
    const char *status_str = status_table[r->status];
    if (status_str != NULL)
        r->res_buf = buffer_cat_cstr(b, status_str);
    r->res_buf = buffer_cat_cstr(b, CRLF);
}

void response_build_date(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    time_t now = time(NULL);
    struct tm *tm = gmtime(&now);
    size_t len = strftime((b->data + b->len), b->spr,"Date: %a, %d %b %Y %H:%M:%S GMT\r\n", tm);
    b->len += len;
    b->spr -= len;
}

void response_build_server(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    r->res_buf = buffer_cat_cstr(b, "Server: ");
    r->res_buf = buffer_cat_cstr(b,  "HTTP SERVER v-0.1"CRLF);
}

void response_build_content_type(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    header_parser_t *ar = &r->parser;

    string_t content_type;
    string_t *mime;
    if (ar->req_error == true)
        content_type = SSTRING("text/html");
    else 
    {
        mime = map_find(&mime_map, &ar->url.extension_MIME);
        if (mime != -1 && mime != NULL)
            content_type = *mime;
        else
            content_type = SSTRING("text/html");    
    }
    r->res_buf = buffer_cat_cstr(b, "Content-Type: ");
    r->res_buf = buffer_cat_cstr(b, content_type.data);
    r->res_buf = buffer_cat_cstr(b, CRLF);
}

void response_build_content_length(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    char cl[128];
    int len;
    buffer_t *ren_page = err_page.ren_page;
    // modify content_length when sending err page
    if (r->parser.req_error)
    {
        len = snprintf(ren_page->data, ren_page->spr + ren_page->len, err_page.raw_page, status_table[r->status]);
        err_page.ren_size = len;
    }
    else
        len = r->size;
    sprintf(cl,"Content-Length: %d\r\n", len);
    r->res_buf = buffer_cat_cstr(b, cl);
}

void response_build_connection(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    string_t connection;
    if (r->parser.keep_alive)
        connection = SSTRING("Connection: keep-alive");
    else
        connection = SSTRING("Connection: close");
    r->res_buf = buffer_cat_cstr(b, connection.data);
    r->res_buf = buffer_cat_cstr(b, CRLF);
}

void response_build_crlf(struct request_info *r)
{
    buffer_t *b = r->res_buf;
    r->res_buf = buffer_cat_cstr(b, CRLF);
}

int response_build(request_t *req)//build response
{
    printf("9$$$$$\n");fflush(stdout);
    response_build_status_line(req);
    response_build_date(req);
    response_build_server(req);
    response_build_content_type(req);
    response_build_content_length(req);
    response_build_connection(req);
    response_build_crlf(req);
    printf("10$$$$$\n");fflush(stdout);
    //assert(0);
    return OK;
}

int response_build_error(request_t *req, int status_code)
{
    req->req_handler = NULL;
    req->parser.req_error = true;
    req->status = status_code;
    req->parser.keep_alive = false;

    response_build_status_line(req);
    response_build_date(req);
    response_build_server(req);
    response_build_content_type(req);
    response_build_content_length(req);
    response_build_connection(req);
    response_build_crlf(req);

    req->res_buf = buffer_cat_cstr(req->res_buf, err_page.ren_page->data);
    connection_close_in(epoll_fd, req->con);
    connection_open_out(epoll_fd, req->con);
    req->parser.response_done = true;
    return OK;
}

