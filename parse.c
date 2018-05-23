#include "parse.h"
#include "buffer.h"
// #define DEBUG
#   define ALLOWED_IN_ABS_PATH      			\
		/*多级目录*/								\
			 'A' ... 'Z':          				\
		case 'a' ... 'z':          				\
		case '0' ... '9':          				\
		/* Mark */                 				\
		case '-':case '_':case '!':case '~':	\
		case '*':case '\'':case '(':case ')':   \
		/* Escaped */              				\
		case '%':case ':':case '@':case '&':    \
		case '=':case '+':case '$':case ',':    \
		case ';'


#   define ALLOWED_IN_EXTENSION     ALLOWED_IN_ABS_PATH

		/* Query */
#   define ALLOWED_IN_QUERY         \
             '.':                   \
        case '/':                   \
        case '?':                   \
        case '#':					\
        case ALLOWED_IN_ABS_PATH
#ifdef DEBUG
char sssss[][50] ={
    "S_RE_BEGIN",
    "S_RE_METHOD",// 
    "S_RE_SPACES_BEFORE_URI",
    // <-URI's 
    "S_RE_CHECK_URI",

    "S_RE_VERSION_SPACES_BEFORE",// X/[ ]HTTP
    "S_RE_VERSION_H",
    "S_RE_VERSION_HT",
    "S_RE_VERSION_HTT",
	"S_RE_VERSION_HTTP",
    "S_RE_VERSION_SLASH", // HTTP/
    "S_RE_VERSION_MAJOR", // HTTP/1
    "S_RE_VERSION_DOT",   // HTTP/1.
    "S_RE_VERSION_MINOR", // HTTP/1.X
    "S_RE_VERSION_SPACES_AFTER",
    "S_RE_ALMOST_DONE",
    "S_RE_DONE",

    //header-field's
    "S_HF_BEGIN",
    "S_HF_SPEC",// IGNORE
    "S_HF_NAME",// xD
    "S_HF_COLON",// x:
    "S_HF_SPACE_BEFORE_VALUE", // x:[ ]value
    "S_HF_VALUE",// x: value
    "S_HF_SPACE_AFTER_VALUE",// x: value[ ]
    "S_HF_ALMOST_DONE",
    "S_HF_DONE",


    //URI's 
    "S_URI_BEGIN",
    "S_URI_SCHEME",
    "S_URI_SCHEME_COLON",
    "S_URI_SCHEME_SLASH",
    "S_URI_SCHEME_DSLASH",
    "S_URI_HOST",
    "S_URI_HOST_COLON",
    "S_URI_HOST_PORT",
    "S_URI_ABS_PATH_DOT",
    "S_URI_ABS_PATH_DDOT",
    "S_URI_ABS_PATH_SLASH",
    "S_URI_ABS_PATH_ENTRY",
    "S_URI_EXTENSION_MIME",
    "S_URI_QUERY",
    "S_URI_END"
};
#endif
void parse_archive_init(buffer_t *b, header_parser_t *ar)
{
    memset(ar, 0, sizeof(header_parser_t));
    ar->next_pos = b->data;
    ar->isCRLF = true;
    ar->content_length = -1;
}

static inline method_t parse_method(char *begin, char *end)
{
	size_t len = end - begin;
	switch(len)	{
		case 3:
			if(strncmp(begin, "GET", len) == 0)
				return MD_GET;
			else if(strncmp(begin, "PUT", len) == 0)
				return MD_PUT;
			break;
		case 4:
			if(strncmp(begin, "POST", len) == 0)
				return MD_POST;
			else if(strncmp(begin, "HEAD", len) == 0)
				return MD_HEAD;
			break;
		case 5:
			if(strncmp(begin, "TRACE", len) == 0)
				return MD_TRACE;
			break;
		case 6:
			if(strncmp(begin, "DELETE", len) == 0)
				return MD_DELETE;
			break;
		case 7:
			if(strncmp(begin, "OPTIONS", len) == 0)
				return MD_OPTIONS;
			break;
		default:
			return MD_INVALID;
	}
	return MD_INVALID;
}

