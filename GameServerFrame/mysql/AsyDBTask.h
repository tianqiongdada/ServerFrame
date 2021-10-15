#pragma once
//#include "DatabaseMysql.h"
#include "MysqlCommon.h"
#include "MysqlLinkManager.h"
#include "MysqlLink.h"

//提交一个异步任务
class CAsyDBTask
{
public:
	CAsyDBTask();
	CAsyDBTask(string&& strQuery, IAsyCallBack* pCallBack, std::shared_ptr<CMysqlLink> pCon, int nDBCallBackID);
	virtual ~CAsyDBTask();

public:
	virtual bool doTask();

protected:
	string m_strQeury;
	string m_strErr;
	IAsyCallBack* m_pCallBack;
	int m_nDBCallBackID;
	std::shared_ptr<CMysqlLink> m_pDataBaseCon;
};


class CAsyProcedureTask : public CAsyDBTask
{
public:
	CAsyProcedureTask(IAsyCallBack* pCallBack, std::shared_ptr<CMysqlLink> pCon, int nDBCallBackID);
	virtual ~CAsyProcedureTask();

public:
	virtual bool doTask();

public:
	void SetProcedureName(const string& strName) { m_strProcedureName = std::move(strName); }

	//FileName带@ 设置为数据库全局变量
	void AddParam(stPcdParamType type, const string& strFieldName, const string& strFieldValue);  //添加参数
	void AddParam(stPcdParamType type, const string& strFieldName, const int& nFieldValue);  //添加参数

private:
	string	m_strProcedureName;		//存储过程名称
	std::vector<stProcedureParam> m_vcProcedureParam;	//存储过程调用参数，写入顺序不能搞错
};

static bool doAsyDBTask(std::shared_ptr<CAsyDBTask> pAsyDBTask)
{
	if (pAsyDBTask.get())
		return pAsyDBTask->doTask();

	return false;
}


static bool doAsyDBTask2(CAsyDBTask* pAsyDBTask)
{
	bool bSuc = false;
	if (pAsyDBTask)
	{
		bSuc = pAsyDBTask->doTask();
		//delete pAsyDBTask;
		//pAsyDBTask = nullptr;
	}

	return bSuc;
}
