#pragma once
#include "commonCmd.h"

#define MAX_RECVBUF_LEN	4096+32

class CCmdDeal
{
public:
	CCmdDeal();
	virtual ~CCmdDeal();

public:
	void DealCmd(uint nMainCmd, uint nSubCmd, 
		char* cData, uint nDataLen, char* cSendData, uint &nSendLen);

private:
	bool _DealLoginData(char* cData, uint nDataLen);
};

