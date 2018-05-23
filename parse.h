#ifndef __PARSEH__
#define  __PARSEH__
// #define DEBUG
#include <string.h>
#include <stdint.h>
#include "util.h"
#include "s_string.h"
#include "buffer.h"

#define INVALID_REQUEST (-1)
#define EMPTY_LINE (2)


/*
         HTTP-message   = start-line   = request-line / status-line
                      *( header-field CRLF )
                      CRLF
                      [ message-body ]
    rfc 2616:
   HTTP header fields, which include 
   general-header (section 4.5),
                      = Cache-Control            ; Section 14.9
                      | Connection               ; Section 14.10
                      | Date                     ; Section 14.18
                      | Pragma                   ; Section 14.32
                      | Trailer                  ; Section 14.40
                      | Transfer-Encoding        ; Section 14.41
                      | Upgrade                  ; Section 14.42
                      | Via                      ; Section 14.45
                      | Warning                  ; Section 14.46
   request-header (section 5.3), 
                      = Accept                   ; Section 14.1
                      | Accept-Charset           ; Section 14.2
                      | Accept-Encoding          ; Section 14.3
                      | Accept-Language          ; Section 14.4
                      | Authorization            ; Section 14.8
                      | Expect                   ; Section 14.20
                      | From                     ; Section 14.22
                      | Host                     ; Section 14.23
                      | If-Match                 ; Section 14.24
                      | If-Modified-Since        ; Section 14.25
                      | If-None-Match            ; Section 14.26
                      | If-Range                 ; Section 14.27
                      | If-Unmodified-Since      ; Section 14.28
                      | Max-Forwards             ; Section 14.31
                      | Proxy-Authorization      ; Section 14.34
                      | Range                    ; Section 14.35
                      | Referer                  ; Section 14.36
                      | TE                       ; Section 14.39
                      | User-Agent               ; Section 14.43
   response-header (section 6.2)
                       = Accept-Ranges           ; Section 14.5
                       | Age                     ; Section 14.6
                       | ETag                    ; Section 14.19
                       | Location                ; Section 14.30
                       | Proxy-Authenticate      ; Section 14.33
                       | Retry-After             ; Section 14.37
                       | Server                  ; Section 14.38
                       | Vary                    ; Section 14.44
                       | WWW-Authenticate        ; Section 14.47
   entity-header (section 7.1) fields
                      = Allow                    ; Section 14.7
                      | Content-Encoding         ; Section 14.11
                      | Content-Language         ; Section 14.12
                      | Content-Length           ; Section 14.13
                      | Content-Location         ; Section 14.14
                      | Content-MD5              ; Section 14.15
                      | Content-Range            ; Section 14.16
                      | Content-Type             ; Section 14.17
                      | Expires                  ; Section 14.21
                      | Last-Modified            ; Section 14.29
                      | extension-header = message-header

        Transfer-Encoding = 1#transfer-coding
        transfer-coding    = "chunked" ; Section 4.1
                        / "compress" ; Section 4.2.1
                        / "deflate" ; Section 4.2.2
                        / "gzip" ; Section 4.2.3
                        / transfer-extension
        transfer-extension = token *( OWS ";" OWS transfer-parameter )

    controls
   +-------------------+--------------------------+
   | Header Field Name | Defined in...            |
   +-------------------+--------------------------+
   | Cache-Control     | Section 5.2 of [RFC7234] |
   | Expect            | Section 5.1.1            |
   | Host              | Section 5.4 of [RFC7230] |
   | Max-Forwards      | Section 5.1.2            |
   | Pragma            | Section 5.4 of [RFC7234] |
   | Range             | Section 3.1 of [RFC7233] |
   | TE                | Section 4.3 of [RFC7230] |
   +-------------------+--------------------------+

    Registrations
   +-------------------+----------+----------+-----------------+
   | Header Field Name | Protocol | Status   | Reference       |
   +-------------------+----------+----------+-----------------+
   | Accept            | http     | standard | Section 5.3.2   |
   | Accept-Charset    | http     | standard | Section 5.3.3   |
   | Accept-Encoding   | http     | standard | Section 5.3.4   |
   | Accept-Language   | http     | standard | Section 5.3.5   |
   | Allow             | http     | standard | Section 7.4.1   |
   | Content-Encoding  | http     | standard | Section 3.1.2.2 |
   | Content-Language  | http     | standard | Section 3.1.3.2 |
   | Content-Location  | http     | standard | Section 3.1.4.2 |
   | Content-Type      | http     | standard | Section 3.1.1.5 |
   | Date              | http     | standard | Section 7.1.1.2 |
   | Expect            | http     | standard | Section 5.1.1   |
   | From              | http     | standard | Section 5.5.1   |
   | Location          | http     | standard | Section 7.1.2   |
   | Max-Forwards      | http     | standard | Section 5.1.2   |
   | MIME-Version      | http     | standard | Appendix A.1    |
   | Referer           | http     | standard | Section 5.5.2   |
   | Retry-After       | http     | standard | Section 7.1.3   |
   | Server            | http     | standard | Section 7.4.2   |
   | User-Agent        | http     | standard | Section 5.5.3   |
   | Vary              | http     | standard | Section 7.1.4   |
   +-------------------+----------+----------+-----------------+
*/
/**
   +---------+-------------------------------------------------+-------+
   | Method  | Description                                     | Sec.  |
   +---------+-------------------------------------------------+-------+
   | GET     | Transfer a current representation of the target | 4.3.1 |
   |         | resource.                                       |       |
   | HEAD    | Same as GET, but only transfer the status line  | 4.3.2 |
   |         | and header section.                             |       |
   | POST    | Perform resource-specific processing on the     | 4.3.3 |
   |         | request payload.                                |       |
   | PUT     | Replace all current representations of the      | 4.3.4 |
   |         | target resource with the request payload.       |       |
   | DELETE  | Remove all current representations of the       | 4.3.5 |
   |         | target resource.                                |       |
   | CONNECT | Establish a tunnel to the server identified by  | 4.3.6 |
   |         | the target resource.                            |       |
   | OPTIONS | Describe the communication options for the      | 4.3.7 |
   |         | target resource.                                |       |
   | TRACE   | Perform a message loop-back test along the path | 4.3.8 |
   |         | to the target resource.                         |       |
   +---------+-------------------------------------------------+-------+
*/
/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  XX(33, INVALID,     INVALID)      \

