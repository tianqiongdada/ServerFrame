#pragma once
struct stUserFlags;
struct stUserData;

struct IUserManger
{
	virtual bool RecvData(int nIndex, char* cRcvData, uint nDataLen) = 0;
	virtual stUserData* GetUserData(int nIndex) = 0;
	virtual int SendData(stUserFlags* pUserFlags) = 0;
	virtual	int SendData(stUserFlags* pUserFlags,const char* cSendData, uint nSendDataLen) = 0;
	virtual bool addUser(int nSocketID, int& nIndex) = 0;
	virtual bool CloseConnect(int nIndex) = 0;
	virtual int SendCmd(stUserFlags* pUserFlags, uint uMainCmd, uint uSubCmd, const char* cSendData, uint nDataLen) = 0;
	//virtual int SendCmd(stUserFlags* pUserFlags, uint uMainCmd, uint uSubCmd, const void* buf, uint nLen) = 0;
	virtual int BroadcastCmd(uint uMainCmd,  uint uSubCmd, const vector<uint>&& vcExceptUserID, const char* cSendData, uint nDataLen) = 0;
};