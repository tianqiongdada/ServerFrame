#pragma once
#include <stdint.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mutex>
#include <queue>
#include <fstream>
#include <stdarg.h>
#include <atomic>
#include "Field.h"

struct DatabaseInfo
{
	string strHost;
	string strUser;
	string strPwd;
	string strDBName;
};

#define MAX_QUERY_LEN   1024
#define QUERY_OK 0


class CMysqlLink
{
public:
	CMysqlLink(int nPoolSize);
	virtual ~CMysqlLink();

public:
	bool Initialize(const string& host, const string& user, const string& pwd, const string& dbname);
	MYSQL* CreateLink();
	void ResetIdelConnect();
	vector<vector<Field>> Query(const char* sql, string& strErr);
	vector<vector<Field>> Query(const string& sql, string& strErr) { return Query(sql.c_str(), strErr); }
	vector<vector<Field>> PQuery(string& strErr, const char* format, ...);
	bool Execute(const char* sql, string& strErr);
	bool Execute(const char* sql, uint32_t& uAffectedCount, string& strErr);
	bool PExecute(const char* format, ...);
	void ClearStoredResults();
	DatabaseInfo GetDBInfo() { return m_DBInfo; }
	void PushLink(MYSQL* mysql);
	MYSQL* PopLink();
	bool CanDoSql();
	enum Field::DataTypes ConvertNativeType(enum_field_types mysqlType) const;


private:
	DatabaseInfo m_DBInfo;				//链接库信息
	queue<MYSQL*> m_qMySqlPool;			//连接池,空闲连接		
	mutex m_lock;						//连接池锁
	mutex m_initlock;					//mysql 初始化锁
	int	m_nPoolSize{16};				//连接池大小，一般来说要超过线程池的大小
	atomic<bool> m_bInitFinish{false};	//初始化完成标志
};

