#ifndef __S_STRINGH__
#define __S_STRINGH__ 

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h" 

typedef struct {
	char *data;
	int len;
} string_t;

#define SSTRING(str) (string_t){str, sizeof(str) - 1}


static inline void string_init(string_t *str)
{
	str->data = NULL;
	str->len = 0;
}

static inline void string_free(string_t *str)
{
	free(str->data);
	str->len = 0;
}

/* 使用strlen会有问题 不能准确获取字符串的长度 */
static inline void string_set(string_t *str, const char* ss)
{
	str->data = (char *)ss;
	str->len = strlen(ss);
}

extern bool string_equal(const string_t *str,  const string_t* ss);
extern void string_print(const string_t *str);
extern int string_cmp(const string_t *str1, const string_t *str2);



#endif
