    #include <stdio.h>     
    #include <sys/types.h>     
    #include <sys/socket.h>     
    #include <netinet/in.h>     
    #include <arpa/inet.h>   
    #include <stdlib.h>  
    #include <string.h>  
    #include <sys/epoll.h>  
     
    #define BUFFER_SIZE 40  
    #define MAX_EVENTS 10  
      
    int main(int argc, char * argv[])     
    {  
        int server_sockfd;// 服务器端套接字     
        int client_sockfd;// 客户端套接字     
        int len;     
        struct sockaddr_in my_addr;   // 服务器网络地址结构体     
        struct sockaddr_in remote_addr; // 客户端网络地址结构体     
        int sin_size;     
        char buf[BUFFER_SIZE];  // 数据传送的缓冲区     
        memset(&my_addr,0,sizeof(my_addr)); // 数据初始化--清零     
        my_addr.sin_family=AF_INET; // 设置为IP通信     
        my_addr.sin_addr.s_addr=INADDR_ANY;// 服务器IP地址--允许连接到所有本地地址上     
        my_addr.sin_port=htons(8000); // 服务器端口号     
        // 创建服务器端套接字--IPv4协议，面向连接通信，TCP协议  
        if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)     
        {       
            perror("socket");     
            return 1;     
        }     
        // 将套接字绑定到服务器的网络地址上  
        if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)     
        {     
            perror("bind");     
            return 1;     
        }     
        // 监听连接请求--监听队列长度为5   
        listen(server_sockfd,5);     
        sin_size=sizeof(struct sockaddr_in);   
        // 创建一个epoll句柄  
        int epoll_fd;  
        epoll_fd=epoll_create(MAX_EVENTS);  
        if(epoll_fd==-1)  
        {  
            perror("epoll_create failed");  
            exit(EXIT_FAILURE);  
        }  
        struct epoll_event ev;// epoll事件结构体  
        struct epoll_event events[MAX_EVENTS];// 事件监听队列  
        ev.events=EPOLLIN;  
        ev.data.fd=server_sockfd;  
        // 向epoll注册server_sockfd监听事件  
        if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_sockfd,&ev)==-1)  
        {  
            perror("epll_ctl:server_sockfd register failed");  
            exit(EXIT_FAILURE);  
        }  
        int nfds;// epoll监听事件发生的个数  
        // 循环接受客户端请求      
        while(1)  
        {  
            // 等待事件发生  
            nfds=epoll_wait(epoll_fd,events,MAX_EVENTS,-1);  
            if(nfds==-1)  
            {  
                perror("start epoll_wait failed");  
                exit(EXIT_FAILURE);  
            }  
            int i;  
            for(i=0;i<nfds;i++)  
            {  
                // 客户端有新的连接请求  
                if(events[i].data.fd==server_sockfd)  
                {  
                    // 等待客户端连接请求到达  
                    if((client_sockfd=accept(server_sockfd,(struct sockaddr *)&remote_addr,&sin_size))<0)  
                    {     
                        perror("accept client_sockfd failed");     
                        exit(EXIT_FAILURE);  
                    }  
                    // 向epoll注册client_sockfd监听事件  
                    ev.events=EPOLLIN;  
                    ev.data.fd=client_sockfd;  
                    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_sockfd,&ev)==-1)  
                    {  
                        perror("epoll_ctl:client_sockfd register failed");  
                        exit(EXIT_FAILURE);  
                    }  
                    printf("accept client %s/n",inet_ntoa(remote_addr.sin_addr));  
                }  
                // 客户端有数据发送过来  
                else  
                {  
                    len=recv(client_sockfd,buf,BUFFER_SIZE,0);  
                    if(len<0)  
                    {  
                        perror("receive from client failed");  
                        exit(EXIT_FAILURE);  
                    }  
                    printf("receive from client:%s",buf);  
                    send(client_sockfd,"I have received your message.",30,0);  
                }  
            }  
        }  
        return 0;     
    }    