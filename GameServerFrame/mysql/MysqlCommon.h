#pragma once
#include "Field.h"
#include "../base/Singleton.h"

struct IAsyCallBack
{
	virtual bool DBCallBack(vector<vector<Field>>&& vcResults, int nDbCallBackID) = 0;
};

enum stPcdParamType
{
	P_IN = 0,  //IN
	P_OUT ,	   //OUT
	P_INOUT	   //INOUT
};

struct stProcedureParam
{
	stProcedureParam()
	{

	}
	stProcedureParam(const stPcdParamType tp, const string& strName, const string& strValue)
	{
		type = tp;
		strFieldName = std::move(strName);
		strFieldValue= std::move(strValue);
	}

	stProcedureParam(const stPcdParamType tp, const string& strName, const int nValue)
	{
		type = tp;
		strFieldName = std::move(strName);

		char szValue[64] = "";
		sprintf(szValue, "%d", nValue);
		strFieldValue = std::move(szValue);
	}


	stPcdParamType type;	//存储过程参数类型
	string	strFieldName;	//参数名称
	string	strFieldValue;	//参数值
};

#define  DB_CALL_LOGIN 0x01

