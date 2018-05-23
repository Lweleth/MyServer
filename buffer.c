#include "buffer.h"

char* buffer_now_end(buffer_t *buf)
{
    return buf->data + buf->len; 
}

char* buffer_now_begin(buffer_t *buf)
{
    return buf->data + buf->offest;
}

buffer_t* buffer_new(size_t len)
{
    buffer_t *buf = malloc(sizeof(buffer_t));
    if(buf == NULL)
        return NULL;
    buf->len = 0;
    buf->spr = len;
    buf->offest = 0;
    buf->data[len] = '\0';
    return buf;
}

void buffer_free(buffer_t *buf)
{
    if(buf != NULL)
        free(buf);
    return ;
}

void buffer_clear(buffer_t *buf)
{
    buf->spr += buf->len;
    buf->len = 0;
    buf->offest = 0;
}

void buffer_print(const buffer_t *buf)
{
    if( buf == NULL )
        return ;
    for (int i = 0; i < buf->len; i++)
    {
        putchar(buf->data[i]);
        fflush(stdout);
    }
    printf("\n");
}

buffer_t* buffer_cat(buffer_t *buf,char* str, size_t len)
{
    if( buf == NULL)
        return NULL;
    //  throw error 
    if(buf->spr < len)
    {
        printf("buf->spr:%d\nsendlen:%d\n", buf->spr,len);
        assert(false);
    }
    else // not need to
    {
        memcpy(buf->data + buf->len, str, len);
        buf->len += len;
        buf->spr -= len;
        return buf;
    }
}

buffer_t* buffer_cat_cstr(buffer_t *buf, char* str)
{
    return buffer_cat(buf, str, strlen(str));
}

buffer_t* buffer_init()
{
    return buffer_new(BUFFER_SIZE);
}
// 返回三种状态OK, AGAIN, ERROR
// OK: 所有数据已经接收完毕，准备关闭连接符
// AGAIN: 缓冲区已满，等待再次接收
// ERROR: 错误

int buffer_recv(buffer_t* buf, int fd)
{
    while( buf->spr != 0) 
    {
        int real_len = recv(fd, buffer_now_end(buf), buf->spr, 0/*FLAGS*/);
        
        //printf("%s\ntotalspc: %d thislen:%d\n",buf->data,buf->spr,real_len);
        if(real_len == 0)
        {
            buffer_clear(buf);
            buf->offest = 0;
            return OK;
        }
        if(real_len == -1)
        {
            //printf("%s\ntotalspc: %d thislen:%d ERRNO:%d\n",buf->data,buf->spr,real_len, errno);
            //perror("recv");
            //fflush(stdout);
            if(errno == EAGAIN)
                return AGAIN;
            perror("recv");
            return ERROR;
        }
        buf->len += real_len;
        buf->spr -= real_len;
    }
    return AGAIN;
}

int buffer_send(buffer_t* buf, int fd)
{
    while( buffer_remain(buf) > 0)
    {
        int real_len = send(fd, buffer_now_begin(buf), buffer_remain(buf), 0/*flags*/);
        if(real_len == 0)
        {
            buffer_clear(buf);
            buf->offest = 0;
            return OK;
        }
        if(real_len == -1)
        {
            if(errno == EAGAIN)
                return AGAIN;
            perror("send");
            return ERROR;
        }
        buf->offest += real_len;
    }
    return OK;
}
/*
 int main()
 {
    buffer_t* t = buffer_init();
    buffer_t* p = buffer_init();
    buffer_cat(t, "123456789", 11);
    buffer_cat_cstr(p, &SSTRING("123456789"));
    buffer_print(t);
    buffer_print(p);
 }*/