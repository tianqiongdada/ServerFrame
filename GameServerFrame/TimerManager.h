#pragma once
#include <map>
#include "common.h"
#include "commonCmd.h"
#include <sys/timerfd.h>
#include <future>
#include "Interface/IEpoller.h"
#include "Interface/IUserManager.h"
#include <uuid/uuid.h>;

enum TimeEnum
{
	Time_100_MS = 0, //100毫秒执行一次
	Time_1_Sec,		 //1秒
	Time_5_Sec,		 //5秒
	Time_30_Sec,	 //30秒
	Time_1_Min,		 //1分钟
};

struct stTimerParam:  public stEpollParamBase
{
	//int tfd;
	//TimingFunc func;
	//uint uTimes;
};

using TimingTask = std::function<void()>;

struct stTimingTaskInfo
{
	stTimingTaskInfo(TimingTask&& t, string&& strDes)
	{
		task = std::move(t);
		t = nullptr;
		strDescribe = move(strDes);
		strDescribe = "";
	}
	TimingTask task;
	string strDescribe;
};
extern std::map<TimeEnum, std::vector<stTimingTaskInfo>> g_mapTimingTasks;

//提交一个定时执行的任务
template<class T, class... Args>
void TimingTaskCommit(TimeEnum entime, string&& strDes, T&& t, Args&&... args)
{
	auto task = bind(forward<T>(t), forward<Args>(args)...); // 把函数入口及参数,打包(绑定)

	function<void()> func = [task] { task(); };

	if (g_mapTimingTasks.find(entime) == g_mapTimingTasks.end())
	{
		LOG_ERROR << "no such timetask object!";
		return;
		//g_mapTimingTasks[entime] = std::vector<stTimingTaskInfo>{};
	}

	//uuid_t uu;
	//uuid_generate(uu);
	stTimingTaskInfo info(std::move(func), std::move(strDes));
	g_mapTimingTasks[entime].emplace_back(std::move(info));
}

class TimerManager
{
public:
	TimerManager(IEpoller* pEpoller);
	virtual ~TimerManager();

private:
	using TimingFunc = std::function<void(int)>;
	std::map<int, TimingFunc>	m_mapTimer;
	IEpoller*					m_pEpoller;

private:
	//创建定时器对象
	int _CreateTimerObj(uint uiSec, uint uiNsec);


	//将定时器描述符添加到epoll
	int _AddTimerObjToEpoll(int tFd, stTimerParam* pParam);

	//定时执行的任务
private:
	static void Timer100mSec(int tFd);		//100毫秒秒执行的任务
	static void Timer1Sec(int tFd);			//1秒执行的任务
	static void Timer5Sec(int tFd);			//5秒执行的任务
	static void Timer30Sec(int tFd);		//30秒执行的任务
	static void Timer60Sec(int iFd);		//60秒执行的任务

public:
	//创建一个定时任务
	int CreateTimerTask(uint uiSec, uint uiNsec, TimingFunc func);

	//创建所有的定时任务
	bool CreateTimerTasks();
	bool IsTimerTask(int tFd);	//是否是定时器任务
	bool doTimerTask(int tFd);	//执行定时任务
	void CloseTimerObj(int tFd, stTimerParam* pParam);

};

