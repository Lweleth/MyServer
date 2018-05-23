# MyServer
# 特性
* 基于事件驱动的非阻塞式读写
* 状态转移解析request header
* 使用优先队列对keep-alive的连接进行维护
* 部分MIME资源映射
* 简单的状态响应
* 实现JSON解析，读取配置

## 遗留问题
1. 中文路径会转换成%XX（需要UNICODE转码UTF-8）

# 设计

## buffer & string
1. 自己实现一个简单的String结构，注意其比较和初始化。使用sizeof，会在编译期就确定一个字符串常量的准确大小，但是如果以字符指针接受，就无法使用sizeof了，但使用strlen会造成'\0'字符遗漏，不能准确确定大小。故强制要求比较函数的两个参数都是已经初始化的*string_t*对象(已经过sizeof确定大小)。另有小细节"\012\xef"是两个字符+一个'\0'd。类似地实现一个Buffer类，方便从中读取或写入数据。recv/send 注意 -1时errno 返回EAGAIN的处理（此时报文还未发送完毕，因此返回AGAIN）

## 内存池 
1. 暂时不考虑设计一个优秀的内存池进行内存管理，这对效率提升可能不是很大，未来为了学习可能会增加内存池的实现。

## Connection 
1. 由于使用长连接，需要维护一个有关连接时间的存储*Connection*的数据结构。根据需要保证及时移除超时连接的特性，那么考虑一次event刷新*active_time*的值，并且维护一个优先队列（堆）,每次O(logn)从队首移除Connction就行了。
2. 因此数据结构需要有indice，active_time, 对端的文件描述符， 存储请求的结构。其余为一些为了方便获取的信息。

## HTTP解析
1. 基于要实现多路复用的非阻塞I/O的特性的理由，需要解析不完整的请求头，故使用有限状态自动机（FSM），其HTTP状态数较少预先在头文件定义出各种状态，以便O（N）进行状态的转移

2. 实现Parser类，主要用于解析请求头，主要调用**parse_request_line**,其中转移状态到**parse_uri**和**header_line**，中间为了调试设置了一些条件编译的输出。设计的自动机的状态转移的结构不是很清晰，导致有些存储uri的某些信息的起始位置和长度可能会未被定义。因此这部分的状态自动机还需要对URI的标准进行细读以及进行路径测试。

# 一些错误记录
1. accept() 接受一个客户端连接时失败 errno 显示的消息是BAD ADDRESS，注意是否确保了sockaddr_in的长度是作为指针传入的

2. errno 11 暂不可用的资源，查资料这种情况出现在非阻塞方式的连接中...阻塞方式只需要确认一次是否返回正确的fd即可，而在非阻塞方式中需要循环确认返回值，当然我是从很蠢的问题发现的...发现自己循环没有写跳出...

3. Address already in use。编写过程中发现在关闭连接后再次启动程序会出现这个错误。实际上是TCP协议的要求：当一端关闭连接后会进入TIME_WAIT状态，保证对端的ACK信息失败能够重发。此时通过命令ls -i:port可以看到socket还在活动。setsockopt设置地址复用，多进程下还要重用端口。

4. nodelay  禁用nagle算法 man 7 tcp

5. 语法问题。在使用offsetof得到头部字段在其结构体内的偏移值后，需要注意其结构体起始位置的指针的地址需要转换成整形，否则直接位移会使地址加上对应类型size*offset的距离

6. 页面对于一个能够成功访问的请求，如果有Keep-alive选项，在对端断开后重连会进行连接复用，在这里要**注意清空buffer**重置上一次连接遗留的信息...否则就会造成无论访问什么页面响应的都是第一次请求的页面，直至缓冲区用完。

7. SIGPIPE BROKEN 这个问题在以前学习tinyhttpd的源码时就遇到过，管道破裂在对端关闭连接时，本端还在尝试写入数据会收到RST响应，再写就会收到SIGPIPE的信号。通常设置忽略该信号，尝试写入就会返回失败。

# 小技巧
1. do{}while(0) 可以起到{}的包容作用,因此可在在其中写入break代替goto.
2. 宏定义。避免冗余。

# 经验（keng）总结
1. 先进行结构设计，以及进行需求分析，确定要实现的功能范围。最好设计易于扩展的结构。
2. 从顶端逐渐往底层实现，逐渐丰富功能，应用->系统->协议->解析。这样每一步可以进行单个模块的测试，易于发现问题。我先进行了状态机的设计和实现再写的请求处理模块，但最初由于对非阻塞socket的不了解导致最后的状态跳出转移有很多问题，花费了很多时间进行逐步调试判断问题。
3. 遇到奇怪的问题先man，后搜索引擎
4. 要善用GDB（项目后期之前一直在用printf和assert调试...）
5. 基础回顾。链接顺序？宏定义？条件编译？指针偏移？

