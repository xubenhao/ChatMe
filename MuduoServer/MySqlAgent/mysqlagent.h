//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef MYSQL_AGENT_MYSQLAGENT_H
#define MYSQL_AGENT_MYSQLAGENT_H
#include "header.h"

class MySqlAgent
{
public:

public:
    MySqlAgent(
        char* pStrHost_,
        int nHostLen_,
        char* pStrUser_,
        int nUserLen_,
        char* pStrPassword_,
        int nPasswordLen_);
    void Connect();

    ~MySqlAgent();

    // no use parameter bind to set sql and get result
    // for no parameter bind and no returning result sql
    bool Query(
        char* pContent_);
    bool StoreResult(MYSQL_RES** ppRes_);
    void FieldSeek(MYSQL_RES* pRes_, int nIndex_);
    int NumFields(MYSQL_RES* pRes_);
    MYSQL_FIELD* FetchField(MYSQL_RES* pRes_);
    MYSQL_ROW FetchRow(MYSQL_RES* pRes_);
    int NumRows(MYSQL_RES* pRes_);
    void FreeResult(MYSQL_RES* pRes_);


    // use parameter bind to set parameter and get result
    // 1.prepare
    // 2.bind parameter
    // 3.execute
    // 4.bind_result
    // 5.store result
    // 6.fetch
    // 7.free result
    bool Prepare(
        char* pContent_,
        int nLength_);
    bool BindParameter(MYSQL_BIND* pBinds_);
    bool Execute();
    bool BindResult(MYSQL_BIND* pBinds_);
    bool StoreResult();
    int Fetch();
    void FreeResult();



private:
    char m_strHost[100];
    char m_strUser[100];
    char m_strPassword[100];

    MYSQL *m_pConnection;
    MYSQL_STMT *m_pStmt;

};

#endif // MYSQLAGENT_H
