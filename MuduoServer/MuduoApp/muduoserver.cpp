//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "muduoserver.h"

MuduoServer::MuduoServer(
    EventLoop* loop,
    const InetAddress& listenAddr,
    char* pStrHost_,
    int nHostLen_,
    char* pStrUser_,
    int nUserLen_,
    char* pStrPassword_,
    int nPasswordLen_)
    : m_nServer(loop, listenAddr, "MuduoServer"),
      m_nSqlAgent(pStrHost_, nHostLen_, pStrUser_, nUserLen_, pStrPassword_, nPasswordLen_)
{
    // set client connection's building and destroying callback
    m_nServer.setConnectionCallback(
        std::bind(
            &MuduoServer::onConnection,
            this,
            _1));
    // set client connection's receiving orginal message callback
    m_nServer.setMessageCallback(
        std::bind(
            &MuduoServer::onMessage,
            this,
            _1,
            _2,
            _3));

    m_nSqlAgent.Connect();
}

void MuduoServer::setThreadNum(int numThreads)
{
    m_nServer.setThreadNum(numThreads);
}

void MuduoServer::start()
{
    m_nServer.setThreadInitCallback(
        std::bind(
            &MuduoServer::threadInit,// 线程池内每个线程会执行
            this,
            _1));
    m_nServer.start();
}

void MuduoServer::threadInit(EventLoop* loop)
{
    // LocalConnections模板类
    // 模板类型T为容器，容器元素是值语义的智能指针
    assert(LocalConnections::pointer() == NULL);
    // 执行结果
    // 针对调用线程产生一个动态容器对象，
    // 调用线程的线程特定数据LocalConnections::m_pValue指向此动态对象
    LocalConnections::instance();
    assert(LocalConnections::pointer() != NULL);
    MutexLockGuard lock(m_nMutex);
    // 一个事件循环背后是一个与其绑定的线程
    // 通过事件循环对象的访问可以获得
    // 其状态，属性，与其交互［也即与其背后所关联的线程交互］
    m_nLoops.insert(loop);
}

// 不论连接回调由服务器端那个线程触发
// 执行的回调函数都是这个
// 且由于成员函数性质，所有线程执行回调时，
// 均可获得一个多个线程共享的回调语境－－－一个共享的MuduoServer对象
void MuduoServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_TRACE
        << conn->localAddress().toIpPort()
        << " -> "
        << conn->peerAddress().toIpPort()
        << " is "
        << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
        // 让调用线程在其自己的特定的动态对象里
        // 存储由其负责的所有TcpConnection对象
        LocalConnections::instance().insert(conn);

        // MutexLockGuard lock(m_nMutex);
        // m_mapConnections.insert(std::pair());
    }
    else
    {
        LocalConnections::instance().erase(conn);

        // MutexLockGuard lock(m_nMutex);
        // ConnectionMapping m_mapConnections;
    }
}

void MuduoServer::onMessage(
    const TcpConnectionPtr& conn,
    Buffer* buf,
    TimeStamp receiveTime)
{
    size_t _nFixLength = sizeof(int32_t);
    while (buf->readableBytes() >= _nFixLength)
    {
        const void* data = buf->peek();
        int32_t be32 = *static_cast<const int32_t*>(data);
        const int32_t len = networkToHost32(be32);
        if (len > 65536
            || len < 0)
        {
            LOG_ERROR
                << "Invalid length "
                << len;
            conn->shutdown();
            break;
        }
        else if (buf->readableBytes() >= (size_t)len)
        {
            buf->retrieve(_nFixLength);

            char _strMessage[len-_nFixLength];
            memcpy(_strMessage, buf->peek(), len - _nFixLength);
            // 这里应用层将协议打包和解码抽离为一个独立的类
            // 用将解码好的消息执行应用层回调
            onStringMessage(
                conn,
                _strMessage,
                len-_nFixLength,
                receiveTime);
            buf->retrieve(len - _nFixLength);
        }
        else
        {
            break;
        }
    }
}

