#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

// 线程池类定义
template <typename T>
class threadpool {
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool,
               int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request);
    // bool append_p(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg); // 线程处理函数和运行函数设置为私有属性
    void run();

private:
    int m_thread_number; // 线程池中的线程数
    int m_max_requests; // 请求队列中允许的最大请求数
    pthread_t *m_threads; // 描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; // 请求队列
    locker m_queuelocker; // 保护请求队列的互斥锁
    sem m_queuestat; // 是否有任务需要处理
    bool m_stop; // 是否结束线程
    connection_pool *m_connPool; // 数据库

    // int m_actor_model; // 模型切换
};

// 线程池创建与回收
template <typename T>
threadpool<T>::threadpool(connection_pool *connPool, int thread_number, int max_requests) : m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_connPool(connPool) {
    if (thread_number <= 0 || max_requests <= 0) {
        throw std::exception();
    }
    // 线程id初始化
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }
    for (int i = 0; i < thread_number; ++i) {
        printf("create the %dth thread\n",i);
        // 循环创建线程，并将工作线程按要求运行
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw ::exception();
        }

        // 将线程分离后，不用单独对工作线程进行回收
        if (pthread_detach(m_threads[i])) { // 分离线程
            delete[] m_threads;
            throw std::exception();
        }
    }
}
/*
构造函数中创建线程池,pthread_create函数中将类的对象作为参数传递给
静态函数(worker),在静态函数中引用这个对象,并调用其动态方法(run)。

具体的，类对象传递时用this指针，传递给静态函数后，
将其转换为线程池类，并调用私有成员函数run。
*/

template <typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

// 向请求队列中添加任务
template <typename T>
bool threadpool<T>::append(T *request) {
    m_queuelocker.lock();

    // 根据硬件，预先设置请求队列的最大值
    if (m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }

    // request->m_state = state;

    // 添加任务
    m_workqueue.push_back(request);
    m_queuelocker.unlock();

    // 信号量提醒有任务要处理
    m_queuestat.post();
    return true;
}
/*
通过list容器创建请求队列，向队列中添加时，通过互斥锁保证线程安全，
添加完成后通过信号量提醒有任务要处理，最后注意线程同步。
*/

// template <typename T>
// bool threadpool<T>::append_p(T *request) {

// }

// 线程处理函数
template <typename T>
void *threadpool<T>::worker(void *arg) {

    // 将参数强转为线程池类，调用成员方法
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
/*
内部访问私有成员函数run，完成线程处理要求
*/

// run执行任务
template <typename T>
void threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait(); //信号量等待
        m_queuelocker.lock(); //被唤醒后先加互斥锁
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }

        //从请求队列中取出第一个任务
        //将任务从请求队列删除
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        //从连接池中取出一个数据库连接
        connectionRAII mysqlcon(&request->mysql, m_connPool);

        //process(模板类中的方法,这里是http类)进行处理
        request->process();
    }
}
/*
主要实现，工作线程从请求队列中取出某个任务进行处理，注意线程同步。
*/
#endif