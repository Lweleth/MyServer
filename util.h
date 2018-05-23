#ifndef __UTILH__
#define __UTILH__ 

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

#define OK 		(0)
#define AGAIN   (1)
#define ERROR   (-1)
#define CRLF "\r\n"


#define ERR_ON(cond, msg)                  						\
	do {														\
		if(cond) {												\
			fprintf(stderr, "%s: %d: ", __FILE__, __LINE__);	\
			perror(msg);										\
		}														\
	} while(0)                              					

#define ABORT_ON(cond, msg) 									\
	do{															\
		if(cond) {												\
			fprintf(stderr, "%s: %d ", __FILE__, __LINE__);		\
			perror(msg);										\
			abort();											\
		}														\
	} while(0)													

extern void print_log(int level, const char* format, ...);



#endif