// 传入是
// 原始接收信息－－解码－－解码后一条完整信息
void MuduoServer::onStringMessage(
      const TcpConnectionPtr& pTcpConnection_,
      void* pStrMessage_,
      int nLength_,
      TimeStamp)
{
    Q_UNUSED(nLength_);
    // information format
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for register type
    // the content format
    // 1 byte for user-id length, thus we can process a user-id at most 2^8-1 byte
    // 1 byte for password length, thus we can process a password at most 2^8-1 byte
    const void* data = pStrMessage_;
    int16_t be16 = *static_cast<const int16_t*>(data);
    const int16_t _nType = networkToHost16(be16);
    if(_nType == REGISTER)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for user-id len
        int8_t _nUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strUserId[_nUserIdLen];
        memcpy(_strUserId, _pPos, _nUserIdLen);
        // for password len
        _pPos += _nUserIdLen;
        int8_t _nPasswordLen = *(int8_t*)_pPos;
        // for password
        _pPos += 1;
        char _strPassword[_nPasswordLen];
        memcpy(_strPassword, _pPos, _nPasswordLen);
        // Process Register Use Userid and Password
        ProcessRegister(
            pTcpConnection_,
            _strUserId,
            _nUserIdLen,
            _strPassword,
            _nPasswordLen);
    }
    else if(_nType == LOGIN)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for user-id len
        int8_t _nUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strUserId[_nUserIdLen];
        memcpy(_strUserId, _pPos, _nUserIdLen);
        // for password len
        _pPos += _nUserIdLen;
        int8_t _nPasswordLen = *(int8_t*)_pPos;
        // for password
        _pPos += 1;
        char _strPassword[_nPasswordLen];
        memcpy(_strPassword, _pPos, _nPasswordLen);
        // Process Register Use Userid and Password
        ProcessLogin(
            pTcpConnection_,
            _strUserId,
            _nUserIdLen,
            _strPassword,
            _nPasswordLen);
    }
    else if(_nType == ADD)
    {
        char* _pPos = (char*)data;
        _pPos += 2;

        // for master-user-id len
        int8_t _nMasterUserIdLen = *((int8_t*)_pPos);
        // for master-user-id
        _pPos += 1;
        char _strMasterUserId[_nMasterUserIdLen];
        memcpy(_strMasterUserId, _pPos, _nMasterUserIdLen);
        _pPos += _nMasterUserIdLen;

        // for user-id len
        int8_t _nUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strUserId[_nUserIdLen];
        memcpy(_strUserId, _pPos, _nUserIdLen);
        // Process Add Use Userid
        ProcessAdd(
            pTcpConnection_,
            _strMasterUserId,
            _nMasterUserIdLen,
            _strUserId,
            _nUserIdLen);
    }
    else if(_nType == UPDATE_FRIEND_LIST)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for user-id len
        int8_t _nUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strUserId[_nUserIdLen];
        memcpy(_strUserId, _pPos, _nUserIdLen);
        // Process Update-Friend-List
        ProcessUpdateFriendList(
            pTcpConnection_,
            _strUserId,
            _nUserIdLen);
    }
    else if(_nType == OFF_LINE)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for user-id len
        int8_t _nUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strUserId[_nUserIdLen];
        memcpy(_strUserId, _pPos, _nUserIdLen);

        // for TimeStamp
        _pPos += _nUserIdLen;
        int64_t _nNetTime = *((int64_t*)_pPos);
        const int64_t _nTime = networkToHost64(_nNetTime);
        TimeStamp _nTimeStamp(_nTime);
        // Process Off-Line
        ProcessOffLine(
            pTcpConnection_,
            _strUserId,
            _nUserIdLen,
            _nTimeStamp);
    }
    else if(_nType == NEED_TO_PROCESS_USER_LIST)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for user-id len
        int8_t _nUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strUserId[_nUserIdLen];
        memcpy(_strUserId, _pPos, _nUserIdLen);
        // Process Need-To-Process-User-List
        ProcessNeedToProcessUserList(
            pTcpConnection_,
            _strUserId,
            _nUserIdLen);
    }
    else if(_nType == CHAT)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for sender
        // for user-id len
        int8_t _nSenderUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strSenderUserId[_nSenderUserIdLen];
        memcpy(_strSenderUserId, _pPos, _nSenderUserIdLen);

        _pPos += _nSenderUserIdLen;
        // for receiver
        // for user-id len
        int8_t _nReceiverUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strReceiverUserId[_nReceiverUserIdLen];
        memcpy(_strReceiverUserId, _pPos, _nReceiverUserIdLen);

        _pPos += _nReceiverUserIdLen;
        // for chat-content
        int16_t _nNetChatLen = *((int16_t*)(_pPos));
        const int16_t _nChatLen = networkToHost16(_nNetChatLen);
        // for chat-content
        _pPos += 2;
        char _strChatContent[_nChatLen];
        memcpy(_strChatContent, _pPos, _nChatLen);

        // Process Chat-Message
        ProcessChatMessage(
            pTcpConnection_,
            _strSenderUserId,
            _nSenderUserIdLen,
            _strReceiverUserId,
            _nReceiverUserIdLen,
            _strChatContent,
            _nChatLen);
    }
    else if(_nType == GET_NEED_TO_PROCESS_MESSAGES)
    {
        char* _pPos = (char*)data;
        _pPos += 2;
        // for sender
        // for user-id len
        int8_t _nSenderUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strSenderUserId[_nSenderUserIdLen];
        memcpy(_strSenderUserId, _pPos, _nSenderUserIdLen);

        _pPos += _nSenderUserIdLen;
        // for receiver
        // for user-id len
        int8_t _nReceiverUserIdLen = *((int8_t*)_pPos);
        // for user-id
        _pPos += 1;
        char _strReceiverUserId[_nReceiverUserIdLen];
        memcpy(_strReceiverUserId, _pPos, _nReceiverUserIdLen);

        // Process Chat-Message
        ProcessGetNeedToProcessMessagesMessage(
            pTcpConnection_,
            _strSenderUserId,
            _nSenderUserIdLen,
            _strReceiverUserId,
            _nReceiverUserIdLen);
    }
}

bool MuduoServer::ProcessOffLine(
    const TcpConnectionPtr& pTcpConnection_,
    char* pUserId_,
    int nUserIdLen_,
    const TimeStamp& nTimeStamp_)
{
    Q_UNUSED(pTcpConnection_);
    char _strUserId[nUserIdLen_ + 1];
    memset(_strUserId, 0, sizeof(_strUserId));
    memcpy(_strUserId, pUserId_, nUserIdLen_);

    // as a server
    // 1.we recever information sended from client
    // 2.we parse the information
    // we should identify this as a off-line.
    // then we get the userid and login-time.
    m_nMutexForMapping.lock();

    ComplexConnectionsMapping::const_iterator _iter = m_mapComplexConnectionsMapping.find(QByteArray(_strUserId));
    if(_iter != m_mapComplexConnectionsMapping.end())
    {
        ComplexConnectionsPtr _nPtr = _iter->second;
        if(_nPtr.get())
        {
            int i = 0;
            for(; i < _nPtr->GetSize(); i++)
            {
                std::pair<TimeStamp, TcpConnectionWeakPtr> _nPair = _nPtr->operator[](i);
                if(_nPair.first == nTimeStamp_)
                {
                    break;
                }
            }

            assert(i >= 0 && i < _nPtr->GetSize());
            _nPtr->DeleteByIndex(i);
        }
    }

    m_nMutexForMapping.unlock();
    return true;
}

