#include "MysqlLink.h"
#include "../base/Logging.h"

CMysqlLink::CMysqlLink(int nPoolSize):m_nPoolSize(nPoolSize)
{
}

CMysqlLink::~CMysqlLink()
{
	std::lock_guard<mutex> lock{ m_lock };

	LOG_INFO << "pool size：" << m_qMySqlPool.size();
	while (m_qMySqlPool.size() > 0)
	{
		MYSQL* mysql = m_qMySqlPool.front();
		m_qMySqlPool.pop();
		mysql_close(mysql);
		mysql_library_end();
	}

	LOG_INFO << "m_qMySqlPool clear finish";
	getchar();
}

bool CMysqlLink::Initialize(const string& host, const string& user, const string& pwd, const string& dbname)
{
	if (host.empty() || user.empty() || pwd.empty() || dbname.empty())
	{
		LOG_ERROR << "CMysqlLink::Initialize param err!";
		return false;
	}

	m_DBInfo.strDBName = dbname;
	m_DBInfo.strHost = host;
	m_DBInfo.strUser = user;
	m_DBInfo.strPwd = pwd;

	//填充连接池
	for (int n = 0; n < m_nPoolSize; ++n)
	{
		if (!CreateLink())
			return false;
	}

	m_bInitFinish = true;
	return true;
}

MYSQL* CMysqlLink::CreateLink()
{
	std::lock_guard<mutex> lock{ m_initlock };
	//mysql_thread_init();
	MYSQL* mysql = mysql_init(NULL);
	if (nullptr == mysql)
	{
		LOG_ERROR << " mysql_init err ";
		return nullptr;
	}

	if (mysql_library_init(0, nullptr, nullptr) != QUERY_OK)
	{
		LOG_ERROR << mysql_error(mysql);
		return nullptr;
	}

	my_bool reconnect = 1;
	mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);
	if (nullptr == mysql_real_connect(mysql, m_DBInfo.strHost.c_str(), m_DBInfo.strUser.c_str(),
		m_DBInfo.strPwd.c_str(), m_DBInfo.strDBName.c_str(), 0, NULL, CLIENT_MULTI_STATEMENTS))
	{
		LOG_ERROR << mysql_error(mysql);
		mysql_close(mysql);
		//mysql_thread_end();
		return nullptr;
	}

	mysql_query(mysql, "set names utf8");
	PushLink(mysql);
	//mysql_thread_end();
	return mysql;
}

void CMysqlLink::ResetIdelConnect()
{
	int nIdelNum = 0;
	{
		std::lock_guard<mutex> lock{ m_lock };
		while (m_qMySqlPool.size() > 0)
		{
			MYSQL* mysql = m_qMySqlPool.front();
			m_qMySqlPool.pop();
		
			mysql_reset_connection(mysql);
			mysql_close(mysql);
			mysql = nullptr;
			mysql_library_end();
			++nIdelNum;
		}
	}


	LOG_INFO << "空闲链接数量:" << nIdelNum;
	for (int n = 0; n < nIdelNum; ++n)
	{
		if (!CreateLink())
		{
			LOG_ERROR << "链接添加失败";
		}
	}
}

vector<vector<Field>> CMysqlLink::Query(const char* sql, string& strErr)
{
	if (!CanDoSql())
		return vector<vector<Field>>{};

	//mysql_thread_init();
	MYSQL* mysql = PopLink();
	if (nullptr == mysql)
	{
		strErr = "no mysql link can user!";
		return vector<vector<Field>>{};
	}

	if (mysql_real_query(mysql, sql, strlen(sql)) != QUERY_OK)
	{
		unsigned int uErrno = mysql_errno(mysql);
		strErr += mysql_error(mysql);
		PushLink(mysql);
		return vector<vector<Field>>{};
		//if (CR_SERVER_GONE_ERROR == uErrno)
		//{
		//	if (false == reConnectMysql(mysql)) //尝试重连
		//	{
		//		return nullptr;
		//	}

		//	if (mysql_real_query(mysql, sql, strlen(sql)) != QUERY_OK)
		//	{
		//		strErr += mysql_error(mysql);
		//		return nullptr;
		//	}
		//}
		//else
		//{
		//	strErr += mysql_error(mysql);
		//	return nullptr;
		//}
	}

	MYSQL_RES* result = nullptr;
	MYSQL_FIELD* fields = nullptr;
	MYSQL_ROW row = nullptr;
	unsigned long int* ulFieldLength = nullptr;
	uint32_t unFieldCount = 0;
	vector<vector<Field>> vcResults;
	do
	{
		if (!(result = mysql_store_result(mysql)))
		{
			//LOG_ERROR << "mysql_store_result err!";
			//strErr += mysql_error(mysql);
			continue;
		}

		unFieldCount = mysql_field_count(mysql);
		while (row = mysql_fetch_row(result))
		{
			ulFieldLength = mysql_fetch_lengths(result);
			fields = mysql_fetch_fields(result);
			vector<Field> vcResult;
			for (int nField = 0; nField < unFieldCount; ++nField)
			{
				MYSQL_FIELD mysql_field = fields[nField];
				Field filed;
				filed.SetName(mysql_field.name);
				filed.SetType(ConvertNativeType(mysql_field.type));
				filed.SetValue(row[nField], ulFieldLength[nField]);
				vcResult.emplace_back(filed);
			}
			vcResults.emplace_back(vcResult);
		}

		mysql_free_result(result);
	} while (!mysql_next_result(mysql));


	//if (reConnectMysql(mysql))
	//{
	//}
	//重置失败？
	if (mysql_reset_connection(mysql) != QUERY_OK)
		LOG_ERROR << mysql_error(mysql);

	PushLink(mysql);
	//mysql_thread_end();
	return std::move(vcResults);
}