static inline int parse_uri(char *begin, char* end, header_parser_t *ar)
{
	ar->url_str.data = begin;
	ar->url_str.len = end - begin;
	uri_t *uri;
	uri = &(ar->url);
	int stat = S_URI_BEGIN;
	char *pos;
	char ch;
	for(pos = begin; pos <= end; pos++)
	{
		ch = *pos;
		#ifdef DEBUG
			printf("%d-%c s:[%s]\n", pos, ch, sssss[stat]);
		#endif
		switch(stat){
			//
			case S_URI_BEGIN:
				switch(ch){
					case '/':
						stat = S_URI_ABS_PATH_SLASH;//
						uri->abs_path.data = pos;
						uri->ref_path.data = pos;
						uri->ref_path.len = 1;
						break;
					case 'A' ... 'Z':
					case 'a' ... 'z':
						uri->scheme.data = pos;
						stat = S_URI_SCHEME;//
						break;
					default:
						return ERROR;
				}
				break;
			//file[:]
			case S_URI_SCHEME:
				switch(ch){
					case 'A' ... 'Z':
					case 'a' ... 'z':
					case '0' ... '9':
					case '+': case '-': case '.':
						break;
					case ':':
						uri->scheme.len = pos - uri->scheme.data;
						stat = S_URI_SCHEME_COLON;//
						break;
					default:
						return ERROR;
				}
				break;
			//file:[/]
		    case S_URI_SCHEME_COLON:
		    	switch(ch){
		    		case '/':
		    			stat = S_URI_SCHEME_SLASH;//
		    			break;
		    		default:
		    			return ERROR;
		    	}
		    	break;
		    //file:/[/]
		    case S_URI_SCHEME_SLASH:
		    	switch(ch){
		    		case '/':
		    			stat = S_URI_SCHEME_DSLASH;//
		    			break;
		    		default:
		    			return ERROR;
		    	}
		    	break;
		    //file://[1]0.x.x.x
		    case S_URI_SCHEME_DSLASH:
		    	switch(ch){
		    		case 'A' ... 'Z':
		    		case 'a' ... 'z':
		    		case '0' ... '9':
		    			uri->host.data = pos;
		    			stat = S_URI_HOST;//
		    			break;
		    		default:
		    			return ERROR;
		    	}
		    	break;
		    //file://10.0.0.1[:/]
		    case S_URI_HOST:
		    	switch(ch){
		    		case 'A' ... 'Z':
		    		case 'a' ... 'z':
		    		case '0' ... '9':
		    		case '.': case '-': case '+':
				#ifndef ALLOW_UNDERSCORE
		    		case '_':
				#endif
		    			break;
		    		case ':':
		    			uri->host.len = pos - uri->host.data;
		    			stat = S_URI_HOST_COLON;
		    			break;
		    		case '/': //没有端口号直接来到 绝对路径
		    			uri->host.len = pos - uri->host.data;
		    			uri->ref_path.data = pos;
		    			uri->ref_path.len = 1;
		    			stat = S_URI_ABS_PATH_SLASH;//
		    			break;
		    		default:
		    			return ERROR;
		    	}
		    	break;
		    //xx.xx.xx.xx:[8]080
		    case S_URI_HOST_COLON:
		    	switch(ch){
		    		case '0' ... '9':
		    			uri->port.data = pos;
		    			stat = S_URI_HOST_PORT;//
		    			break;
		    		default:
		    			return ERROR;
		    	}
		    	break;
		    //:[ /]
		    case S_URI_HOST_PORT:
		    	switch(ch) {
		    		case '0' ... '9':
		    			break;
		    		case ' ':
		    		case '/'://端口号结束 相对路径开始
		    			uri->port.len = pos - uri->port.data;
		    			uri->ref_path.data = pos;
		    			uri->ref_path.len = 1;
		    			stat = S_URI_ABS_PATH_SLASH;//
		    			break;
		    		default:
		    			return ERROR;
		    	}
		    	break;
		    // :8080/[*]
		    case S_URI_ABS_PATH_SLASH:
		    	switch(ch){
		    		case ' '://URI结束 
		    			uri->abs_path.data = begin;
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;

		    			uri->query.data = pos;
		    			uri->query.len = 0;
		    			stat = S_URI_END;//
		    			break;
		    		case '.'://dot
		    			stat = S_URI_ABS_PATH_DOT;//
		    			break;
		    		case '?'://query
		    			uri->abs_path.data = begin;
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;

		    			stat = S_URI_QUERY;//
		    			//begin = pos + 1
		    			uri->query.data = pos;
		    			break;
		    		case '/'://双斜杠省略
		    			break;
		    		//多级目录
		    		case ALLOWED_IN_ABS_PATH:
						stat = S_URI_ABS_PATH_ENTRY;//
						break;
					default:
						return ERROR;
		    	}
		    	break;
		    // /.[ .?/]
		    // /. fin
		   	// /.[.]
		    // /.?
		   	// /.[j]pg 
		    case S_URI_ABS_PATH_DOT:
		    	switch(ch){
		    		case ' ':
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;
		    			stat = S_URI_END;
		    			break;
		    		case '.':
		    			stat = S_URI_ABS_PATH_DDOT;
		    			break;
		    		case '?':
		    			uri->abs_path.data = begin;
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;
		    			stat = S_URI_QUERY;//

		    			uri->query.data = pos;
		    			break;
		    		case '/':
		    			stat = S_URI_ABS_PATH_SLASH;
		    			break;

					case ALLOWED_IN_ABS_PATH:
						stat = S_URI_EXTENSION_MIME;//
						uri->extension_MIME.data = pos - 1;
						break;
					default:
						return ERROR;
		    	}
		    	break;
		    case S_URI_ABS_PATH_DDOT:
		    	switch(ch){
		    		case ' ':
			    		uri->num_dots++;
			    		uri->abs_path.len = pos - begin;
			    		uri->ref_path.len = pos - uri->ref_path.data;
			    		stat = S_URI_END;
			    		break;
			    	case '.'://第三个点出现 请求文件
			    		uri->extension_MIME.data = pos;
			    		stat = S_URI_EXTENSION_MIME;
			    		break;
			    	case '?':
			    		uri->num_dots++;
		    			uri->abs_path.data = begin;
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;
		    			stat = S_URI_QUERY;//

		    			uri->query.data = pos;
			    	case '/':
			    		uri->num_dots++;
			    		stat = S_URI_ABS_PATH_SLASH;
			    		break;
			    	case ALLOWED_IN_ABS_PATH:
						stat = S_URI_EXTENSION_MIME;//
						uri->extension_MIME.data = pos;
						break;
					default:
						return ERROR;
		    	}
		    	break;
		    case S_URI_ABS_PATH_ENTRY:
		    	switch(ch){
		    		case '/':
		    			if(uri->num_entries >= uri->num_dots)
		    				uri->num_entries++;
		    			stat = S_URI_ABS_PATH_SLASH;
		    			break;
		    		case '.':
		    			uri->extension_MIME.data = pos;
		    			stat = S_URI_EXTENSION_MIME;
		    			break;
		    		case '?':
		    			if(uri->num_entries >= uri->num_dots)
		    				uri->num_entries++;
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;
		    			stat = S_URI_QUERY;

		    			uri->query.data = pos;
		    			break;
		    		case ' ':// /aaba/assdsd[ ]
		    			if(uri->num_entries >= uri->num_dots)
		    				uri->num_entries++;
		    			uri->abs_path.data = begin;
		    			uri->abs_path.len = pos - begin;
		    			uri->ref_path.len = pos - uri->ref_path.data;
		    			stat = S_URI_END;
		    			break;
		    		case ALLOWED_IN_ABS_PATH:
						break;
					default:
						break;
		    	}
		    case S_URI_EXTENSION_MIME:
		    	switch(ch){
			    	case '.':
			    		uri->extension_MIME.data = pos;
			    		break;
			    	case '/': 
			    		if(uri->num_entries >= uri->num_dots)
			    			uri->num_entries++;
			    		uri->extension_MIME.data = NULL;
			    		stat = S_URI_ABS_PATH_SLASH;
			    		break;
			    	case '?':
			    		if(uri->num_entries >= uri->num_dots)
			    			uri->num_entries++;
			    		uri->extension_MIME.data++;
			    		uri->extension_MIME.len = pos - uri->extension_MIME.data;

			    		uri->abs_path.data = begin;//
			    		uri->abs_path.len = pos - begin;//

			    		uri->query.data = pos;
			    		stat = S_URI_QUERY;
			    		break;
			    	case ' ':
			    		if(uri->num_entries >= uri->num_dots)
		    				uri->num_entries++;
		    			uri->extension_MIME.data++;
			    		uri->extension_MIME.len = pos - uri->extension_MIME.data;

			    		uri->abs_path.data = begin;
			    		uri->abs_path.len = pos - begin;

			    		uri->ref_path.len = pos - uri->ref_path.data;

			    		stat = S_URI_END;
			    		break;
			    	case ALLOWED_IN_EXTENSION:
			    		break;
			    	default:
						break;
		    	}
		    	break;
		    case S_URI_QUERY:
		    	switch(ch){
		    		case ' ':
			    		uri->query.data++;
			    		uri->query.len = pos - uri->query.data;
		    			stat = S_URI_END;
		    			break;
		    		case ALLOWED_IN_QUERY:
		    			break;
		    		default:
						break;
		    	}
		    	break;
		    case S_URI_END:
		    	goto uri_parse_done;
		}
	}
uri_parse_done:;
	if(uri->num_entries < uri->num_dots)
		uri->abs_path.len = 1;
	// char *abs_path_end = uri->abs_path.data + uri->abs_path.len;
	// printf("(%d)(%d)\n", uri->abs_path.data, uri->abs_path.len);
	// for(pos = abs_path_end; pos != uri->abs_path.data; pos--)
	// {
	// 	printf("\03addr:[%d] mark\n", pos);
	// 	if(*pos == '.')
	// 	{
	// 		uri->extension_MIME.data = pos + 1;
	// 		uri->extension_MIME.len = abs_path_end - pos - 1;
	// 		break;
	// 	}
	// 	else if(*pos == '/')
	// 		break;
	// }
	return OK;
}












