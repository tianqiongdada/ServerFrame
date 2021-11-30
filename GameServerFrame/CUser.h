#pragma once
#include "mysql/MysqlCommon.h"
enum emUserStates
{
	STATU_NOT_LOGIN = 0, //未登录
	STATU_LOGINING, //正在登陆
	STATU_LOGIN, //已登录
	STATU_PLAYING, //游戏中
	STATU_OFFLINE, //断线
};

struct stUserStatusInfo
{
	unsigned char ucUserStatus;
	uint uUserID;

	stUserStatusInfo()
	{
		ucUserStatus = 0;
		uUserID = 0;
	}
};


#define USER_NAME_LEN 32

struct stUserFlags;
class CUser : public IAsyCallBack
{
public:
	CUser(stUserFlags* pUserFlags);
	virtual ~CUser();

public:
	string m_strName;		//用户名
	uint m_utID;			//用户ID
	emUserStates m_state;	//用户状态
	time_t m_tLoginTime;	//用户登陆时间
	time_t m_tLastPing;		//用户最后一次心跳回应
	stUserFlags* m_pUserFlags;

public:
	time_t GetLastPingTime() { return m_tLastPing; }
	void UpdateLastPingTime(time_t t) { m_tLastPing = t; }

	//IAsyCallBack
public:
	virtual bool DBCallBack(vector<vector<Field>>&& vcResults, int nDbCallBackID);

};

