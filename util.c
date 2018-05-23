#include "util.h"



/*分级日志*/
void print_log(int level, const char* format, ...)
{
	time_t t = time(NULL);
	struct tm tm = *(localtime(&t));
	FILE *fp = fopen("run_log", "a+");
	if(fp == NULL)
		return ;
	fprintf(fp, "[%4d-%02d-%02d %02d:%02d:%02d] [oid: %5d][L: %02d]",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec, getpid(), level);

	va_list args;
	va_start(args, format);
	vfprintf(fp, format, args);
	fflush(fp);
	va_end(args);
	fprintf(fp, "\n");
	fclose(fp);
}