// how to process get need-to-process-messages
// the server will tell us all the messages
// and set the messages's flag to processed
// and delete the user from needtoprocess table
//
bool MuduoServer::ProcessGetNeedToProcessMessagesMessage(
    const TcpConnectionPtr& pTcpConnection_,
    char* pSenderUserId_,
    int nSenderUserIdLen_,
    char* pReceiverUserId_,
    int nReceiverUserIdLen_)
{
    char _strSender[100];
    memset(_strSender, 0, sizeof(_strSender));
    memcpy(_strSender, pSenderUserId_, nSenderUserIdLen_);
    char _strReceiver[100];
    memset(_strReceiver, 0, sizeof(_strReceiver));
    memcpy(_strReceiver, pReceiverUserId_, nReceiverUserIdLen_);

    // 1.there should have a database name as chatmessage
    bool _bRet = m_nSqlAgent.Query("use chatmessage");
    assert(_bRet);

    // 2.supposing that the sender's user-id is a ,the receiver's user-id is b
    // and a is less than b in sorting.
    // if the table named as a_b not exist,we create it
    // and add the message into the table
    int _nRet = strcmp(_strSender, _strReceiver);
    char _strTableName[201];
    //bool _bDirection;
    memset(_strTableName, 0, sizeof(_strTableName));
    // tablename:a_b
    if(_nRet <= 0)
    {
        snprintf(_strTableName, sizeof(_strTableName), "%s_%s", _strSender, _strReceiver);
        // a to b
        //_bDirection = true;
    }
    else
    {
        snprintf(_strTableName, sizeof(_strTableName), "%s_%s", _strReceiver, _strSender);
        // b to a
        //_bDirection = false;
    }

    char _strSql[300];
    memset(_strSql, 0, sizeof(_strSql));
    // create a_b
    // for direction
    // 1 is a to b
    // -1 is b to a
    // for flag
    // 0 is not-processed
    // 1 is processed
    snprintf(
        _strSql,
        sizeof(_strSql),
        "create table if not exists %s (direction int, message varchar(1024), time timestamp, flag int)",
        _strTableName);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "select * from %s where flag = 0 order by time",
        _strTableName);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    // get-result
    MYSQL_RES* _pRes = nullptr;
    _bRet = m_nSqlAgent.StoreResult(&_pRes);
    assert(_bRet);
    // 3.process result
    NDataStruct::DynArray<Message> _arrMessages;
    while(true)
    {
        MYSQL_ROW _nRow = m_nSqlAgent.FetchRow(_pRes);
        if(_nRow == nullptr)
        {
            break;
        }

        assert(_nRow);
        // direction int, message varchar(1024), time timestamp, flag int
        int _nDirection = QByteArray(_nRow[0]).toInt();
        // _nDirection
        // for table a_b
        // -1
        // b to a
        // 1
        // a to b

        // the direction for source and dest
        // if source <= dest then
        // neednot change
        // if source > dest
        // should change
        int _nCmp = strcmp(_strSender, _strReceiver);
        if(_nCmp > 0)
        {
            _nDirection = -_nDirection;
        }

        //
        QByteArray _nMessage(_nRow[1]);
        //
        QByteArray _nTime(_nRow[2]);
        // int64_t _nTimeLL = _nTime.toLongLong();
        // timestamp show format:yyyy-mm-dd hh:mm:ss
        struct tm _nTm;
        sscanf(_nRow[2], "%d-%d-%d %d:%d:%d",
            &(_nTm.tm_year),
            &(_nTm.tm_mon),
            &(_nTm.tm_mday),
            &(_nTm.tm_hour),
            &(_nTm.tm_min),
            &(_nTm.tm_sec));
        time_t _nTs = mktime(&_nTm);
        int64_t _nTslonglong = TimeStamp::s_nMicroSecondsPerSecond * _nTs;
        Message _nMess;
        _nMess.m_bDirection = _nDirection == 1;
        _nMess.m_nContent = _nMessage;
        _nMess.m_nTimeStamp = TimeStamp(_nTslonglong);
        _arrMessages.Add(_nMess);
    }

    // set all processed message's flag to processed
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "update %s set flag = 1 where flag = 0",
        _strTableName);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    ResForGetNeedToProcessMessagesMessage(pTcpConnection_, _arrMessages);

    // free result
    m_nSqlAgent.FreeResult(_pRes);

    // delete dest from source's needtoprocessfriends table
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(_strSql, sizeof(_strSql), "use %s", _strSender);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "delete from needtoprocessfriends where id = \"%s\"",
        _strReceiver);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    // send the messages to client
    return true;
}

