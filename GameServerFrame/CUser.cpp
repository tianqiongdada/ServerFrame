#include "CUser.h"
#include "base/Logging.h"
#include <ctime>
#include "common.h"
#include "commonCmd.h"

CUser::CUser(stUserFlags* pUserFlags) : m_pUserFlags(pUserFlags)
{
	m_utID = 0;
	m_state = STATU_NOT_LOGIN;
	m_tLoginTime = 0;
	m_tLastPing = 0;
}

CUser::~CUser()
{

}

bool CUser::DBCallBack(vector<vector<Field>>&& vcResults, int nDbCallBackID)
{
	switch (nDbCallBackID)
	{
	case DB_CALL_LOGIN:
	{
		if (vcResults.empty() || vcResults.size() > 1)
			break;

		vector<Field>& vcInfo = vcResults[0];
		m_strName = vcInfo[0].GetString();
		m_utID = vcInfo[1].GetInt32();
		m_state = STATU_LOGIN;
		m_tLoginTime = std::time(0);

		//返回登陆成功信息id加姓名
		struct stUserInfo
		{
			uint uID;
			char szName[USER_NAME_LEN];
			stUserInfo()
			{
				uID = 0;
				memset(szName, 0, USER_NAME_LEN);
			}
		};
		

		stUserInfo userInfo;
		userInfo.uID = m_utID;
		memcpy(userInfo.szName, m_strName.c_str(), m_strName.size() > USER_NAME_LEN ? USER_NAME_LEN : m_strName.size());
		unsigned szID[4] = {};
		memcpy(szID, &userInfo.uID, 4);

		g_Member.pUserMgr->SendCmd(m_pUserFlags, CMD_LOGIN_SUCESS, 0, (const char*)&userInfo, sizeof(stUserInfo));

		g_Member.pUserMgr->BroadcastCmd(CMD_USER_LOING, 0, vector<uint>{m_utID}, m_strName.c_str(), m_strName.size());

		LOG_INFO << "User Login, Name:" << m_strName << " ID:" << m_utID;
		break;
	}
	default:
		break;
	}
}