int parse_request_line(buffer_t *b, header_parser_t *ar)
{
	char ch;
	int state;
	char* pos;
	// pos = buffer_now_begin(b);
	pos = ar->next_pos;
	for(; pos < buffer_now_end(b); pos++)
	{
		ch = *pos;
		state = ar->state;
		#ifdef DEBUG
		printf("%d-%c s:[%s]\n", pos, ch, sssss[state]);
		#endif
		switch(state){
			case S_RE_BEGIN:
				switch(ch){
					case 'A' ... 'Z':
						ar->method_begin = pos;
						ar->state = S_RE_METHOD;
						break;
					default:
						return INVALID_REQUEST;
				}
				break;
			case S_RE_METHOD:
				switch(ch){
					case 'A' ... 'Z':// skip method
						break;
					case ' ':
						ar->method = parse_method(ar->method_begin, pos);
#ifdef DEBUG
	printf("~%s~\n", ar->method==2?"POST":"ERR");
#endif
						if(ar->method == MD_INVALID)
							return INVALID_REQUEST;
						ar->state = S_RE_SPACES_BEFORE_URI;
						break;
					default:
						return INVALID_REQUEST;
				}
				break;
			case S_RE_SPACES_BEFORE_URI:
				switch(ch){
					case '\t':
					case '\r':
					case '\n':
						return INVALID_REQUEST;
					case ' ':
						break;
					default:
						ar->uri_begin = pos;//first char of URI 
						ar->state = S_RE_CHECK_URI;
				}
				break;
			case S_RE_CHECK_URI:
				switch(ch){
					case '\t':
					case '\r':
					case '\n':
						return INVALID_REQUEST;
					case ' ':
						ar->state = S_RE_VERSION_SPACES_BEFORE;
#ifdef DEBUG
	printf("enter statr:[%d] end:[%d]!\n",ar->uri_begin, pos);
#endif
						int status = parse_uri(ar->uri_begin, pos, ar);
						if(status)
							return status;
#ifdef DEBUG
						printf("exit from uri parse\n");
#endif
						break;
					default:
						break;
				}
				break;
			case S_RE_VERSION_SPACES_BEFORE:
				switch (ch) {
			    	case 'H':
			    	case 'h':
			        	ar->state = S_RE_VERSION_H;
			        	break;
			    	default:
			    		return INVALID_REQUEST;
			    }
			    break;
			case S_RE_VERSION_H:
				switch (ch) {
			    	case 'T':
			    	case 't':
			        	ar->state = S_RE_VERSION_HT;
			        	break;
			    	default:
			    		return INVALID_REQUEST;
			    }
			    break;
		   	case S_RE_VERSION_HT:
				switch (ch) {
			    	case 'T':
			    	case 't':
			        	ar->state = S_RE_VERSION_HTT;
			        	break;
			    	default:
			    		return INVALID_REQUEST;
			    }
			    break;
		   	case S_RE_VERSION_HTT:
		   		switch (ch) {
			    	case 'P':
			    	case 'p':
			        	ar->state = S_RE_VERSION_HTTP;
			        	break;
			    	default:
			    		return INVALID_REQUEST;
			    }
			    break;
		   	case S_RE_VERSION_HTTP:
		   		switch (ch) {
			    	case '/':
			        	ar->state = S_RE_VERSION_SLASH;
			        	break;
			    	default:
			    		return INVALID_REQUEST;
			    }
			    break;
		   	case S_RE_VERSION_SLASH: // HTTP/
		   		switch(ch){
		   			case '0' ... '9':
		   				ar->version.major = ar->version.major * 10 + (ch - '0');
		   				ar->state = S_RE_VERSION_MAJOR;
		   				break;
		   			default:
		   				return INVALID_REQUEST;
		   		}
		   		break;
		   	case S_RE_VERSION_MAJOR: // HTTP/1
		   		switch(ch){
		   			case '0' ... '9':
		   				ar->version.major = ar->version.major * 10 + (ch - '0');
		   				if(ar->version.major > 10)
		   					return INVALID_REQUEST;
		   				break;
		   			case '.':
		   				ar->state = S_RE_VERSION_DOT;
		   				break;
		   			default:
		   				return INVALID_REQUEST;
		   		}
		   		break;
		   	case S_RE_VERSION_DOT:   // HTTP/1.
		   		switch(ch){
		   			case '0' ... '9':
		   				ar->version.minor = ar->version.minor * 10 + (ch - '0');
		   				ar->state = S_RE_VERSION_MINOR;
		   				break;
		   			default:
		   				return INVALID_REQUEST;
		   		}
		   		break;
		   	case S_RE_VERSION_MINOR: // HTTP/1.X
		   		switch(ch){
		   			case '0' ... '9':
		   				ar->version.minor = ar->version.minor * 10 + (ch - '0');
		   				if(ar->version.minor > 10)
		   					return INVALID_REQUEST;
		   				break;
		   			case ' ':
		   				ar->state = S_RE_VERSION_SPACES_AFTER;
		   				break;
		   			case '\r':
		   				ar->state = S_RE_ALMOST_DONE;
		   				break;
		   			case '\n':
		   				goto done;
		   			default:
		   				return INVALID_REQUEST;
		   		}
		   		break;
		   	case S_RE_VERSION_SPACES_AFTER:
		   		switch(ch){
		   			case ' ':
		   				break;
		   			case '\r':
		   				ar->state = S_RE_ALMOST_DONE;
		   				break;
		   			case '\n':
		   				return INVALID_REQUEST;
		   			default:
		   				return INVALID_REQUEST;
		   		}
		   		break;
		   	case S_RE_ALMOST_DONE:// CR->LF
		   		switch(ch){
		   			case '\n':
		   				goto done;
		   			default:
		   				return INVALID_REQUEST;
		   		}
		   		break;
    		case S_RE_DONE:
    			break;
		}
	}
	b->offest = b->len;
	ar->next_pos = buffer_now_end(b);
	return AGAIN;
done:;
	b->offest = (pos - b->data) + 1;
	ar->next_pos = pos + 1;
	
	ar->state = S_HF_BEGIN;
	return OK;
}

