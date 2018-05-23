#include "config.h"

#define CONF_ERROR(msg, p) fprintf(stderr, "ERROR: " msg " %s\n", (p));

static char *load_file(const char *filepath)
{
    struct stat st;
    if (stat(filepath, &st) == -1)
    {
        CONF_ERROR("can't find file", filepath);
        return NULL;
    }
    int fd = open(filepath, O_RDONLY);
    if (fd == -1)
    {
        CONF_ERROR("can't open file", filepath);
        return NULL;
    }
    char *text = (char *)malloc(st.st_size + 1); // this is not going to be freed
    if (st.st_size != read(fd, text, st.st_size))
    {
        CONF_ERROR("can't read file", filepath);
        close(fd);
        return NULL;
    }
    close(fd);
    text[st.st_size] = '\0';
    return text;
}


void config_init(config_t* server_config)
{
    server_config->port = 8000;
    server_config->timeout = 60,
    server_config->worker = 1,
    server_config->rootdir = "NULL",
    server_config->rootdir_fd = -1,
    server_config->debug = true,
    server_config->text = NULL;

}

int config_load(config_t* server_config)
{
    config_init(server_config);
    char conf_path[] = "./config.json";
    if(server_config->text != NULL)
        free(server_config->text);
    char* input = load_file(conf_path);
    server_config->text = input;

    ////
    myJson *json = json_parse(input);
    if(json == NULL)
    {
        CONF_ERROR("can't parse json", input);
        return ERROR;
    }
    const myJson *rootdir = json_get_object(json, "root");
    if(rootdir != NULL && rootdir->type == JSON_STRING)
    {
        server_config->rootdir = rootdir->value_text;
        printf("ROOT:%s\n", server_config->rootdir);
    }

    const myJson *port = json_get_object(json, "port");
    if(port != NULL && port->type == JSON_INTEGER)
        server_config->port = port->value_int;
    printf("PORT: %d\n", server_config->port);

    const myJson *timeout = json_get_object(json, "timeout");
    if(timeout != NULL && timeout->type == JSON_INTEGER)
        server_config->timeout = timeout->value_int;
    printf("TIMEOUT: %d\n", server_config->timeout);

    const myJson *worker = json_get_object(json, "worker");
    if(worker != NULL && worker->type == JSON_INTEGER)
        server_config->worker = worker->value_int;
    printf("WORKER: %d\n", server_config->worker);

    const myJson *debug = json_get_object(json, "debug");
    if(debug != NULL && debug->type == JSON_BOOLEAN)
        server_config->debug = debug->value_int;
    printf("DEBUG: %d\n", server_config->debug);

    json_free(json);
    return OK;
}

int config_parse(int argc, char *argv[], config_t *server_config)
{
    int ch;
    int flag = 0;
    while( ~(ch = getopt(argc, argv, "p:dt:w:r:s")) ) //EOF port-debug-timeout-worker-rtaddr
    {
        if(flag == 1)
            break;
        printf("%c\n", ch);
        switch(ch)
        {
            case 's':
                flag = 1;
                break;
            case 'p':
                server_config->port = atoi(optarg);
                break;
            case 'd':
                server_config->debug = true;
                break;
            case 't':
                server_config->timeout = atoi(optarg);
                break;
            case 'w':
                server_config->worker = atoi(optarg);
                if(server_config->worker > sysconf(_SC_NPROCESSORS_ONLN))
                {
                    fprintf(stderr, "ERROR: worker more than cpu cores available.\n");
                    return -1;
                }
                break;
            case 'r':
                server_config->rootdir = optarg;
                break;
            default: 
                return ERROR;
        }
    }
    int stat;
    if(flag == 1)
        stat = config_load(server_config);
    if(stat == ERROR)
        return ERROR;
    if(server_config->rootdir == NULL)
    {
        perror(server_config->rootdir);
        return ERROR;
    } 
    DIR *dirp = opendir(server_config->rootdir);
    if(dirp == NULL)//非空文件夹
    {
        perror(server_config->rootdir);
        return ERROR;
    }
    closedir(dirp);
    server_config->rootdir_fd = open(server_config->rootdir, O_RDONLY);
    //CUSTOM ERROR JUDGE 
    ERR_ON((server_config->rootdir_fd == ERROR), server_config->rootdir);
    return 0;
}