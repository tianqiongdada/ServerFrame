#pragma once
#include "common.h"
#include "Interface/IEpoller.h"
using namespace std;

//@brief epoller操作类，已经默认采用了EPOLLET方式做触发(边缘触发方式)
class TC_Epoller : public IEpoller
{
public:
	//@brief 构造函数.
    //@param bEt 默认是ET模式，当状态发生变化的时候才获得通知
	TC_Epoller(bool bEt = true);
	~TC_Epoller();
	   
	//@brief 生成 epoll句柄.
	//@param max_connections epoll服务需要支持的最大连接数
	bool create(int max_connections);

	//@brief 等待时间.
	//@param millsecond 毫秒
	//@return int  有事件触发的句柄数
	int wait(int millsecond);

	//@brief 获取被触发的事件.
	//@return struct epoll_event&被触发的事件
	struct epoll_event& getEvent(int i);

	//IEpoller
public:
	//@brief 添加监听句柄.
	//@param fd    句柄
	//@param data  辅助的数据, 可以后续在epoll_event中获取到
	//@param event 需要监听的事件EPOLLIN|EPOLLOUT
	virtual int add(int fd, void* pData, __uint32_t event);

	//@brief 修改句柄事件.
	//@param fd    句柄
	//@param data  辅助的数据, 可以后续在epoll_event中获取到
	//@param event 需要监听的事件EPOLLIN|EPOLLOUT
	virtual int mod(int fd, void* pData, __uint32_t event);
	   
	//@brief 删除句柄事件.
	//@param fd    句柄
	//@param data  辅助的数据, 可以后续在epoll_event中获取到
	//@param event 需要监听的事件EPOLLIN|EPOLLOUT
	virtual int del(int fd, void* pData, __uint32_t event);

protected:
    //@brief 控制 epoll，将EPOLL设为边缘触发EPOLLET模式
    //@param fd    句柄，在create函数时被赋值
    //@param data  辅助的数据, 可以后续在epoll_event中获取到
    //@param event 需要监听的事件
    //@param op    EPOLL_CTL_ADD： 注册新的 fd到epfd 中
    //EPOLL_CTL_MOD：修改已经注册的 fd的监听事件
    //EPOLL_CTL_DEL：从 epfd中删除一个fd
	int ctrl(int fd, void* pData, __uint32_t events, int op);

private:
	int			m_nEpollfd;							//epoll
	int			m_nMaxConnections;					//最大链接数量
	struct		epoll_event *m_EventCollections;	//当前事件集合
	bool		m_bET;								//是否et模式
};