typedef enum http_method
  {
#define XX(num, name, string) MD_##name = num,
  HTTP_METHOD_MAP(XX)
#undef XX
} method_t;
/* http method */
// typedef enum 
// {
//     MD_GET,
//     MD_HEAD,
//     MD_POST,
//     MD_PUT,
//     MD_DELETE,
//     MD_CONNECT,
//     MD_OPTIONS,
//     MD_TRACE,
//     MD_INVALID,
// } method_t;

/* http version */
typedef struct
{
    uint16_t major;
    uint16_t minor;
} version_t;

/* URI-line */
// URI = URI = scheme:[//authority]path[?query][#fragment]
// authority   = [ userinfo "@" ] host [ ":" port ]

typedef struct 
{
    string_t    scheme;
    string_t    host;
    string_t    port;
    string_t    abs_path;
    string_t    ref_path;
    string_t    extension_MIME;
    string_t    query;
    int         num_dots;
    int         num_entries;
    int         state;
} uri_t;

/* http request header*/
typedef struct 
{
    /*general header*/
    string_t    cache_control;
    string_t    connection;
    string_t    date;
    string_t    pragma;//
    string_t    trailer;//
    string_t    transfer_encoding;
    string_t    upgrade;//
    string_t    via;//
    string_t    warning;//
    /*request header*/
    string_t    accept;
    string_t    accept_charset           ;
    string_t    accept_encoding          ;
    string_t    accept_language          ;
    string_t    authorization            ;
    string_t    expect                   ;
    string_t    from                     ;
    string_t    host                     ;
    string_t    ifmatch                  ;
    string_t    if_modified_since        ;
    string_t    if_none_match            ;
    string_t    if_range                 ;
    string_t    if_unmodified_since      ;
    string_t    max_forwards             ;
    string_t    proxy_authorization      ;
    string_t    range                    ;
    string_t    referer                  ;
    string_t    te                       ;
    string_t    user_agent               ;
    /*entity header*/
    string_t    allow;                 
    string_t    content_encoding;      
    string_t    content_language;      
    string_t    content_length;        
    string_t    content_location;      
    string_t    content_md5;           
    string_t    content_range;         
    string_t    content_type;          
    string_t    expires;               
    string_t    last_modified;
} request_headers_t;

