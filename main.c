#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h> // #include <cassert>
#include <sys/epoll.h>

#include "./lock/locker.h"
#include "./threadpool/threadpool.h"
#include "./timer/lst_timer.h"
#include "./http/http_conn.h"
#include "./log/log.h"
#include "./CGImysql/sql_connection_pool.h"

#define MAX_FD 65536 // 最大文件描述符
#define MAX_EVENT_NUMBER 10000 // 最大事件数
#define TIMESLOT 5 // 最小超时单位

#define SYNLOG // 同步写日志
// #define ASYNLOG // 异步写日志

// #define listenfdET // 边缘触发非阻塞
#define listenfdLT // 水平触发阻塞

// 这三个函数在http_conn.cpp 中定义，改变链接属性
extern int addfd(int epollfd, int fd, bool one_shot);
extern int remove(int epollfd, int fd);
extern int setnonblocking(int fd);

// 设置定时器相关属性
static int pipefd[2];
static sort_timer_lst timer_lst;
static int epollfd = 0;

// 信号处理函数
void sig_handler(int sig) {
    // 为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

// 设置信号函数
void addsig(int sig, void(handler)(int), bool restart = true) {

}

// 定时处理任务，重新定时以不断触发SIGALRM信号
void timer_handler() {

}

// 定时器回调函数，删除非活动连接在socket上的注册事件，并关闭
void cb_func(client_data *user_data) {

}

void show_error(int connfd, const char *info) {

}

int main(int argc, char *argv[]) {
#ifdef ASYNLOG
    Log::get_instance()->init("ServerLog", 2000, 800000, 8); // 异步日志模型
#endif

#ifdef SYNLOG
    Log::get_instance()->init("ServerLog", 2000, 800000, 0); // 同步日志模型
#endif

    if (argc <= 1) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    int port = atoi(argv[1]);

    addsig(SIGPIPE, SIG_IGN);

    // 创建数据库连接池
    connection_pool *connPool = connection_pool::GetInstance();
    connPool->init("localhost", "root", "root", 'yuancf1024db', 3306, 8);

    // 创建线程池
    threadpool<http_conn> *pool = NULL;
    try {
        pool = new threadpool<http_conn>(connPool);
    }
    catch (...) {
        return 1;
    }

    // 创建MAX_FD个http类对象
    http_conn *users = new http_conn[MAX_FD];

    // 创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    epollfd = epoll_create(5);
    assert(epollfd != -1);

    // 将listenfd放在epoll树上
    addfd(epollfd, listenfd, false);

    // 将上述epollfd赋值给http类对象的m_epollfd顺属性
    http_conn::m_epollfd = epollfd;

    while (!stop_server) {
        // 等待所监控文件描述符上有事件的产生
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            break;
        }
        // 对所有就绪事件进行处理
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;

            // 处理新到的客户连接
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
// LT水平触发
#ifdef LT
                int connfd =
                    accept(listenfd, (struct sockaddr *)&client_address,
                           &client_addrlength);
                if (connfd < 0) {
                    continue;
                }
                if (http_conn::m_user_count >= MAX_FD) {
                    show_error(connfd, "Internal server busy");
                    continue;
                }
                users[connfd].init(connfd, client_address);
#endif

// ET非阻塞边缘触发
#ifdef ET
                // 需要循环接收数据
                while (1) {
                    int connfd =
                        accept(listenfd, (struct sockaddr *)&client_address,
                            &client_addrlength);
                    if (connfd < 0) {
                        break;
                    }
                    if (http_conn::m_user_count >= MAX_FD) {
                        show_error(connfd, "Internal server busy");
                        break;
                    }
                    users[connfd].init(connfd, client_address);
                }
                continue;
#endif

            }
            // 处理异常事件
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 服务器端关闭连接
            }

            // 处理信号
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {

            }

            // 处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN) {
                // 读入对应缓冲区
                if (users[sockfd].read_once()) {
                    // 若监测到读事件，将该事件放入请求队列
                    pool->append(users + sockfd);
                }
                else {
                    // 服务器关闭
                }
            }
        }
    }
}