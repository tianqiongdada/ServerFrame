#pragma once
#include <memory>
#include "MysqlCommon.h"
#include "MysqlLinkManager.h"
#include "AsyDBTask.h"

enum emHostID
{
	HOST_TEST = 0,
};

#define MAXCMDLEN 8192

struct STableField
{
	STableField(){}
	STableField(std::string strName,std::string strType,std::string strIndex):
		m_strName(strName),
		m_strType(strType),
		m_strDesc(strIndex)
	{
	}
	std::string m_strName;
	std::string m_strType;
	std::string m_strDesc;
};

struct STableInfo
{
	STableInfo(){}
	STableInfo(std::string strName)
		:m_strName(strName)
	{
	}
	std::string m_strName;
	std::map<std::string,STableField> m_mapField;
	std::string m_strKeyString;
};

struct STableData
{
	STableData(){}
	STableData(std::string strName):m_strName(strName)
	{
	}
	std::string m_strName;
	std::map<std::string,std::string> m_mapData;
};

//mysql 数据库管理
//管理多台服务器的数据库链接
class CMysqlManager : public IAsyCallBack
{
public:
    CMysqlManager(void);
    virtual ~CMysqlManager(void);

public:
	bool Init();
	bool InitMysqlLink(emHostID hostID, char* szHost, const char* szUser, const char* szPwd, const std::vector<string>& vcDatabase);

    void InitSTableInfo(const STableInfo& info);
    bool CreateTable(void);
    bool InsertData(const STableData& data);
    bool DeleteData(const string& cmd);

	std::shared_ptr<CMysqlLinkManager> GetMysqlLinkManager(emHostID hostID) { return m_mapMysqlLinkManager[hostID]; }
public:
	bool DBCallBack(vector<vector<Field>>&& vcResults, int nDbCallBackID);

private:
	bool _IsDBExist();
	bool _CreateDB();
	bool _CheckTable(const STableInfo& table);
	bool _CreateTable(const STableInfo& table);
	bool _UpdateTable(const STableInfo& table);


protected:
	std::vector<STableInfo> m_vecTableInfo;

private:
	std::map<emHostID, std::shared_ptr<CMysqlLinkManager>> m_mapMysqlLinkManager;

};

extern CMysqlManager g_MysqlManager;

