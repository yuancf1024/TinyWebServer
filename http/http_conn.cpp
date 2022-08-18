#include "http_conn.h"
#include "../log/log.h"
#include <map>
#include <mysql/mysql.h>
#include <fstream>

// #define connfdET // 边缘触发非阻塞
#define connfdLT // 水平触发阻塞

// #define listenfdET // 边缘触发非阻塞
#define listenfdLT // 水平触发阻塞

// 定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form =
    "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form =
    "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form =
    "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form =
    "There was an unusual problem serving the request file.\n";


// 当浏览器出现连接重置时，可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
const char *doc_root = "/home/chenfengyuan/Linux/TinyWebServer/root";

// 将表中的用户名和密码放入map
map<string, string> users;
locker m_lock;

void http_conn::initmysql_result(connection_pool *connPool) {
    // 先从连接池中取出一个连接
    MYSQL *mysql = NULL;
    connectionRAII mysqlcon(&mysql, connPool);

    // 在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
        LOG_ERROR("SELECT error: %s\n", mysql_error(mysql));
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    // 返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    // 返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    // 从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

// 对文件描述符设置非阻塞
int setnonblocking(int fd) { 
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int fd, bool one_shot) { 
    epoll_event event;
    event.data.fd = fd;

#ifdef connfdET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef connfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

#ifdef listenfdET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef listenfdLT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 从内核事件表删除描述符
void removefd(int epollfd, int fd) { 
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// 将事件重置为EPOLLONESHOT
void modfd(int epollfd, int fd, int ev) { 
    epoll_event event;
    event.data.fd = fd;

#ifdef connfdET
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
#endif

#ifdef connfdLT
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
#endif

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

// 关闭连接，关闭一个连接，客户总量减一
void http_conn::close_conn(bool real_close) {
    if (real_close && (m_sockfd != -1)) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

// 初始化连接，外部调用初始化套接字
void http_conn::init(int sockfd, const sockaddr_in &addr) {

}


// 初始化新接受的连接
// check_satate默认为分析请求行状态
void http_conn::init() {

}

// 从状态机，用于分析出一行内容
// 返回值为行的读取状态，有LINE_OK, LINE_BAD, LINE_OPEN
http_conn::LINE_STATUS http_conn::parse_line() {

}


// 循环读取客户数据，直到无数据可读或对方关闭连接
// 非阻塞ET工作模式下，需要一次性将数据读完
bool http_conn::read_once() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;
    while (true) {
        // 从套接字接收数据，存储在m_read_buf缓冲区
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx,
                          READ_BUFFER_SIZE - m_read_idx, 0);
        if (bytes_read == -1) {
            // 非阻塞ET模式下，需要一次性将数据读完
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        } else if (bytes_read == 0) {
            return false;
        }
        // 修改m_read_idx的读取字节数
        m_read_idx += bytes_read;
    }
    return true;
}

// 解析http请求行，获得请求方法，目标url及http版本号
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {

}

// 解析http请求的一个头部信息
http_conn::HTTP_CODE http_conn::parse_headers(char *text) {

}

// 判断http请求是否被完整读入
http_conn::HTTP_CODE http_conn::parse_content(char *text) {

}

// 解析报文整体流程
http_conn::HTTP_CODE http_conn::process_read() {
    // 初始化从状态机状态、HTTP请求解析结果
    
}

// 发起请求
http_conn::HTTP_CODE http_conn::do_request() {

}

void http_conn::unmap() {

}

bool http_conn::write() {

}

// 添加响应
bool http_conn::add_response(const char *format, ...) {

}

// 添加状态行
bool http_conn::add_status_line(int status, const char *title) {

}

// 添加响应头部
bool http_conn::add_headers(int content_len) {

}

// 添加响应正文长度
bool http_conn::add_content_length(int content_len) {

}

// 添加响应正文类型
bool http_conn::add_content_type() {

}

// 添加linger
bool http_conn::add_linger() {

}

// 添加空行
bool http_conn::add_blank_line() {

}

// 添加响应正文

bool http_conn::add_content(const char *content) {

}

bool http_conn::process_write(HTTP_CODE ret) {

}

void http_conn::process() { 
    HTTP_CODE read_ret = process_read(); 

    //NO_REQUEST，表示请求不完整，需要继续接收请求数据
    if (read_ret == NO_REQUEST) {
        // 注册并监听读事件
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    // 调用process_write完成报文响应
    bool write_ret = process_write(read_ret);
    if (!write_ret) {
        close_conn();
    }
    // 注册并监听写事件
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}
/*
各子线程通过process函数对任务进行处理，调用process_read函数
和process_write函数分别完成报文解析与报文响应两个任务。
*/