bool MuduoServer::ProcessChatMessage(
    const TcpConnectionPtr& pTcpConnection_,
    char* pSenderUserId_,
    int nSenderUserIdLen_,
    char* pReceiverUserId_,
    int nReceiverUserIdLen_,
    char* pChatContent_,
    int nChatContentLen_)
{
    ResForChatMessage(pTcpConnection_);
    char _strSender[100];
    memset(_strSender, 0, sizeof(_strSender));
    memcpy(_strSender, pSenderUserId_, nSenderUserIdLen_);
    char _strReceiver[100];
    memset(_strReceiver, 0, sizeof(_strReceiver));
    memcpy(_strReceiver, pReceiverUserId_, nReceiverUserIdLen_);
    char _strChatContent[nChatContentLen_ + 1];
    memset(_strChatContent, 0, sizeof(_strChatContent));
    memcpy(_strChatContent, pChatContent_, nChatContentLen_);
    // b.1. server record this message in the table of ab.


    // 1.there should have a database name as chatmessage
    bool _bRet = m_nSqlAgent.Query("create database if not exists chatmessage");
    assert(_bRet);

    _bRet = m_nSqlAgent.Query("use chatmessage");
    assert(_bRet);

    // 2.supposing that the sender's user-id is a ,the receiver's user-id is b
    // and a is less than b in sorting.
    // if the table named as a_b not exist,we create it
    // and add the message into the table
    int _nRet = strcmp(_strSender, _strReceiver);
    char _strTableName[201];
    bool _bDirection;
    memset(_strTableName, 0, sizeof(_strTableName));
    // tablename:a_b
    if(_nRet <= 0)
    {
        snprintf(_strTableName, sizeof(_strTableName), "%s_%s", _strSender, _strReceiver);
        // a to b
        _bDirection = true;
    }
    else
    {
        snprintf(_strTableName, sizeof(_strTableName), "%s_%s", _strReceiver, _strSender);
        // b to a
        _bDirection = false;
    }

    char _strSql[300];
    memset(_strSql, 0, sizeof(_strSql));
    // create a_b
    // for direction
    // 1 is a to b
    // -1 is b to a
    // for flag
    // 0 is not-processed
    // 1 is processed
    snprintf(
        _strSql,
        sizeof(_strSql),
        "create table if not exists %s (direction int, message varchar(1024), time timestamp, flag int)",
        _strTableName);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "insert into %s values(%d, \"%s\", now(), 0)",
        _strTableName,
        _bDirection ? 1 : -1,
        _strChatContent);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    // for every record message, recoring its content, receiving time, processing flag.
    // b.2. server add a record in needtoprocess-user table in database b
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(_strSql, sizeof(_strSql), "use %s", _strReceiver);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "insert into needtoprocessfriends values(\"%s\")",
        _strSender);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    // b.3. if b is off-line, process end.
    //m_nMutex.lock();
    m_nMutexForMapping.lock();
    ComplexConnectionsMapping::const_iterator _iter = m_mapComplexConnectionsMapping.find(QByteArray(_strReceiver));
    if(_iter == m_mapComplexConnectionsMapping.end())
    {
        // b is off-line
        // do nothing
    }
    else
    {
        // if b is on-line
        // send him a notification to tell him having a message to read
        ComplexConnectionsPtr _nPtr = _iter->second;
        if(_nPtr.get())
        {
            // delete expired
            while(true)
            {
                int i = 0;
                for(; i < _nPtr->GetSize(); i++)
                {
                    std::pair<TimeStamp, TcpConnectionWeakPtr> _nPair = _nPtr->operator[](i);
                    if(_nPair.second.expired())
                    {
                        break;
                    }
                }

                if(i >= 0 && i < _nPtr->GetSize())
                {
                    _nPtr->DeleteByIndex(i);
                }
                else
                {
                    break;
                }
            }

            for(int i = 0; i < _nPtr->GetSize(); i++)
            {
                std::pair<TimeStamp, TcpConnectionWeakPtr> _nPair = _nPtr->operator[](i);
                TcpConnectionPtr _nTcpConnectionPtr = _nPair.second.lock();
                ResForNewMessageNotification(_nTcpConnectionPtr, QByteArray(_strSender));
            }
        }
    }

    m_nMutexForMapping.unlock();
    return true;
    // b.4. if b is on-line, send b a notification to telling it there is a message needed to process.
}

bool MuduoServer::ProcessNeedToProcessUserList(
    const TcpConnectionPtr& pTcpConnection_,
    char* pUserId_,
    int nUserIdLen_)
{
    char _strUserId[nUserIdLen_ + 1];
    memset(_strUserId, 0, sizeof(_strUserId));
    memcpy(_strUserId, pUserId_, nUserIdLen_);

    // as a server
    // 1.we recever information sended from client
    // 2.we parse the information
    // we should identify this as a need-to-process-user-list-info.
    // then we get the userid from the need-to-process-user-list-info.
    // 3.we as a client run a query to a database server and recever the process result
    // we should process the result
    // to get the need-to-process-user list for the specific user
    // then we should send back to the client to tell it this information

    // every user should have a database named as the user's id

    // 1.use user-id;
    char _strSql[100];
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "use %s",
        _strUserId);
    bool _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    // 2.select id from needtoprocessusers;
    _bRet = m_nSqlAgent.Query("select id from needtoprocessfriends");
    assert(_bRet);

    MYSQL_RES* _pRes = nullptr;
    _bRet = m_nSqlAgent.StoreResult(&_pRes);
    assert(_bRet);
    // 3.process result
    NDataStruct::DynArray<QByteArray> _arrUserIds;
    while(true)
    {
        MYSQL_ROW _nRow = m_nSqlAgent.FetchRow(_pRes);
        if(_nRow == nullptr)
        {
            break;
        }

        assert(_nRow);
        QByteArray _nUserId(_nRow[0]);
        _arrUserIds.Add(_nUserId);
    }

    ResForNeedToProcessUserList(pTcpConnection_, _arrUserIds);

    // free result
    m_nSqlAgent.FreeResult(_pRes);

    return true;
}

