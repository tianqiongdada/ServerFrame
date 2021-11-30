#include "CmdDeal.h"
#include <stdio.h>
#include "mysql/MysqlManager.h"
#include "mysql/AsyDBTask.h"
#include "common.h"
#include "UserData.h"

CCmdDeal::CCmdDeal()
{
}

CCmdDeal::~CCmdDeal()
{
}

void CCmdDeal::DealCmd(int nIndex, uint nMainCmd, uint nSubCmd, char* cData, uint nDataLen, char* cSendData, uint& nSendLen)
{
	switch (nMainCmd)
	{
	case CMD_LOGIN:
	{
		std::shared_ptr<CMysqlLink> pLink = g_MysqlManager.GetMysqlLinkManager(HOST_LOCAL)->GetLink("User");
		stUserData* pData = g_Member.pUserMgr->GetUserData(nIndex);
		if (pData && pLink)
		{
			std::shared_ptr<CAsyProcedureTask> pTask = make_shared<CAsyProcedureTask>(pData->pUser, pLink, DB_CALL_LOGIN);
			pTask->SetProcedureName("getUser");
			char cName[32] = "";
			sprintf(cName, "\"%s\"", cData);
			pTask->AddParam(P_INOUT, "@name", cName);
			pTask->AddParam(P_OUT, "@id", 0);
			pTask->AddParam(P_OUT, "@returnValue", 0);
			g_Member.pThreadPool->TaskCommit(doAsyDBTask, pTask);
		}

		break;
	}
	case CMD_TALK:
	{
		if (g_Member.pUserMgr)
			g_Member.pUserMgr->BroadcastCmd(CMD_TALK, 0, vector<uint>{}, cData, nDataLen);
		break;
	}
	case CMD_PING:
	{
		if (g_Member.pUserMgr)
		{
			stUserData* pUserData = g_Member.pUserMgr->GetUserData(nIndex);
			if (pUserData)
			{
				if (pUserData->pUser)
					pUserData->pUser->UpdateLastPingTime(std::time(0));
			}

		}
		break;
	}
	default:
		break;
	}
}

bool CCmdDeal::_DealLoginData(char* cData, uint nDataLen)
{
	return false;
}
