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

#define MAX_DATA_LEN		102400			//数据存储区域
#define MAX_USER_NUM		1024			//最大用户链接数

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


#define DESCRIBE_TIMER_USER_KEEP_LIFE  "User_Keep_Life"