bool MuduoServer::ProcessUpdateFriendList(
    const TcpConnectionPtr& pTcpConnection_,
    char* pUserId_,
    int nUserIdLen_)
{
    char _strUserId[nUserIdLen_ + 1];
    memset(_strUserId, 0, sizeof(_strUserId));
    memcpy(_strUserId, pUserId_, nUserIdLen_);

    // as a server
    // 1.we recever information sended from client
    // 2.we parse the information
    // we should identify this as a update-friend-list-info.
    // then we get the userid from the update-friend-list-info.
    // 3.we as a client run a query to a database server and recever the process result
    // we should process the result
    // to get the friend list for the specific user
    // then we should send back to the client to tell it this information

    // every user should have a database named as the user's id

    // 1.use user-id;
    char _strSql[100];
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "use %s",
        _strUserId);
    bool _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    // 2.select id from friends;
    _bRet = m_nSqlAgent.Query("select id from friends");
    assert(_bRet);

    MYSQL_RES* _pRes = nullptr;
    _bRet = m_nSqlAgent.StoreResult(&_pRes);
    assert(_bRet);
    // 3.process result
    NDataStruct::DynArray<QByteArray> _arrUserIds;
    while(true)
    {
        MYSQL_ROW _nRow = m_nSqlAgent.FetchRow(_pRes);
        if(_nRow == nullptr)
        {
            break;
        }

        assert(_nRow);
        QByteArray _nUserId(_nRow[0]);
        _arrUserIds.Add(_nUserId);
    }

    ResForUpdateFriendList(pTcpConnection_, _arrUserIds);

    // free result
    m_nSqlAgent.FreeResult(_pRes);

    return true;
}


// when add, we need to know who add who.
// when login success, at server we should build a bind for user-id and the connection
bool MuduoServer::ProcessAdd(
    const TcpConnectionPtr& pTcpConnection_,
    char* pMasterUserId_,
    int nMasterUserIdLen_,
    char* pUserId_,
    int nUserIdLen_)
{
    char _strMasterUserId[nMasterUserIdLen_ + 1];
    memset(_strMasterUserId, 0, sizeof(_strMasterUserId));
    memcpy(_strMasterUserId, pMasterUserId_, nMasterUserIdLen_);

    char _strUserId[nUserIdLen_ + 1];
    memset(_strUserId, 0, sizeof(_strUserId));
    memcpy(_strUserId, pUserId_, nUserIdLen_);

    // as a server
    // 1.we recever information sended from client
    // 2.we parse the information
    // we should identify this as a add-info.
    // then we get the userid from the add-info.
    // 3.we as a client run a query to a database server and recever the process result
    // we should process the result to identify if the added user-info has exist or not
    // if the added user-info has existed ,
    // then we should send back to the client to tell it this information.
    // if the added user-info not has existed,
    // then we should send back to the client to tell it this information

    // every user should have a database named as the user's id

    // 1.use global;
    bool _bRet = m_nSqlAgent.Query("use global");
    assert(_bRet);

    // 2.select count(*) from user where id="xxxx";
    char _strSql[200];
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "select count(*) from user where id = \"%s\"",
        _strUserId);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    MYSQL_RES* _pRes = nullptr;
    _bRet = m_nSqlAgent.StoreResult(&_pRes);
    assert(_bRet);
    // 3.process result
    MYSQL_ROW _nRow = m_nSqlAgent.FetchRow(_pRes);

    assert(_nRow);
    QByteArray _nCount(_nRow[0]);

    int _nNum = _nCount.toInt(&_bRet);
    assert(_bRet);
    if(_nNum == 0)
    {
        // if result is 0,then
        // we report add fail to client
        ResForAdd(pTcpConnection_, 0);
    }
    else if(_nNum == 1)
    {
        // if result is 1,then
        // 0. create database userid if not exist;
        char _strSql[100];
        memset(_strSql, 0, sizeof(_strSql));
        //snprintf(
        //    _strSql,
        //    sizeof(_strSql),
        //    "create database if not exists %s",
        //    _strUserId);
        //_bRet = m_nSqlAgent.Query(_strSql);
        //assert(_bRet);

        // 1. use userid;
        memset(_strSql, 0, sizeof(_strSql));
        snprintf(
            _strSql,
            sizeof(_strSql),
            "use %s",
            _strMasterUserId);
        _bRet = m_nSqlAgent.Query(_strSql);
        assert(_bRet);

        // 2.  friends
        //memset(_strSql, 0, sizeof(_strSql));
        //snprintf(
        //    _strSql,
        //    sizeof(_strSql),
        //    "create table if not exist friends (id varchar not null unique)");
        //_bRet = m_nSqlAgent.Query(_strSql);
        //assert(_bRet);

        // 3. insert into friends
        memset(_strSql, 0, sizeof(_strSql));
        snprintf(
            _strSql,
            sizeof(_strSql),
            "insert into friends values (\"%s\")",
            _strUserId);
        _bRet = m_nSqlAgent.Query(_strSql);
        assert(_bRet);

        // if result is 1,
        // we report add success to client
        ResForAdd(pTcpConnection_, 1);
    }
    else
    {
        // report to client there is a exception
        ResForAdd(pTcpConnection_, 2);
    }

    // free result
    m_nSqlAgent.FreeResult(_pRes);

    return true;
}

