#include "MysqlManager.h"
#include <sstream>
#include "../base/Logging.h"
#include "../base/Singleton.h"
CMysqlManager g_MysqlManager;

CMysqlManager::CMysqlManager(void)
{

}

CMysqlManager::~CMysqlManager(void)
{
    //m_poConn.reset();
}


bool CMysqlManager::Init()
{
	return InitMysqlLink(HOST_TEST, "172.17.0.12", "root", "XiOnG02082958413");
}


bool CMysqlManager::InitMysqlLink(emHostID hostID, char* szHost, const char* szUser, const char* szPwd)
{
	if (m_mapMysqlLinkManager.find(hostID) != m_mapMysqlLinkManager.end())
	{
		LOG_ERROR << "hostID:" << hostID << "is already init!";
		return false;
	}

	if (!szHost || !szUser || !szPwd)
	{
		LOG_ERROR << "CMysqlManager::InitMysqlLink param is valid!";
		return false;
	}

	std::shared_ptr<CMysqlLinkManager> pLinkManager = std::make_shared<CMysqlLinkManager>(szHost, szUser, szPwd);

	if (!pLinkManager->Init())
		return false;

	m_mapMysqlLinkManager[hostID] = pLinkManager;
	return true;
}

bool CMysqlManager::_IsDBExist()
{
	//if (NULL == m_poConn)
	//{
	//	return false;
	//}

	//QueryResult* pResult = m_poConn->Query("show databases");
	
	return false;
}

bool CMysqlManager::_CreateDB()
{
	
	return false;
}

void CMysqlManager::InitSTableInfo(const STableInfo& info)
{
	m_vecTableInfo.push_back(info);
}

bool CMysqlManager::CreateTable(void)
{
	if(m_vecTableInfo.size() == 0)
	{
		LOG_WARN << "The CreateTable's Infomation is empty, it will not be created!";

		return true;
	}
	for (size_t i = 0; i < m_vecTableInfo.size(); i++)
	{
		STableInfo table = m_vecTableInfo[i];
		if (!_CheckTable(table))
		{
	  		LOG_FATAL << "CMysqlManager::Init, table check failed : " << table.m_strName;
		 	return false;
	  	}
	 }

	m_vecTableInfo.clear();
	//	LOG_INFO << "m_vecTableInfo.size() = " << m_vecTableInfo.size() ;
	return true;
}

bool CMysqlManager::InsertData(const STableData& data)
{
	if (data.m_strName.find_first_not_of("\t\r\n ") == string::npos)
	{
	   	LOG_WARN << "CMysqlManager::_CheckTable, tale info not valid";
	   	return true;
	}

	string field;
	string value;
	 for (map<string, string>::const_iterator it = data.m_mapData.begin();
		 it != data.m_mapData.end(); ++it)
	 {
		 field += it->first;
		 field += ",";
		 value += it->second;
		 value += ",";
	 }

	 field.erase(field.end()-1);
	 value.erase(value.end()-1);

	 stringstream ss;
	 ss << "INSERT INTO " << data.m_strName << "("
		<< field << ") " << "VALUES(" << value << ")";

	 string sql = ss.str();

	 //if (m_poConn->Execute(sql.c_str()))
	 //{
		//	 LOG_INFO << sql;
	 //}
	 //else
	 //{
		//   LOG_ERROR << "CMysqlManager::InsertData failed : " << sql;
		//  return false;
	 //}

	 return true;
}


bool CMysqlManager::DeleteData(const string& cmd)
{
	string sql = cmd.c_str();

	//if (m_poConn->Execute(sql.c_str()))
	//{
	//	LOG_INFO << sql;
	//}
	//else
	//{
	//	LOG_ERROR << "CMysqlManager::InsertData failed : " << sql;
	//	return false;
	//}

	return true;
}


bool CMysqlManager::DBCallBack(vector<vector<Field>>&& vcResults, int nDbCallBackID)
{
	switch (nDbCallBackID)
	{
	case CALL_LOGIN:
	{
		if (vcResults.empty())
			break;

		//static int n = 0;
		//for (auto result : vcResults)
		//{
		//	++n;
		//	LOG_INFO << "line:"<< n << " " << result[0].GetName() <<":" << result[0].GetInt32() << ","<< result[1].GetName() << ":" << result[1].GetString() ;
		//}

		//CallLoginOut login;
		////LOG_INFO << "Result row:" << pResult->GetRowCount();
		//while (true)
		//{
		//	Field* pRow = pResult->Fetch();
		//	if (pRow == NULL)
		//		break;

		//	//LOG_INFO << "num= " << pRow[0].GetInt32()
		//	//	<< "  name= " << pRow[1].GetString();
		//	if (!pResult->NextRow())
		//	{
		//		break;
		//	}
		//}

		//pResult->EndQuery();
		break;
	}
	default:
		break;
	}
	return true;
}

bool CMysqlManager::_CheckTable(const STableInfo& table)
{
	/*if (NULL == m_poConn)
	{
		return false;
	}*/

	if (table.m_strName.find_first_not_of("\t\r\n ") == string::npos)
	{
		LOG_WARN << "CMysqlManager::_CheckTable, tale info not valid";
		return true;
	}

	return true;
}

bool CMysqlManager::_CreateTable(const STableInfo& table)
{
	if (table.m_mapField.size() == 0)
	{
		LOG_ERROR << "CMysqlManager::_CreateTable, table info not valid, " << table.m_strName;
		return false;
	}
	stringstream ss;
	ss << "CREATE TABLE IF NOT EXISTS " << table.m_strName << " (";
	
	for (map<string, STableField>::const_iterator it = table.m_mapField.begin();
		it != table.m_mapField.end(); ++it)
	{
		if (it != table.m_mapField.begin())
		{
			ss << ", ";
		}

		STableField field = it->second;
		ss << field.m_strName << " " << field.m_strType;		
	}

	if (table.m_strKeyString != "")
	{
		ss << ", " << table.m_strKeyString;
	}

	ss << ")default charset = utf8, ENGINE = InnoDB;";
	//if (m_poConn->Execute(ss.str().c_str()))
	//{
	//	return true;
	//}

	return false;
}

