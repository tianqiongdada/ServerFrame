#pragma once

//epoll对象接口
struct IEpoller
{
	//@brief 添加监听句柄.
	//@param fd    句柄
	//@param data  辅助的数据, 可以后续在epoll_event中获取到
	//@param event 需要监听的事件EPOLLIN|EPOLLOUT
	virtual int add(int fd, void* pData, __uint32_t event) = 0;

	//@brief 修改句柄事件.
	//@param fd    句柄
	//@param data  辅助的数据, 可以后续在epoll_event中获取到
	//@param event 需要监听的事件EPOLLIN|EPOLLOUT
	virtual int mod(int fd, void* pData, __uint32_t event) = 0;

	//@brief 删除句柄事件.
	//@param fd    句柄
	//@param data  辅助的数据, 可以后续在epoll_event中获取到
	//@param event 需要监听的事件EPOLLIN|EPOLLOUT
	virtual int del(int fd, void* pData, __uint32_t event) = 0;
};