/* http request header*/
typedef struct
{
    /*general header*/
    string_t    cache_control;
    string_t    connection;
    string_t    date;
    string_t    pragma;//
    string_t    trailer;//
    string_t    transfer_encoding;
    string_t    upgrade;//
    string_t    via;//
    string_t    warning;//
    /*response header*/
    string_t    accept_ranges           ;
    string_t    age                     ;
    string_t    etag                    ;
    string_t    location                ;
    string_t    proxy_authenticate      ;
    string_t    retry_after             ;
    string_t    server                  ;
    string_t    vary                    ;
    string_t    www_authenticate        ;
    /*entity header*/
    string_t    allow;                 
    string_t    content_encoding;      
    string_t    content_language;      
    string_t    content_length;        
    string_t    content_location;      
    string_t    content_md5;           
    string_t    content_range;         
    string_t    content_type;          
    string_t    expires;               
    string_t    last_modified;
} response_headers_t;

/* transfer encoding */
typedef enum
{
    TE_IDENTITY, // 
    TE_CHUNKED,
    TE_COMPRESS,
    TE_DEFLATE,
    TE_GZIP,
} transfer_encoding_t;

/* finity state machine/automaon */
typedef enum 
{
    //request line's
    S_RE_BEGIN,
    S_RE_METHOD,// 
    S_RE_SPACES_BEFORE_URI,
    // <-URI's 
    S_RE_CHECK_URI,

    S_RE_VERSION_SPACES_BEFORE,// XXX/[ ]HTTP
    S_RE_VERSION_H,
    S_RE_VERSION_HT,
    S_RE_VERSION_HTT,
    S_RE_VERSION_HTTP,
    S_RE_VERSION_SLASH, // HTTP/
    S_RE_VERSION_MAJOR, // HTTP/1
    S_RE_VERSION_DOT,   // HTTP/1.
    S_RE_VERSION_MINOR, // HTTP/1.X
    S_RE_VERSION_SPACES_AFTER,
    S_RE_ALMOST_DONE,
    S_RE_DONE,

    //header-field's
    S_HF_BEGIN,
    S_HF_SPEC,
    S_HF_NAME,// xD
    S_HF_COLON,// x:
    S_HF_SPACE_BEFORE_VALUE, // x:[ ]value
    S_HF_VALUE,// x: value
    S_HF_SPACE_AFTER_VALUE,// x: value[ ]
    S_HF_ALMOST_DONE,
    S_HF_DONE,

    //URI's 
    S_URI_BEGIN,
    S_URI_SCHEME,
    S_URI_SCHEME_COLON,
    S_URI_SCHEME_SLASH,
    S_URI_SCHEME_DSLASH,
    S_URI_HOST,
    S_URI_HOST_COLON,
    S_URI_HOST_PORT,
    S_URI_ABS_PATH_DOT,
    S_URI_ABS_PATH_DDOT,
    S_URI_ABS_PATH_SLASH,
    S_URI_ABS_PATH_ENTRY,
    S_URI_EXTENSION_MIME,
    S_URI_QUERY,
    S_URI_END,
} parse_state_t;

typedef struct request_header_parser
{
    /* request-line */
    method_t       method;
    version_t      version;
    string_t       url_str;
    uri_t          url;

    /* header-line  */
    //some special header-field
    bool                keep_alive;
    int                 content_length;
    int                 transfer_encoding;   

    request_headers_t   req_headers;
    int                 num_headers;

    string_t            header_key;//header-field k-v
    string_t            header_val;

    /* state */
    char        *next_pos;//store parse pos/ instead of offest+begin?
    int          state;//parse state

    char        *method_begin;
    char        *uri_begin;

    size_t       body_recv;
    int          buffer_sent;

    bool         isCRLF;//?
    bool         response_done;
    bool         req_error;
} header_parser_t;

extern void parse_archive_init(buffer_t *b, header_parser_t *ar);

extern int parse_request_line(buffer_t *b, header_parser_t *ar);
extern int parse_header_line(buffer_t *b, header_parser_t *ar);
extern int parse_header_body_identity(buffer_t *b, header_parser_t *ar);


#endif