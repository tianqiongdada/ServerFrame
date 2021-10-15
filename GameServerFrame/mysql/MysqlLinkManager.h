#pragma once
#include "MysqlLink.h"
#include <future>
#include <map>

#define MAX_DB_LINK_SIZE 16 

//一台服务器数据库的链接管理
//线程安全
class CMysqlLinkManager
{
public:
	CMysqlLinkManager(string&& strHost, string&& strUser, string&& strPwd);
	virtual ~CMysqlLinkManager();

private:
	string	m_strHost;	//主机
	string	m_strUser;	//登陆用户
	string	m_strPwd;	//登陆密码
	std::map<string, std::shared_ptr<CMysqlLink>> m_mapMysqlLink; //连接对象

public:
	//bool Init();
	bool Init(const std::vector<string>& vcDatabase);
	bool AddLink(string&& baseName);
	std::shared_ptr<CMysqlLink> GetLink(const string& baseName) { return m_mapMysqlLink[baseName]; }
};

