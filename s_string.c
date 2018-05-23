#include "s_string.h"

bool string_equal(const string_t *str, const string_t *ss)
{
	/*char *a, *b;
	a = str->data, b = (char *)ss;
	if(str->len != sizeof(ss) - 1)
		return false;
	for( ;*a != '\0' && *b != '\0' && *a++ == *b++;){}
	return *a == *b;*/
	return string_cmp(str, ss) == 0 ? true : false;
}


void string_print(const string_t *str)
{
	if( str == NULL || str->data == NULL )
		return ;
	for(int i = 0; i < str->len; i++)
	{
		putchar(str->data[i]);
		fflush(stdout);
	}
	//putchar('\n');
	// printf("%s", str->data);
	return ;
}


int string_cmp(const string_t *str1, const string_t *str2)
{
	if((str1 == str2)
		|| (str1->len == str2->len && str1->len ==0)
		|| (str1->len == str2->len && str1->data == str2->data))
		return 0;
	if(str1->data == NULL || str2->data == NULL)
		return str1->data==NULL ? -1 : 1;
	int len1 = str1->len, 
		len2 = str2->len;
	int minlen = len1 > len2 ? len2 : len1;
	int pos = 0;
	while(pos < minlen && str1->data[pos] == str2->data[pos])
		pos++;
	if( pos != minlen && str1->data[pos] != str2->data[pos] )
		return str1->data[pos] > str2->data[pos] ? 1 : -1;
	if( len1 == len2 ) 
		return 0;
	return  len1 > len2 ? 1 : -1;
}

int string_ncase_equal(const string_t *str,  const string_t* ss)
{
	if(str->len != ss->len)
		return 0;
	int len = str->len;
	for(int i = 0; i < len; i++)
	{
		if(str->data[i] != ss->data[i])
		{
			if((str->data[i] >= 'A' && str->data[i] <= 'Z' && (ss->data[i] - 'a' + 'A') == str->data[i])
			|| (str->data[i] >= 'a' && str->data[i] <= 'z' && (ss->data[i] - 'A' + 'a') == str->data[i]))
				continue;
			else 
				return 0;
		}
	}
	return 1;
}
/*
int main()
{
	string_t a = SSTRING("123456789\0\0");
	string_t b = SSTRING("123456789\0");
	string_t c = SSTRING("123456789\0");
	printf("%d~%d~%d\n", a.len, b.len, c.len);
	printf("%d", string_equal(&a, &SSTRING("123456789")));
}*/