#include "AsyDBTask.h"
#include "../base/Logging.h"

//sql异步调用
CAsyDBTask::CAsyDBTask()
{

}

CAsyDBTask::CAsyDBTask(string&& strQuery, IAsyCallBack* pCallBack, std::shared_ptr<CMysqlLink> pCon, int nDBCallBackID)
{
	m_strQeury = std::move(strQuery);
	m_pCallBack = pCallBack;
	m_pDataBaseCon = pCon;
	m_nDBCallBackID = nDBCallBackID;
}

CAsyDBTask::~CAsyDBTask()
{
}

bool CAsyDBTask::doTask()
{
	if (!m_pDataBaseCon)
	{
		LOG_ERROR << "CAsyDBTask::doTask() m_pDataBaseCon err!";
		return false;
	}

	vector<vector<Field>> vcResults = std::move(m_pDataBaseCon->Query(m_strQeury.c_str(), m_strErr));
	if (!m_strErr.empty())
	{
		LOG_ERROR << "CAsyDBTask::doTask() err:" << m_strErr;
		return false;
	}

	if (m_pCallBack)
		m_pCallBack->DBCallBack(std::move(vcResults), m_nDBCallBackID);

	return true;
}


//存储过程sql异步调用
CAsyProcedureTask::CAsyProcedureTask(IAsyCallBack* pCallBack, std::shared_ptr<CMysqlLink> pCon, int nDBCallBackID)
{
	m_pCallBack = pCallBack;
	m_pDataBaseCon = pCon;
	m_nDBCallBackID = nDBCallBackID;
}

CAsyProcedureTask::~CAsyProcedureTask()
{
	m_strProcedureName.clear();
	m_vcProcedureParam.clear();
}

bool CAsyProcedureTask::doTask()
{
	if (!m_pDataBaseCon)
	{
		LOG_ERROR << "CAsyProcedureTask::doTask() m_pDataBaseCon err!";
		return false;
	}

	//设置参数
	m_strQeury = "SET ";
	for (auto param : m_vcProcedureParam)
	{
		m_strQeury += param.strFieldName;
		m_strQeury += "=";
		m_strQeury += param.strFieldValue;
		m_strQeury += ",";
	}
	m_strQeury = m_strQeury.substr(0, m_strQeury.length() - 1); //去掉最后一个逗号
	m_strQeury += ";";

	//调用存储过程
	m_strQeury += ("CALL " + m_strProcedureName + "(");
	for (auto param : m_vcProcedureParam)
	{
		m_strQeury += param.strFieldName;
		m_strQeury += ",";
	}
	m_strQeury = m_strQeury.substr(0, m_strQeury.length() - 1); //去掉最后一个逗号
	m_strQeury += ")";
	m_strQeury += ";";

	//读取结果集
	m_strQeury += "SELECT ";
	for (auto param : m_vcProcedureParam)
	{
		if (P_OUT == param.type || P_INOUT == param.type) //读取IN或者INOUT参数的结果
		{
			m_strQeury += param.strFieldName;
			m_strQeury += ",";
		}
	}
	m_strQeury = m_strQeury.substr(0, m_strQeury.length() - 1); //去掉最后一个逗号
	m_strQeury += ";";

	vector<vector<Field>> vcResults = std::move(m_pDataBaseCon->Query(m_strQeury.c_str(), m_strErr));
	if (!m_strErr.empty())
	{
		LOG_ERROR << m_strErr;
		return false;
	}

	if (m_pCallBack)
		m_pCallBack->DBCallBack(std::move(vcResults), m_nDBCallBackID);

	return true;
}

void CAsyProcedureTask::AddParam(stPcdParamType type, const string& strFieldName, const string& strFieldValue)
{
	m_vcProcedureParam.push_back(stProcedureParam(type, strFieldName, strFieldValue));
}

void CAsyProcedureTask::AddParam(stPcdParamType type, const string& strFieldName, const int& nFieldValue)
{
	m_vcProcedureParam.push_back(stProcedureParam(type, strFieldName, nFieldValue));
}
