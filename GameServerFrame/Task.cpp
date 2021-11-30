#include "Task.h"

CTask::CTask()
{

}

CTask::~CTask()
{

}

void CTask::TaskAccept(IUserManger* pUserManager, IEpoller* pEpoller, int nListenSocket)
{
	if (!pUserManager || !pEpoller || nListenSocket < 0)
	{
		printf(" CTask::TaskAccept params is incorrect!");
		return;
	}

	struct sockaddr_in clientaddr;
	socklen_t clilen = sizeof(struct sockaddr_in);

	int nAcceptfd = 0;
	while ((nAcceptfd = accept(nListenSocket, (struct sockaddr*) & clientaddr, &clilen)) > 0)
	{
		if (nAcceptfd < 0)
		{
			break;
		}

		LOG_INFO << "Acceptfd:" << nAcceptfd << ",IP:" << inet_ntoa(clientaddr.sin_addr) << ",Port:" << ntohs(clientaddr.sin_port);
		if (setNonBlocking(nAcceptfd) < 0)
		{
			continue;
		}

		int nUserIndex = -1;
		if (!pUserManager->addUser(nAcceptfd, nUserIndex))
		{
			close(nAcceptfd);
			continue;
		}

		if (pEpoller->add(nAcceptfd, pUserManager->GetUserData(nUserIndex)->pUserFlags, EPOLLIN) < 0)
			LOG_ERROR << "Epoll ctl add error!";
	}
}

void CTask::TaskRead(IUserManger* pUserManager, stUserFlags* pUserFlags)
{
	if (!pUserManager  || !pUserFlags)
	{
		LOG_ERROR << " CTask::TaskRead params is incorrect!";
		return;
	}

	int nSocketID = pUserFlags->nSocketID;
	int nUserIndex = pUserFlags->nUserIndex;
	if (nSocketID < 0 || (nUserIndex < 0 || nUserIndex >= MAX_USER_NUM))
		return;

	int nTotaleRead = 0;
	int nRead = 0;
	char szRecvBuf[MAX_RECVBUF_LEN] = {};

	//ET模式，循环读取数据直到读取完毕
	while ((nRead = read(nSocketID, szRecvBuf + nRead, MAX_RECVBUF_LEN - 1)) > 0)
	{
		nTotaleRead += nRead;
	}

	if (nRead < 0)
	{
		//if (errno == EINTR)
		//{
		//	nRead = 0;
		//}
		if (EAGAIN == errno) //如果内核缓冲区空了，继续往下面走;
		{
			//LOG_INFO << "read error , no more data! len:" << nTotaleRead;
		}
		else //链接出错了，关闭链接
		{
			pUserManager->CloseConnect(nUserIndex);
			perror("read error , close socket . ");
			return;
		}
	}

	//对端关闭了链接
	if (0 == nRead)
	{
		LOG_INFO << "client had closed!, SockedID:" << nSocketID;
		pUserManager->CloseConnect(nUserIndex);
		return;
	}

	//读取数据完成，处理数据
	pUserManager->RecvData(nUserIndex, szRecvBuf, nTotaleRead);
}

void CTask::TaskWrite(IUserManger* pUserManager, IEpoller* pEpoller, stUserFlags* pUserFlags)
{
	if (!pUserManager || !pEpoller || !pUserFlags)
	{
		LOG_ERROR << "params is incorrect!";
		return;
	}

	int nSocketID = pUserFlags->nSocketID;
	int nUserIndex = pUserFlags->nUserIndex;

	int nRet = pUserManager->SendData(pUserFlags);
	if (nRet < 0 && EAGAIN == errno)
	{
		//继续输出事件
		pEpoller->mod(nSocketID, pUserFlags, EPOLLOUT);
	}
	else
	{
		if (nRet != 0)
		{
			//更换为输入事件
			pEpoller->mod(nSocketID, pUserFlags, EPOLLIN);
		}
	}
}

void CTask::UserKeepLife(IUserManger* pUserManager)
{
	pUserManager->BroadcastCmd(CMD_PING, 0, vector<uint>{}, "this is a test:测试!", strlen("this is a test:测试!"));
}
