#pragma once
#include "WebSocket/WebSocketHelper.h"
#include "Interface/IEpoller.h"
#include "common.h"
#include "CmdDeal.h"
#include "Interface/IUserManager.h"
#include "CUser.h"

struct stUserFlags : public stEpollParamBase    //用户标志，SocketID 和用户数组下标，epoll事件传输时获取用户
{
	int nSocketID;		//socketID
	int nUserIndex;		//数组下标

	stUserFlags()
	{
		nSocketID = 0;
		nUserIndex = 0;
	}
};

struct stUserData
{
	WebSocketHelper webSocketHelper;			//webSocket帮助类
	char cRcvData[MAX_DATA_LEN];				//存储的数据
	uint nRcvDataLen;							//接收数据的长度
	char cSendData[MAX_SENDBUF_LEN];			//存储将要发送的数据
	uint nSendDataLen;							//发送数据的长度
	bool bHaveData;								//数据标志
	stUserFlags* pUserFlags;					//玩家标志
	CUser* pUser;

	stUserData()
	{
		Clear();
		init();
	}

	~stUserData()
	{
		Clear();
	}

	void init()
	{
		if (!pUserFlags)
			pUserFlags = new stUserFlags();
		
		if (!pUser)
			pUser = new CUser(pUserFlags);
	}

	void Clear()
	{
		if (pUserFlags)
		{
			delete pUserFlags;
			pUserFlags = nullptr;
		}
		if (pUser)
		{
			delete pUser;
			pUser = nullptr;
		}

		webSocketHelper.Clear();
		memset(cRcvData, 0, MAX_DATA_LEN);
		memset(cSendData, 0, MAX_SENDBUF_LEN);
		nRcvDataLen = 0;
		nSendDataLen = 0;
		bHaveData = false;
	}
};

class CUserManager : public IUserManger
{
public:
	//@brief 构造函数.
	//@param tcp_epoller epoll对象，操作数据I/O
	CUserManager(IEpoller* tcp_epoller);
	~CUserManager();

public:
	//@brief 获取发送数据
	//@param nIndex 用户索引
	//@param cSendData 需要的发送数据
	//@param nSendDataLen 发送数据的长度
	void GetSendData(int nIndex, char* cSrcData, uint cSrcDataLen, char* cSendData, ushort &nSendDataLen);

	//@brief 重置接收区数据
	//@param nIndex 用户索引
	//@param cNewData 新的数据
	//@param nNewDataLen 新的数据长度
	bool RestRcvData(int nIndex, char* cNewData = NULL, uint nNewDataLen = 0);

	//@brief 重置发送区数据
	//@param nIndex 用户索引
	//@param cNewData 新的数据
	//@param nNewDataLen 新的数据长度
	bool RestSendData(int nIndex, const char* cNewData = NULL, uint nNewDataLen = 0);

	//@brief 是否是websocket IO事件
	//@param nIndex 用户索引
	bool IsWebSocketTransData(int nIndex);

	//IUserManager
public:
	//@brief 接收数据.
	//@param nIndex 数组下标
	//@param cRcvData  接收的数据;
	//@param nDataLen  数据长度
	virtual bool RecvData(int nIndex, char* cRcvData, uint nDataLen);

	//@brief 获取用户
	//@param nIndex 用户索引
	virtual stUserData* GetUserData(int nIndex); //获取用户数据

	//@brief 发送数据
	//@param pUserFlags 用户标志
	virtual int SendData(stUserFlags* pUserFlags);

	//@brief 发送数据
	//@param pUserFlags 用户标志
	//@param cSendData 将要发送的数据
	//@param nSendDataLen 发送的数据长度
	virtual	int SendData(stUserFlags* pUserFlags, const char* cSendData, uint nSendDataLen);

	//@brief 添加用户
	//@param nSocketID 链接的客户端ID
	virtual bool addUser(int nSocketID, int& nIndex);

	//@brief 关闭用户连接
	//@param nIndex 用户索引
	virtual bool CloseConnect(int nIndex);

	//@brief 发送数据
	//@param pUserFlags 用户标志
	//@param uMainCmd 主命令码
	//@param uSubCmd 子命令码
	//@param cSendData 将要发送的数据
	//@param nDataLen 数据长度
	virtual int SendCmd(stUserFlags* pUserFlags, uint uMainCmd, uint uSubCmd, const char* cSendData, uint nDataLen);

	////@brief 发送数据
	////@param pUserFlags 用户标志
	////@param uMainCmd 主命令码
	////@param uSubCmd 子命令码
	////@param buf 将要发送的数据
	////@param nLen 数据长度
	//virtual int SendCmd(stUserFlags* pUserFlags, uint uMainCmd, uint uSubCmd, const void* buf, uint nLen);

	//@brief 广播数据
	//@param uMainCmd 主命令码
	//@param uSubCmd 子命令码
	//@param cSendData 将要发送的数据
	virtual int BroadcastCmd(uint uMainCmd, uint uSubCmd, const vector<uint>&& vcExceptUserID, const char* cSendData, uint nDataLen);

private:
	//@brief 解包
	//@param nIndex 用户索引
	void _UnpackData(int nIndex);

private:
	stUserData m_UserDataArr[MAX_USER_NUM];		//用户数组
	IEpoller *  m_pEpoller;				//epoll对象
	CCmdDeal m_CmdDeal;
};