bool MuduoServer::ProcessLogin(
    const TcpConnectionPtr& pTcpConnection_,
    char* pUserId_,
    int nUserIdLen_,
    char* pPassword_,
    int nPasswordLen_)
{
    char _strUserId[nUserIdLen_ + 1];
    memset(_strUserId, 0, sizeof(_strUserId));
    memcpy(_strUserId, pUserId_, nUserIdLen_);

    char _strPassword[nPasswordLen_ + 1];
    memset(_strPassword, 0, sizeof(_strPassword));
    memcpy(_strPassword, pPassword_, nPasswordLen_);

    // as a server
    // 1.we recever information sended from client
    // 2.we parse the information
    // we should identify this as a register-info.
    // then we get the userid and password from the register-info.
    // 3.we as a client run a query to a database server and recever the process result
    // we should process the result to identify if the login-info has exist or not
    // if the register-info has existed ,
    // then we should send back to the client to tell it this information.
    // if the register-info not has existed,
    // then we should send back to the client to tell it this information

    // 1.use global;
    bool _bRet = m_nSqlAgent.Query("use global");
    assert(_bRet);

    // 2.select count(*) from user where id="xxxx" and password = "xxxx";
    char _strSql[200];
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "select count(*) from user where id = \"%s\" and password = \"%s\"",
        _strUserId,
        _strPassword);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    MYSQL_RES* _pRes = nullptr;
    _bRet = m_nSqlAgent.StoreResult(&_pRes);
    assert(_bRet);
    // 3.process result
    MYSQL_ROW _nRow = m_nSqlAgent.FetchRow(_pRes);

    assert(_nRow);
    QByteArray _nCount(_nRow[0]);

    int _nNum = _nCount.toInt(&_bRet);
    assert(_bRet);
    if(_nNum == 0)
    {
        // if result is 0,then
        // we report login fail to client
        ResForLogin(pTcpConnection_, 0);

    }
    else if(_nNum == 1)
    {
        // for every logined user,
        // build a mapping with the user-id to the tcp-connection object.
        m_nMutexForMapping.lock();
        // std::pair<QByteArray, TcpConnectionWeakPtr> _nPair(QByteArray(_strUserId), pTcpConnection_);
        // m_mapConnections.insert(_nPair);
        ComplexConnectionsMapping::const_iterator _iter = m_mapComplexConnectionsMapping.find(QByteArray(_strUserId));
        // we allow one user has multi-login instances.
        // we use timestamp to identify the instances.
        //
        TimeStamp _nNow = TimeStamp::now();
        if(_iter == m_mapComplexConnectionsMapping.end())
        {
            ComplexConnectionsPtr _nComplexConnsPtr(new ComplexConnections());
            _nComplexConnsPtr->Add(std::pair<TimeStamp, TcpConnectionWeakPtr>(_nNow, pTcpConnection_));
            m_mapComplexConnectionsMapping.insert(std::pair<QByteArray, ComplexConnectionsPtr>(QByteArray(_strUserId), _nComplexConnsPtr));
        }
        else
        {
            _iter->second->Add(std::pair<TimeStamp, TcpConnectionWeakPtr>(_nNow, pTcpConnection_));
        }

        m_nMutexForMapping.unlock();

        // if result is 1,
        // we report login success to client
        ResForLogin(pTcpConnection_, 1, _nNow);
    }
    else
    {
        // report to client there is a exception
        ResForLogin(pTcpConnection_, 2);
    }

    // free result
    m_nSqlAgent.FreeResult(_pRes);
    return true;
}

bool MuduoServer::ProcessRegister(
    const TcpConnectionPtr& pTcpConnection_,
    char* pUserId_,
    int nUserIdLen_,
    char* pPassword_,
    int nPasswordLen_)
{
    char _strUserId[nUserIdLen_ + 1];
    memset(_strUserId, 0, sizeof(_strUserId));
    memcpy(_strUserId, pUserId_, nUserIdLen_);

    char _strPassword[nPasswordLen_ + 1];
    memset(_strPassword, 0, sizeof(_strPassword));
    memcpy(_strPassword, pPassword_, nPasswordLen_);

    // as a server
    // 1.we recever information sended from client
    // 2.we parse the information
    // we should identify this as a register-info.
    // then we get the userid and password from the register-info.
    // 3.we as a client run a query to a database server and recever the process result
    // we should process the result to identify if the register-info has exist or not legal
    // if the register-info has existed or is not legal,
    // then we should send back to the client to tell it this information.
    // if the register-info is legal and not has existed,
    // then we run a sql to add the register-info into the user table
    // and send back to the client to tell it this information

    // 1.use global;
    bool _bRet = m_nSqlAgent.Query("use global");
    assert(_bRet);

    // 2.select count(*) from user where id="xxxx";
    char _strSql[100];
    memset(_strSql, 0, sizeof(_strSql));
    snprintf(
        _strSql,
        sizeof(_strSql),
        "select count(*) from user where id = \"%s\"",
        _strUserId);
    _bRet = m_nSqlAgent.Query(_strSql);
    assert(_bRet);

    MYSQL_RES* _pRes = nullptr;
    _bRet = m_nSqlAgent.StoreResult(&_pRes);
    assert(_bRet);
    // 3.process result
    MYSQL_ROW _nRow = m_nSqlAgent.FetchRow(_pRes);

    assert(_nRow);
    QByteArray _nCount(_nRow[0]);

    int _nNum = _nCount.toInt(&_bRet);
    assert(_bRet);
    if(_nNum == 0)
    {
        // if result is 0,then

        // 1.
        // insert into user values ("xxxx", "xxxx")
        // and report register success to client
        char _strSql[100];
        memset(_strSql, 0, sizeof(_strSql));
        snprintf(
            _strSql,
            sizeof(_strSql),
            "insert into user values (\"%s\", \"%s\");",
            _strUserId,
            _strPassword);
        _bRet = m_nSqlAgent.Query(_strSql);
        assert(_bRet);

        // 2. create database user-id
        memset(_strSql, 0, sizeof(_strSql));
        snprintf(
            _strSql,
            sizeof(_strSql),
            "create database %s",
            _strUserId);
        _bRet = m_nSqlAgent.Query(_strSql);
        assert(_bRet);

        // 3. use user-id
        memset(_strSql, 0, sizeof(_strSql));
        snprintf(
            _strSql,
            sizeof(_strSql),
            "use %s",
            _strUserId);
        _bRet = m_nSqlAgent.Query(_strSql);
        assert(_bRet);

        // 4. create table friends (id var(100));
        _bRet = m_nSqlAgent.Query("create table friends (id varchar(100))");
        assert(_bRet);

        // 5. create table needtoprocessfriends(id var(100))
        _bRet = m_nSqlAgent.Query("create table needtoprocessfriends (id varchar(100))");
        assert(_bRet);

        ResForRegister(pTcpConnection_, 0);
        //pTcpConnection_.get()->send()
    }
    else if(_nNum == 1)
    {
        // if result is 1,then register error because id has existed
        ResForRegister(pTcpConnection_, 1);
    }
    else
    {
        // report to client there is a exception
        ResForRegister(pTcpConnection_, 2);
    }

    // free result
    m_nSqlAgent.FreeResult(_pRes);

    return true;
}

