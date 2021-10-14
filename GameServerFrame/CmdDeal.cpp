#include "CmdDeal.h"
#include <stdio.h>

CCmdDeal::CCmdDeal()
{
}

CCmdDeal::~CCmdDeal()
{
}

void CCmdDeal::DealCmd(uint nMainCmd, uint nSubCmd, char* cData, uint nDataLen, char* cSendData, uint& nSendLen)
{
	switch (nMainCmd)
	{
	case CMD_LOGIN:
	{
		static int iIndex = 0;
		++iIndex;
		//测试数据
		char cSend[32] = {};
		sprintf(cSend, "恭喜你登录了 + %d", iIndex);
		PackData(1, 1, cSend, strlen(cSend), cSendData, nSendLen);
		//_DealLoginData(cData, nDataLen);

	}
	break;
	default:
		break;
	}
}

bool CCmdDeal::_DealLoginData(char* cData, uint nDataLen)
{
	return false;
}
