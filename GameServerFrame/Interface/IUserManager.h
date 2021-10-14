#pragma once
struct stUserFlags;
struct stUserData;

struct IUserManger
{
	virtual bool RecvData(int nIndex, char* cRcvData, uint nDataLen) = 0;
	virtual stUserData* GetUser(int nIndex) = 0;
	virtual int SendData(stUserFlags* pUserFlags) = 0;
	virtual	int SendData(stUserFlags* pUserFlags, char* cSendData, uint nSendDataLen) = 0;
	virtual bool addUser(int nSocketID, int& nIndex) = 0;
	virtual bool CloseConnect(int nIndex) = 0;
	virtual int SendData(stUserFlags* pUserFlags, uint uMainCmd, uint uSubCmd, char* cSendData) = 0;
	virtual void KeepLife() = 0;
};