bool MuduoServer::PackRegisterResMessage(
    int nRetCode_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for register-res type
    // the content format
    // 1 byte for res-content

    int _nLength = 4 + 2 + 1;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = REGISTER_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for register-res type
    int8_t _nRegisterResType = nRetCode_;
    memcpy(_pPos, &_nRegisterResType, 1);
    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForGetNeedToProcessMessagesMessage(
    const TcpConnectionPtr& pTcpConnection_,
    const NDataStruct::DynArray<Message>& arrMessages_)
{
    Buffer _nBuf;
    bool _bRet = PackGetNeedToProcessMessagesMessage(arrMessages_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

bool MuduoServer::PackGetNeedToProcessMessagesMessage(
    const NDataStruct::DynArray<Message>& arrMessages_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for get-need-to-process-messages-res
    // the content format
    // 2 byte for need-to-process-messages number
    // for every message
    // 1 byte for record its direction
    // 2 byte for message length, followed by message content
    // 8 byte for timestamp

    int _nLength = 4 + 2;
    int _nVarLen = 2;
    for(int i = 0; i < arrMessages_.GetSize(); i++)
    {
        _nVarLen += 1;
        _nVarLen += arrMessages_[i].m_nContent.length();
        _nVarLen += 8;
    }

    _nLength += _nVarLen;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = GET_NEED_TO_PROCESS_MESSAGES_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // 2 byte for need-to-process-messages number
    int16_t _nNum = arrMessages_.GetSize();
    int16_t _nNetNum = hostToNetwork16(_nNum);
    memcpy(_pPos, &_nNetNum, 2);

    _pPos += 2;
    // for every message
    for(int i = 0; i < arrMessages_.GetSize(); i++)
    {
        // 1 byte for record its direction
        int8_t _nDirection = arrMessages_[i].m_bDirection ? 1 : -1;
        memcpy(_pPos, &_nDirection, 1);
        _pPos += 1;
        // 2 byte for message length, followed by message content
        int16_t _nLen = arrMessages_[i].m_nContent.length();
        int16_t _nNetLen = hostToNetwork16(_nLen);
        memcpy(_pPos, &_nNetLen, 2);
        _pPos += 2;
        // message content
        memcpy(_pPos, arrMessages_[i].m_nContent.data(), _nLen);
        _pPos += _nLen;
        // 8 byte for timestamp
        int64_t _nTs = arrMessages_[i].m_nTimeStamp.microSecondsSinceEpoch();
        int64_t _nNetTs = hostToNetwork64(_nTs);
        memcpy(_pPos, &_nNetTs, 8);
        _pPos += 8;
    }

    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForChatMessage(
    const TcpConnectionPtr& pTcpConnection_)
{
    Buffer _nBuf;
    bool _bRet = PackChatResMessage(_nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

bool MuduoServer::PackChatResMessage(
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for chatres type
    // the content format
    // 1 byte for ret-value
    int _nLength = 4 + 2 + 1;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = CHAT_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for ret
    int8_t _nRet = 0;
    memcpy(_pPos, &_nRet, 1);

    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}


bool MuduoServer::ResForNewMessageNotification(
    const TcpConnectionPtr& pTcpConnection_,
    const QByteArray& nByteSender_)
{
    Buffer _nBuf;
    bool _bRet = PackNewResMessageNotification(nByteSender_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

bool MuduoServer::PackNewResMessageNotification(
    const QByteArray& nByteSender_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for chatres/new-message-notification type
    // the content format
    // 1 byte for sender, thus the max length of sender is 2^8-1
    int _nLen1 = nByteSender_.length();
    int _nLength = 4 + 2 + 1 + _nLen1;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = NEW_MESSAGE_NOTIFICATION;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for sender
    memcpy(_pPos, &_nLen1, 1);
    _pPos += 1;
    memcpy(_pPos, nByteSender_.data(), _nLen1);

    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForNeedToProcessUserList(
    const TcpConnectionPtr& pTcpConnection_,
    const NDataStruct::DynArray<QByteArray>& arrUserIds_)
{
    Buffer _nBuf;
    bool _bRet = PackNeedToProcessUserListResMessage(arrUserIds_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}


bool MuduoServer::PackNeedToProcessUserListResMessage(
    const NDataStruct::DynArray<QByteArray>& arrUserIds_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for need-to-process-user-list-res type
    // the content format
    // 1 byte for friends-num, thus the max number of friends we support is 2^8-1
    // for every friend,
    // use 1 byte to record the friend's user-id length, then record its user-id
    int _nLength = 4 + 2;
    int _nVarLength = 1;
    assert(arrUserIds_.GetSize() < 255);
    for(int i = 0; i < arrUserIds_.GetSize(); i++)
    {
        _nVarLength += 1 + arrUserIds_[i].length();
    }

    _nLength += _nVarLength;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = NEED_TO_PROCESS_USER_LIST_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for need-to-process-user-list-res
    int8_t _nFriendNum = arrUserIds_.GetSize();
    memcpy(_pPos, &_nFriendNum, 1);
    _pPos += 1;
    for(int i = 0; i < arrUserIds_.GetSize(); i++)
    {
        // for user-id length
        int8_t _nUserIdLen = arrUserIds_[i].length();
        memcpy(_pPos, &_nUserIdLen, 1);
        // for user-id content
        _pPos += 1;
        memcpy(_pPos, arrUserIds_[i].data(), arrUserIds_[i].length());
        _pPos += arrUserIds_[i].length();
    }

    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForUpdateFriendList(
    const TcpConnectionPtr& pTcpConnection_,
    const NDataStruct::DynArray<QByteArray>& arrUserIds_)
{
    Buffer _nBuf;
    bool _bRet = PackUpdateFriendListResMessage(arrUserIds_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

bool MuduoServer::PackUpdateFriendListResMessage(
    const NDataStruct::DynArray<QByteArray>& arrUserIds_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for update-friend-list-res type
    // the content format
    // 1 byte for friends-num, thus the max number of friends we support is 2^8-1
    // for every friend,
    // use 1 byte to record the friend's user-id length, then record its user-id
    int _nLength = 4 + 2;
    int _nVarLength = 1;
    assert(arrUserIds_.GetSize() < 255);
    for(int i = 0; i < arrUserIds_.GetSize(); i++)
    {
        _nVarLength += 1 + arrUserIds_[i].length();
    }

    _nLength += _nVarLength;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = UPDATE_FRIEND_LIST_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for friend-number-res
    int8_t _nFriendNum = arrUserIds_.GetSize();
    memcpy(_pPos, &_nFriendNum, 1);
    _pPos += 1;
    for(int i = 0; i < arrUserIds_.GetSize(); i++)
    {
        // for user-id length
        int8_t _nUserIdLen = arrUserIds_[i].length();
        memcpy(_pPos, &_nUserIdLen, 1);
        // for user-id content
        _pPos += 1;
        memcpy(_pPos, arrUserIds_[i].data(), arrUserIds_[i].length());
        _pPos += arrUserIds_[i].length();
    }

    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForAdd(
    const TcpConnectionPtr& pTcpConnection_,
    int nRetCode_)
{
    Buffer _nBuf;
    bool _bRet = PackAddResMessage(nRetCode_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

bool MuduoServer::PackAddResMessage(
    int nRetCode_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for add-res type
    // the content format
    // 1 byte for add-content

    int _nLength = 4 + 2 + 1;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = ADD_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for add-res type
    int8_t _nAddResType = nRetCode_;
    memcpy(_pPos, &_nAddResType, 1);
    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForLogin(
    const TcpConnectionPtr& pTcpConnection_,
    int nRetCode_,
    TimeStamp nTimeStamp_)
{
    Buffer _nBuf;
    bool _bRet = PackLoginResMessage(nRetCode_, nTimeStamp_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

bool MuduoServer::PackLoginResMessage(
    int nRetCode_,
    TimeStamp nTimeStamp_,
    Buffer& nBuf_)
{
    // pack and send to client
    // information format
    // 4 byte for length, thus we can send a message at most 2^32-1 byte
    // 2 byte for type, thus we can process 2^16-1 different types.
    // for login-res type
    // the content format
    // 1 byte for res-content
    // 8 byte for timestamp

    int _nLength = 4 + 2 + 1 + 8;
    char _strMessage[_nLength - 4];
    memset(_strMessage, 0, sizeof(_strMessage));
    // for type
    int16_t _nType = LOGIN_RES;
    int16_t _nNetType = hostToNetwork16(_nType);
    char* _pPos = _strMessage;
    memcpy(_pPos, &_nNetType, 2);

    _pPos += 2;
    // for login-res type
    int8_t _nLoginResType = nRetCode_;
    memcpy(_pPos, &_nLoginResType, 1);

    _pPos += 1;
    // for timestamp
    int64_t _nTime = nTimeStamp_.microSecondsSinceEpoch();
    int64_t _nNetTime = hostToNetwork64(_nTime);
    memcpy(_pPos, &_nNetTime, 8);

    nBuf_.append(
        _strMessage,
        _nLength-4);

    int32_t len = static_cast<int32_t>(_nLength);
    int32_t be32 = hostToNetwork32(len);
    nBuf_.prepend(
        &be32,
        sizeof be32);
    return true;
}

bool MuduoServer::ResForRegister(
    const TcpConnectionPtr& pTcpConnection_,
    int nRetCode_)
{
    Buffer _nBuf;
    bool _bRet = PackRegisterResMessage(nRetCode_, _nBuf);
    if(!_bRet)
    {
        return _bRet;
    }

    //MutexLockGuard lock(m_nMutex);
    assert(pTcpConnection_);
    if (pTcpConnection_)
    {
        pTcpConnection_.get()->send(&_nBuf);
    }
    else
    {
        return false;
    }

    return true;
}

// 服务器的每个线程都执行此回调
void MuduoServer::distributeMessage(
      const string& message)
{
    //LOG_DEBUG << "begin";
    // 每个线程对所有由自己负责的TcpConnection执行
    // 原始信息－－信息编码－－信息发送
    //for (ConnectionList::iterator it = LocalConnections::instance().begin();
    //    it != LocalConnections::instance().end();
    //    ++it)
    //{
    //    m_nCodec.send(
    //        get_pointer(*it),
    //        message);
    //}

    //LOG_DEBUG << "end";
}
