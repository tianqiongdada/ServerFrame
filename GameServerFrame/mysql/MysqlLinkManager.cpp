#include "MysqlLinkManager.h"
#include "../base/Logging.h"
CMysqlLinkManager::CMysqlLinkManager(string&& strHost, string&& strUser, string&& strPwd)
{
	m_strHost = std::move(strHost);
	strHost = "";

	m_strUser = std::move(strUser);
	strUser = "";

	m_strPwd = std::move(strPwd);
	strPwd = "";
	
}

CMysqlLinkManager::~CMysqlLinkManager()
{

}

bool CMysqlLinkManager::Init()
{
	if (!AddLink("test"))
		return false; 

	return true;
}

bool CMysqlLinkManager::Init(const std::vector<string>& vcDatabase)
{
	for (auto database : vcDatabase)
	{
		AddLink(std::move(database));
	}
}

bool CMysqlLinkManager::AddLink(string&& baseName)
{
	if (m_mapMysqlLink.find(baseName) != m_mapMysqlLink.end())
	{
		LOG_ERROR << baseName << " is already link!";
		return false;
	}

	std::shared_ptr<CMysqlLink> pMysqlLink = std::make_shared<CMysqlLink>(MAX_DB_LINK_SIZE);
	if (pMysqlLink->Initialize(m_strHost, m_strUser, m_strPwd, baseName))
	{
		m_mapMysqlLink[std::move(baseName)] = pMysqlLink;
		return true;
	}
	else
	{
		LOG_ERROR << baseName << " DataBase init Err!";
		return false;
	}
}
