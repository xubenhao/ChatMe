//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "mysqlagent.h"

MySqlAgent::MySqlAgent(
    char* pStrHost_,
    int nHostLen_,
    char* pStrUser_,
    int nUserLen_,
    char* pStrPassword_,
    int nPasswordLen_)
{
    assert(nHostLen_ < (int)sizeof (m_strHost));
    assert(nUserLen_ < (int)sizeof(m_strUser));
    assert(nPasswordLen_ < (int)sizeof (m_strPassword));
    memset(m_strHost, 0, sizeof(m_strHost));
    memset(m_strUser, 0, sizeof(m_strUser));
    memset(m_strPassword, 0, sizeof(m_strPassword));

    memcpy(m_strHost, pStrHost_, nHostLen_);
    memcpy(m_strUser, pStrUser_, nUserLen_);
    memcpy(m_strPassword, pStrPassword_, nPasswordLen_);

    m_pConnection = nullptr;
    m_pStmt = nullptr;
    // mysql开发库初始化
    if (mysql_library_init (0, NULL, NULL))
    {
        LOG_TRACE
            << "mysql_library_init() failed";
        return;
    }

    // mysql初始化
    m_pConnection = mysql_init (NULL);
    if (m_pConnection == NULL)
    {
        LOG_TRACE
            << "mysql_init() failed (probably out of memory)";
        return;
    }

    // 从MYSQL得到MYSQL_STMT
    // MYSQL_STMT对于执行需要依赖参数绑定的MYSQL语句
    // 或对返回结果集需要执行参数绑定来提取结果的MYSQL语句是必须的
    m_pStmt = mysql_stmt_init(m_pConnection);
    if (m_pStmt == NULL)
    {
        LOG_TRACE
            << "Could not initialize statement handler";
    }
}

void MySqlAgent::Connect()
{
    assert(m_pConnection);
    // 连接到MYSQL服务器
    // 连接后便可使用MYSQL服务器所提供的服务
    if (mysql_real_connect (
        m_pConnection,
        m_strHost, // host
        m_strUser, // user
        m_strPassword, // pass
        nullptr,
        0,
        nullptr,
        0) == NULL)
    {
        LOG_TRACE
            << "mysql_real_connect() failed";
        mysql_close (m_pConnection);
    }
}

// this is adaptable to the sql that not need parameter bind and not return result.
bool MySqlAgent::Query(
    char* pContent_)// the paramater should pointer to a string ending with '\0'.
{
    assert(m_pConnection);
    // 执行不需要参数绑定，也不返回结果集的MYSQL语句
    // 直接使用mysql_query即可
    if (mysql_query (m_pConnection, pContent_) != 0)
    {
        char _strTip[100];
        memset(_strTip, 0, sizeof(_strTip));
        snprintf(_strTip, sizeof(_strTip), "Query for %s error", pContent_);
        LOG_TRACE
            << _strTip;
        return false;
    }

    return true;
}

bool MySqlAgent::StoreResult(MYSQL_RES** ppRes_)
{
    assert(m_pConnection && ppRes_);

    *ppRes_ = mysql_store_result (m_pConnection);
    if(*ppRes_)
    {
        return true;
    }

    if (mysql_field_count(m_pConnection) == 0)
    {
        char _strTip[100];
        memset(_strTip, 0, sizeof(_strTip));
        snprintf(
            _strTip,
            sizeof(_strTip),
            "Number of rows affected: %lu",
            (unsigned long) mysql_affected_rows (m_pConnection));
        LOG_TRACE
            << _strTip;
        return true;
    }
    else
    {
        LOG_TRACE
            << "Could not retrieve result set";
        return false;
    }
}

void MySqlAgent::FieldSeek(MYSQL_RES* pRes_, int nIndex_)
{
    assert(pRes_);
    mysql_field_seek (pRes_, nIndex_);
}

int MySqlAgent::NumFields(MYSQL_RES* pRes_)
{
    return mysql_num_fields (pRes_);
}

MYSQL_FIELD* MySqlAgent::FetchField(MYSQL_RES* pRes_)
{
    assert(pRes_);
    return mysql_fetch_field (pRes_);
}

MYSQL_ROW MySqlAgent::FetchRow(MYSQL_RES* pRes_)
{
    return mysql_fetch_row (pRes_);
}

int MySqlAgent::NumRows(MYSQL_RES* pRes_)
{
    return (int) mysql_num_rows (pRes_);
}

void MySqlAgent::FreeResult(MYSQL_RES* pRes_)
{
    mysql_free_result (pRes_);
}

bool MySqlAgent::Prepare(
    char* pContent_,
    int nLength_)
{
    // 将要执行的MYSQL语句提交到服务器
    if (mysql_stmt_prepare (
        m_pStmt,
        pContent_,
        nLength_) != 0)
    {
        LOG_TRACE
            << "Could not prepare statement";
        return false;
    }

    return true;
}

bool MySqlAgent::BindParameter(MYSQL_BIND* pBinds_)
{
    // 将MYSQL与?占位符进行关联
    // 关联后，?占位符有了实际的传入参数，参数的属性也在MYSQL_BIND中进行了说明
    if (mysql_stmt_bind_param (m_pStmt, pBinds_) != 0)
    {
        LOG_TRACE
            << "Could not bind parameters";
        return false;
    }

    return true;
}

bool MySqlAgent::Execute()
{
    if (mysql_stmt_execute (m_pStmt) != 0)
    {
        LOG_TRACE
            << "Could not execute statement";
        return false;
    }

    return true;
}

bool MySqlAgent::BindResult(MYSQL_BIND* pBinds_)
{
    // 执行结果集的参数绑定－－－用于将结果集中数据自动提取到所绑定的数据
    if (mysql_stmt_bind_result(m_pStmt, pBinds_) != 0)
    {
        LOG_TRACE
            << "Could not bind parameters";
        return false;
    }

    return true;
}

bool MySqlAgent::StoreResult()
{
    // 一次性从服务器取出所有结果
    if (mysql_stmt_store_result (m_pStmt) != 0)
    {
        LOG_TRACE
            << "Could not buffer result set";
        return false;
    }
    else
    {
        char _strTip[100];
        memset(_strTip, 0, sizeof(_strTip));
        snprintf(
            _strTip,
            sizeof(_strTip),
            "Number of rows retrieved: %lu",
            (unsigned long) mysql_stmt_num_rows (m_pStmt));
        LOG_TRACE
            << _strTip;
        return true;
    }
}

int MySqlAgent::Fetch()
{
    return mysql_stmt_fetch (m_pStmt);
}

void MySqlAgent::FreeResult()
{
    // 执行释放
    mysql_stmt_free_result (m_pStmt);
}

MySqlAgent::~MySqlAgent()
{
    assert(m_pConnection);
    assert(m_pStmt);

    mysql_stmt_close (m_pStmt);
    // 关闭
    mysql_close (m_pConnection);
    // 释放
    mysql_library_end ();
}
