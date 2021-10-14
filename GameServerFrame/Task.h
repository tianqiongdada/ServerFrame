#pragma once
#include "UserData.h"
#include "common.h"
#include "Interface/IUserManager.h"
#include "TimerManager.h"

class CTask
{
public:
	CTask();
	virtual ~CTask();

	//IO事件
public:
	//玩家请求任务
	static void TaskAccept(IUserManger* pUserManager, IEpoller* pEpoller, int nListenSocket);
	//玩家上传数据读取任务
	static void TaskRead(IUserManger* pUserManager, stUserFlags* pUserFlags);
	//玩家下发数据任务
	static void TaskWrite(IUserManger* pUserManager, IEpoller* pEpoller, stUserFlags* pUserFlags);

	//UserTask
public:
	
	//定时任务
public:
	static void UserKeepLife(IUserManger* pUserManager);


};

