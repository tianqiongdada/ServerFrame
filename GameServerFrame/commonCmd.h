#pragma once
#include <sys/types.h>
#include <string.h>

#define CMD_LOGIN	0x01		//登陆
#define CMD_LOGIN_SUCESS	0x02		//登陆成功
#define CMD_USER_LOING	0x03		//有用户登陆成功


#define CMD_TALK	0xFE		//聊天
#define CMD_PING	0xFF		//ping

//协议头
struct HeadInfo
{
	uint u_nMainCmd;	//主命令码
	uint u_nSubCmd;		//子命令码
	uint u_nRetain;		//保留项
	uint u_nDataLen;	//数据长度

	HeadInfo()
	{
		u_nMainCmd = 0;
		u_nSubCmd = 0;
		u_nRetain = 0;
		u_nDataLen = 0;
	}
};

static void PackData(uint nMainCmd, uint nSubCmd, const char* cSrcData, uint nSrcLen, char* cSendData, uint& nSendDataLen)
{
	HeadInfo headInfo;
	headInfo.u_nMainCmd = nMainCmd;
	headInfo.u_nSubCmd = nSubCmd;
	headInfo.u_nDataLen = nSrcLen;

	int nHeadLen = sizeof(HeadInfo);
	memcpy(cSendData + nSendDataLen, &headInfo, nHeadLen); //包头
	memcpy(cSendData + nSendDataLen + nHeadLen, cSrcData, nSrcLen);
	nSendDataLen += (nHeadLen + nSrcLen);
}