int parse_header_line(buffer_t *b, header_parser_t *ar)
{
	char ch;
	char *pos;
	int *stat;
	string_t *header_key;
	string_t *header_val;

	stat = &(ar->state);
	header_key = &(ar->header_key);
	header_val = &(ar->header_val);
	// pos = buffer_now_begin(b);
	pos = ar->next_pos;
	for(; pos < buffer_now_end(b); pos++)
	{
		ch = *pos;
		
		switch(*stat) {
			case S_HF_BEGIN:
				header_key->data = NULL, header_key->len = 0;
				header_val->data = NULL, header_val->len = 0;
				switch(ch){
					case 'A' ... 'Z':
						*pos += 'a' - 'A';
						header_key->data = pos;
						*stat = S_HF_NAME;
						break;
					case '-':
						*pos += '_' - '-';
						header_key->data = pos;
						*stat = S_HF_NAME;
						break;
					case 'a' ... 'z':
					case '0' ... '9':
						header_key->data = pos;
						*stat = S_HF_NAME;
						break;
					case '\r'://empty line ?
						*stat = S_HF_ALMOST_DONE;
						break;
					case '\n'://skip line
						goto parse_filed_done;
					default:// ignore spec char 
						*stat = S_HF_SPEC;
						break;
						//return INVALID_REQUEST;
				}
				break;
		    case S_HF_NAME:// xD
		    	switch(ch){
		    		case 'A' ... 'Z':
						*pos += 'a' - 'A';
						break;
					case '-':
						*pos += '_' - '-';
						break;
					case 'a' ... 'z':
					case '0' ... '9':
						break;
					case ':':
						header_key->len = pos - header_key->data;
						*stat = S_HF_COLON;
						break;
					case '\r'://uncomplete key 
						*stat = S_HF_ALMOST_DONE;
						break;
					case '\n'://incorrect key
					default:// ignore spec char
						*stat = S_HF_SPEC;
						break;
						//return INVALID_REQUEST;
		    	}
		    	break;
		    case S_HF_COLON:// x:
		    	switch(ch){
		    		case ' ':
		    			*stat = S_HF_SPACE_BEFORE_VALUE;
		    			break;
		    		case '\r':
		    			*stat = S_HF_ALMOST_DONE;
						break;
					case '\n':
						goto parse_filed_done;
					default:
						header_val->data = pos;
						*stat = S_HF_VALUE;
						break;
		    	}
		    	break;
		    case S_HF_SPACE_BEFORE_VALUE: // x:[ ]value
		    	switch(ch){
		    		case ' ':
		    			*stat = S_HF_SPACE_BEFORE_VALUE;
		    			break;
		    		case '\r'://uncomplete value
		    			*stat = S_HF_ALMOST_DONE;
						break;
					case '\n':
						goto parse_filed_done;
					default:
						header_val->data = pos;
						*stat = S_HF_VALUE;
						break;	
		    	}
		    	break;
		    case S_HF_VALUE:// x: value//转换状态时先确定字段长度
		    	switch(ch){
		    		case ' ':
		    			header_val->len = pos - header_val->data;
		    			*stat = S_HF_SPACE_AFTER_VALUE;
		    			break;
		    		case '\r':
		    			header_val->len = pos - header_val->data;
		    			*stat = S_HF_ALMOST_DONE;
		    			break;
		    		case '\n':
		    			header_val->len = pos - header_val->data;
		    			goto parse_filed_done;
		    		default:
		    			break; 
		    	}
		    	break;
		    case S_HF_SPACE_AFTER_VALUE:// x: value[ ]
		    	switch(ch){
		    		case ' ':
		    			break;
		    		case '\r':// almost complete
		    			*stat = S_HF_ALMOST_DONE;
		    		case '\n'://
		    			goto parse_filed_done;
		    		default://multi val
		    			*stat = S_HF_VALUE;
		    			break;
		    	}
		    	break;
		    case S_HF_SPEC:// IGNORE SPEC char or pause parse
		    	switch(ch){
		    		case '\n':// restart
		    			*stat = S_HF_BEGIN;
		    			break;
		    		default:// ignore incorrect char
		    			break;
		    	}
		    	break;
		    case S_HF_ALMOST_DONE:
		    	switch(ch){
		    		case '\n'://incorrect line or complete line
		    			goto parse_filed_done;
		    		default://
		    			*stat = S_HF_SPEC;
		    			break;
		    	}
		    	break;
		}
	}
	//printf("len:%d\n", buffer_now_end(b) - ar->next_pos);
	ar->next_pos = buffer_now_end(b);
	b->offest = b->len;
	return AGAIN;
parse_filed_done:;
	//printf("len:%d\n", 1 + pos - ar->next_pos);
	ar->next_pos = pos + 1;
	b->offest = (pos - b->data) + 1;

	ar->num_headers++;
	*stat = S_HF_BEGIN;

	if(header_key->data == NULL || header_key->len == 0)
		return EMPTY_LINE;
	else return OK;
}

int parse_header_body_identity(buffer_t *b, header_parser_t *ar)
{
	if(ar->content_length <= 0)
		return OK;
	size_t received = buffer_now_end(b) - ar->next_pos;
	ar->body_recv += received;
#ifdef DEBUG
	printf("content_length:[%d]\n", ar->content_length);
	printf("%s %d\n", __FUNCTION__, __LINE__);
 	printf("[%s] %lu\n", ar->next_pos, received);
#endif
	ar->next_pos = buffer_now_end(b);
	if(ar->body_recv >= ar->content_length)
		return OK;
	return AGAIN;
}
/*
int main()
{
	char a[] = "DELETE";
	int s = parse_method(a, a + 6);
	printf("%d\n", s);
}*/
