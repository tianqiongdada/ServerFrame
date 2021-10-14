#include "TC_Epoller.h"

TC_Epoller::TC_Epoller(bool bEt)
{
	m_nEpollfd   = -1;
	m_EventCollections       = NULL;
	m_bET         = bEt;
	m_nMaxConnections  = 1024;
}

TC_Epoller::~TC_Epoller()
{
	if (m_EventCollections  != NULL)
	{
		delete[] m_EventCollections;
		m_EventCollections  = NULL;
	}

	if (m_nEpollfd > 0)
	{
		close(m_nEpollfd);
	}
}

int TC_Epoller::ctrl(int fd, void* pData, __uint32_t events, int op)
{
	//typedef union epoll_data {
	//	void* ptr;
	//	int fd;
	//	__uint32_t u32;
	//	__uint64_t u64;
	//} epoll_data_t;

	//struct epoll_event {
	//	__uint32_t events; /* Epoll events */
	//	epoll_data_t data; /* User data variable */
	//};

	//data 变量是联合体，以传入的pData为主
	struct epoll_event ev;
	if (pData)
		ev.data.ptr = pData;
	else
		ev.data.fd = fd;

	if (m_bET)
	{
		ev.events   = events | EPOLLET;
	}
	else
	{
		ev.events   = events;
	}

	return epoll_ctl(m_nEpollfd, op, fd, &ev);
}

bool TC_Epoller::create(int max_connections)
{
	m_nMaxConnections  = max_connections;
	m_nEpollfd  = epoll_create(m_nMaxConnections  + 1);
	if (m_nEpollfd <= 0)
		return false;

	if (m_EventCollections != NULL)
	{
		delete[] m_EventCollections;
		m_EventCollections = NULL;
	}

	m_EventCollections  = new epoll_event[m_nMaxConnections  + 1];
	return true;
}

int TC_Epoller::add(int fd, void* pData, __uint32_t event)
{
	return ctrl(fd, pData, event, EPOLL_CTL_ADD);
}

int TC_Epoller::mod(int fd, void* pData, __uint32_t event)
{
	return ctrl(fd, pData, event, EPOLL_CTL_MOD);
}

int TC_Epoller::del(int fd, void* pData, __uint32_t event)
{
	return ctrl(fd, pData, event, EPOLL_CTL_DEL);
}

int TC_Epoller::wait(int millsecond)
{
	return epoll_wait(m_nEpollfd, m_EventCollections, m_nMaxConnections  + 1, millsecond);
}

struct epoll_event& TC_Epoller::getEvent(int i)
{
	return m_EventCollections[i];
}