vector<vector<Field>> CMysqlLink::PQuery(string& strErr, const char* format, ...)
{
	if (!format)
	{
		LOG_ERROR << "PQuery param is null!";
		return vector<vector<Field>>{};
	}

	va_list ap;
	char szQuery[MAX_QUERY_LEN];
	va_start(ap, format);
	int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
	va_end(ap);

	if (res == -1)
	{
		strErr += "SQL Query truncated (and not execute) for format: ";
		strErr += format;
		return vector<vector<Field>>{};
	}

	return Query(szQuery, strErr);
}

bool CMysqlLink::Execute(const char* sql, string& strErr)
{
	if (!CanDoSql())
		return false;

	MYSQL* mysql = PopLink();
	if (nullptr == mysql)
	{
		strErr = "no mysql link can user!";
		return false;
	}

	if (mysql_query(mysql, sql) != QUERY_OK)
	{
		unsigned int uErrno = mysql_errno(mysql);
		strErr += mysql_error(mysql);
		PushLink(mysql);
		return false;
		//if (CR_SERVER_GONE_ERROR == uErrno)
		//{
		//	if (false == reConnectMysql(mysql)) //尝试重连
		//	{
		//		strErr += m_DBInfo.strDBName;
		//		strErr += " Link Err!";
		//		return false;
		//	}

		//	if (mysql_query(mysql, sql) != QUERY_OK)
		//	{
		//		strErr += mysql_error(mysql);
		//		return false;
		//	}

		//}
		//else
		//{
		//	strErr += mysql_error(mysql);
		//	return false;
		//}
	}

	PushLink(mysql);
	return true;
}

bool CMysqlLink::Execute(const char* sql, uint32_t& uAffectedCount, string& strErr)
{
	if (!CanDoSql())
		return false;

	MYSQL* mysql = PopLink();
	if (nullptr == mysql)
	{
		strErr = "no mysql link can user!";
		return false;
	}

	if (mysql_query(mysql, sql) != QUERY_OK)
	{
		strErr += mysql_error(mysql);
		unsigned int uErrno = mysql_errno(mysql);
		strErr += mysql_error(mysql);
		PushLink(mysql);
		return false;
		//if (CR_SERVER_GONE_ERROR == uErrno)
		//{
		//	if (false == reConnectMysql(mysql))
		//	{
		//		strErr += m_DBInfo.strDBName;
		//		strErr += " Link Err!";
		//		return false;
		//	}

		//	if (mysql_query(mysql, sql) != QUERY_OK)
		//	{
		//		strErr += mysql_error(mysql);
		//		return false;
		//	}
		//}
		//else
		//{
		//	strErr += mysql_error(mysql);
		//	return false;
		//}
	}

	PushLink(mysql);
	uAffectedCount = static_cast<uint32_t>(mysql_affected_rows(mysql));
	return true;
}

bool CMysqlLink::PExecute(const char* format, ...)
{
	if (!format)
		return false;

	va_list ap;
	char szQuery[MAX_QUERY_LEN];
	va_start(ap, format);
	int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
	va_end(ap);

	if (res == -1)
	{
		LOG_ERROR << "SQL Query truncated (and not execute) for format: " << format;
		return false;
	}
	
	string strErr;
	return Execute(szQuery, strErr);
}

void CMysqlLink::ClearStoredResults()
{
	if (!CanDoSql())
		return;

	std::lock_guard<mutex> lock{ m_lock };
	while (m_qMySqlPool.size() > 0)
	{
		MYSQL_RES* result = NULL;
		MYSQL* mysql = m_qMySqlPool.front();
		m_qMySqlPool.pop();

		while (!mysql_next_result(mysql))
		{
			if ((result = mysql_store_result(mysql)) != NULL)
			{
				mysql_free_result(result);
			}
		}
	}
}

void CMysqlLink::PushLink(MYSQL* mysql)
{
	std::lock_guard<mutex> lock{ m_lock };
	m_qMySqlPool.push(mysql);
}

MYSQL* CMysqlLink::PopLink()
{
	std::lock_guard<mutex> lock{ m_lock };
	if (m_qMySqlPool.empty())
	{
		LOG_INFO << "cur mysql pool is empty!";
		return nullptr;
	}

	MYSQL* mysql = m_qMySqlPool.front();
	m_qMySqlPool.pop();
	return mysql;
}

bool CMysqlLink::CanDoSql()
{
	if (!m_bInitFinish)
	{
		LOG_ERROR << "mysql is not init finish!";
		return false;
	}

	return true;
}

enum Field::DataTypes CMysqlLink::ConvertNativeType(enum_field_types mysqlType) const
{
	switch (mysqlType)
	{
	case FIELD_TYPE_TIMESTAMP:
	case FIELD_TYPE_DATE:
	case FIELD_TYPE_TIME:
	case FIELD_TYPE_DATETIME:
	case FIELD_TYPE_YEAR:
	case FIELD_TYPE_STRING:
	case FIELD_TYPE_VAR_STRING:
	case FIELD_TYPE_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case FIELD_TYPE_SET:
	case FIELD_TYPE_NULL:
		return Field::DB_TYPE_STRING;
	case FIELD_TYPE_TINY:
	case FIELD_TYPE_SHORT:
	case FIELD_TYPE_LONG:
	case FIELD_TYPE_INT24:
	case FIELD_TYPE_LONGLONG:
	case FIELD_TYPE_ENUM:
		return Field::DB_TYPE_INTEGER;
	case FIELD_TYPE_DECIMAL:
	case FIELD_TYPE_FLOAT:
	case FIELD_TYPE_DOUBLE:
		return Field::DB_TYPE_FLOAT;
	default:
		return Field::DB_TYPE_UNKNOWN;
	}
}