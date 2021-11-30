#include "common.h"
#include "Task.h"
#include "ThreadPool.h"
#include "TC_Epoller.h"
#include "UserData.h"
#include "TimerManager.h"
#include "mysql/MysqlManager.h"

//#include <pthread.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"
#endif
#endif

//#define SERV_IP			"192.168.20.128"	
#define SERV_IP				"172.17.0.12"	
#define SERV_PORT           (12345)				//服务端端口
#define MAX_LISTENQ         (32)				//服务端监听数量
#define MAX_EVENT           (65535)				//最大套接字数目

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		LOG_INFO << "未设置端口号";
		return 0;
	}

	whx::CThreadPool	myThreadPool{ 4 };										 //根据2n+2创建线程数量， n是内核数
	TC_Epoller			tc_eppller;												 //epoll 对象
	CUserManager*		pUserManger = new CUserManager((IEpoller*)&tc_eppller);  //用户管理
	TimerManager		timeManager((IEpoller*)&tc_eppller);					 //定时器管理

	g_Member.pThreadPool = &myThreadPool;
	g_Member.pUserMgr = (IUserManger*)pUserManger;
	g_Member.pEpoll = &tc_eppller;

	int nEventIndex = 0;	//事件索引
	int nRet = -1;			//socket 操作返回值	
	int nSockfd = 0;		//当前操作的socketID
	int nUserIndex = -1;	//当前玩家下标
	int nAcceptfd = 0;		//客户端的接入ID
	int nEventNum;			//当前产生的事件数量
	int nListenfd;			//服务端监听ID

	struct sockaddr_in serveraddr;
	socklen_t clilen = sizeof(struct sockaddr_in);
	struct stUserFlags* userFlags = NULL;

	nListenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (nListenfd < 0)
	{
		printf("[%s %d] Socket Create fail return:%d!\n", __FUNCTION__, __LINE__, nListenfd);
		return 0;
	}

	if (setNonBlocking(nListenfd) < 0)
	{
		return 0;
	}

	//下面是服务端IP地址的绑定和监听
	int nIP_TYPE = AF_INET;

	vector<string> vcIpList;
	get_ip_linux(nIP_TYPE, vcIpList);
	string strIP;
	for (auto ip : vcIpList)
	{
		if ( ip != "127.0.0.1")
		{
			strIP = ip;
			break;
		}
	}

	memset(&serveraddr, 0x00, sizeof(serveraddr));
	serveraddr.sin_family = nIP_TYPE;
	serveraddr.sin_addr.s_addr = inet_addr(strIP.c_str());
	int nPort = atoi(argv[1]);
	serveraddr.sin_port = htons(nPort);
	LOG_INFO << "绑定IP:" << strIP << ",端口号:" << argv[1];

	if (bind(nListenfd, (struct sockaddr*) & serveraddr, sizeof(serveraddr)) < 0)
	{
		LOG_ERROR << "Bind fd fail!";
		return 0;
	}

	if (listen(nListenfd, MAX_LISTENQ) < 0)
	{
		LOG_ERROR << "Listen fd fail!";
		return 0;
	}

	//epoll创建
	if (!tc_eppller.create(MAX_EVENT))
	{
		LOG_ERROR << "Epoll Create fail!";
		return 0;
	}

	stEpollParamBase* pData = new stEpollParamBase();
	pData->nFd = nListenfd;
	pData->enType = TYPE_SOCKET;
	if (tc_eppller.add(nListenfd, pData, EPOLLIN) < 0)
	{
		LOG_ERROR << "nListenfd Epoll add error!";
		return 0;
	}

	if (!timeManager.CreateTimerTasks()) //创建定时任务
	{
		LOG_ERROR << "定时器创建失败!";
		return 0;
	}

	if (!g_MysqlManager.Init())
	{
		LOG_ERROR << "mysql data init err!";
		return 0;
	}

	//心跳链接
	TimingTaskCommit(Time_5_Sec, "UserKeepLife", CTask::UserKeepLife, (IUserManger*)pUserManger);

	for (;;)
	{
		nEventNum = tc_eppller.wait(-1); //等待socket事件产生
		for (nEventIndex = 0; nEventIndex < nEventNum; nEventIndex++)
		{
			uint32_t u32_events = tc_eppller.getEvent(nEventIndex).events;
			stEpollParamBase* pData = (stEpollParamBase*)tc_eppller.getEvent(nEventIndex).data.ptr;
			if (!pData)
			{
				LOG_ERROR << "tc_eppller.getEvent(nEventIndex).data.ptr err";
				continue;
			}

			enEpollType type = pData->enType;
			int nCurFd = pData->nFd;

			if ((u32_events & EPOLLERR) || (u32_events & EPOLLHUP))
			{
				LOG_ERROR << "Epoll Event Error!";

				switch (type)
				{
				case TYPE_SOCKET:
					{
						userFlags = (struct stUserFlags*) tc_eppller.getEvent(nEventIndex).data.ptr;
						if (!userFlags)
						{
							continue;
						}

						nUserIndex = userFlags->nUserIndex;
						pUserManger->CloseConnect(nUserIndex); //关闭客户端链接
					}
					break;
				case TYPE_TIMER:
					timeManager.CloseTimerObj(nCurFd, (stTimerParam*)tc_eppller.getEvent(nEventIndex).data.ptr);
					break;
				case TYPE_FILL:
					break;
				default:
					break;
				}

				continue;
			}

			if (TYPE_TIMER == type)
			{
				timeManager.doTimerTask(nCurFd);
				continue;
			}
	
			if (TYPE_SOCKET == type)
			{
				if (nListenfd == nCurFd)   //客户端接入请求
				{
					myThreadPool.TaskCommit(CTask::TaskAccept, (IUserManger*)pUserManger, (IEpoller*)&tc_eppller, nListenfd);
					continue;
				}

				userFlags = (struct stUserFlags*) tc_eppller.getEvent(nEventIndex).data.ptr;
				if (!userFlags)
					continue;

				nUserIndex = userFlags->nUserIndex;
				nSockfd = userFlags->nSocketID;
				if (nSockfd < 0)
					continue;

				if (u32_events & EPOLLIN)  //数据输入
				{
					myThreadPool.TaskCommit(CTask::TaskRead, (IUserManger*)pUserManger, userFlags);
					continue;
				}
				else if (u32_events & EPOLLOUT)  //数据输出
				{
					myThreadPool.TaskCommit(CTask::TaskWrite, (IUserManger*)pUserManger, (IEpoller*)&tc_eppller, userFlags);
					continue;
				}
			}
			
		}
	}

	close(nListenfd);
}
