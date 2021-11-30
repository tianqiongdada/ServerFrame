#pragma once
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>   
#include <sys/epoll.h>   
#include <sys/types.h>
#include <arpa/inet.h>   
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include "base/Logging.h"
#include "ThreadPool.h"
#include "Interface/IEpoller.h"
#include "Interface/IUserManager.h"
#include <ifaddrs.h>

#define MAX_DATA_LEN		102400			//数据存储区域
#define MAX_USER_NUM		1024			//最大用户链接数
static int get_ip_linux(int ipv4_6, std::vector<std::string>& out_list_ip)
{
	int ret_val = 0;

	struct ifaddrs* ifAddrStruct = NULL;
	void* tmpAddrPtr = NULL;

	// 1.
	ret_val = getifaddrs(&ifAddrStruct);
	if (0 != ret_val)
	{
		ret_val = errno;

		return ret_val;
	}

	// 2.
	std::string str_ipvX;

	int padress_buf_len = 0;
	char addressBuffer[INET6_ADDRSTRLEN] = { 0 };

	if (AF_INET6 == ipv4_6)
		padress_buf_len = INET6_ADDRSTRLEN;
	else
		padress_buf_len = INET_ADDRSTRLEN;


	while (NULL != ifAddrStruct)
	{
		if (ipv4_6 == ifAddrStruct->ifa_addr->sa_family)
		{
			// is a valid IP4 Address
			tmpAddrPtr = &((struct sockaddr_in*)ifAddrStruct->ifa_addr)->sin_addr;

			inet_ntop(ipv4_6, tmpAddrPtr, addressBuffer, padress_buf_len);
			str_ipvX = std::string(addressBuffer);

			out_list_ip.push_back(str_ipvX);

			memset(addressBuffer, 0, padress_buf_len);
		}

		ifAddrStruct = ifAddrStruct->ifa_next;
	}

	return ret_val;
}

static int setNonBlocking(int p_nSock)
{
	int nOpts;
	nOpts = fcntl(p_nSock, F_GETFL);
	if (nOpts < 0)
	{
		printf("[%s %d] Fcntl Sock GETFL fail!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	nOpts = nOpts | O_NONBLOCK;
	if (fcntl(p_nSock, F_SETFL, nOpts) < 0)
	{
		printf("[%s %d] Fcntl Sock SETFL fail!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

enum enEpollType
{
	TYPE_SOCKET,
	TYPE_TIMER,
	TYPE_FILL
};

struct stEpollParamBase
{
	enEpollType enType;
	int nFd;

	stEpollParamBase()
	{
		enType = TYPE_SOCKET;
		nFd = -1;
	}

	void setParams(enEpollType type, int fd)
	{
		enType = type;
		nFd = fd;
	}
};

struct stGlobalMember
{
	whx::CThreadPool* pThreadPool;
	IEpoller* pEpoll;
	IUserManger* pUserMgr;
};
extern stGlobalMember g_Member;


#define DESCRIBE_TIMER_USER_KEEP_LIFE  "User_Keep_Life"