#include "../../Lib/lib.h"
#include "../../Tcp/lib.h"

typedef std::shared_ptr<TcpClient> TcpClientPtr;
// const int kMaxConns = 1;
const size_t s_nMaxPacketLen = 255;
const size_t s_nHeaderLen = 3;
const uint16_t s_nListenPort = 9999;
const char* socksIp = "127.0.0.1";
const uint16_t s_nSocksPort = 7777;

// 接口与责任：
// TcpClient:
// １．设置客户端属性，如是否支持失败重连，重连次数，．．．
// ２．设置收到来自套接字数据的回调
// －－典型使用
// 对数据进行分析，
// 若数据包含一个完整消息，则取出消息处理
// 若数据不包含一个完整消息，则忽略，等待后续达到一个消息时再在回调时进行处理
// ３．设置客户端连接建立／连接撤销回调
// －－典型使用
// 连接建立时，存储作为参数传入的shared_ptr<TcpConnection>
// 连接销毁时，删除存储的shared_ptr<TcpConnection>
// TcpServer：
// １．设置服务端属性，如是否支持线程池，线程池数目，．．．．
// ２．设置收到来自负责的客户套接字数据的回调
// －－典型使用
// 对数据进行分析，
// 若数据包含一个完整消息，则取出消息处理
// 若数据不包含一个完整消息，则忽略，等待后续达到一个消息时再在回调时进行处理
// ３．设置接收客户端连接／断开客户端连接回调
// －－典型使用
// 连接建立时，存储作为参数传入的shared_ptr<TcpConnection>
// 连接销毁时，删除存储的shared_ptr<TcpConnection>
//
// 这里与TcpClient的３区别是
// 一个TcpClient其至多维护一个shared_ptr<TcpConnection>
// 一个TcpServer对与其连接的每个客户维护一个shared_ptr<TcpConnection>

// TcpServer为了标识每个与其连接的TcpConnection
// 且实现标识与TcpConnection间快速的双向定位，
// 典型做法就是：
// 为每个TcpConnection分配id，以<id, shared_ptr<TcpConnection>>形式用红黑树[map]存储shared_ptr<TcpConnection>
// 对每个TcpConnection将其id设置为对象的语境
// TcpConnection：
// １．设置连接属性
// ２．通过接口给套接字发送数据
// ３．进行连接控制，如关闭连接／．．．
struct Entry
{
  int connId;
  TcpClientPtr client;
  TcpConnectionPtr connection;
  Buffer pending;
};

class DemuxServer 
{
 public:
  DemuxServer(
    EventLoop* loop, 
    const InetAddress& listenAddr, 
    const InetAddress& socksAddr)
    : m_pLoop(loop),
      m_nServer(
        loop, 
        listenAddr, 
        "DemuxServer"),
      m_nSocksAddr(socksAddr)
  {
    m_nServer.setConnectionCallback(
        std::bind(
            &DemuxServer::onServerConnection, 
            this, 
            _1));
    m_nServer.setMessageCallback(
        std::bind(
            &DemuxServer::onServerMessage, 
            this, 
            _1, 
            _2, 
            _3));
  }

  // 服务器启动
  void start()
  {
    m_nServer.start();
  }

  // 作为服务器只接收一个连接［只有一个服务的客户端］
  void onServerConnection(
          const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      if (m_nServerConn)
      {
        conn->shutdown();
      }
      else
      {
        m_nServerConn = conn;
        LOG_INFO 
            << "onServerConnection set m_nServerConn";
      }
    }
    else
    {
      if (m_nServerConn == conn)
      {
        m_nServerConn.reset();
        m_nSocksConns.clear();
        LOG_INFO 
            << "onServerConnection reset serverConn_";
      }
    }
  }

  // 收到来自客户端的消息
  void onServerMessage(
    const TcpConnectionPtr& conn, 
    Buffer* buf, 
    TimeStamp)
  {
    while (buf->readableBytes() > s_nHeaderLen)
    {
      int len = static_cast<uint8_t>(*buf->peek());
      // 内容不足一个完整消息，忽略
      // 待后续累计到一个完整消息，才再回调时处理
      if (buf->readableBytes() < len + s_nHeaderLen)
      {
        break;
      }
      else
      {
          // 按协议提取
        int connId = static_cast<uint8_t>(buf->peek()[1]);
        connId |= (static_cast<uint8_t>(buf->peek()[2]) << 8);
        if (connId != 0)
        {
          assert(m_nSocksConns.find(connId) != m_nSocksConns.end());
          
          TcpConnectionPtr& socksConn = m_nSocksConns[connId].connection;
          
          if (socksConn)
          {
            assert(m_nSocksConns[connId].pending.readableBytes() == 0);
            socksConn->send(
                buf->peek() + s_nHeaderLen, 
                len);
          }
          else
          {
            m_nSocksConns[connId].pending.append(
                buf->peek() + s_nHeaderLen, 
                len);
          }
        }
        else
        {
          // 命令处理
          string cmd(
            buf->peek() + s_nHeaderLen, 
            len);
          doCommand(cmd);
        }

        buf->retrieve(len + s_nHeaderLen);
      }
    }
  }

  void doCommand(const string& cmd)
  {
    static const string s_nConn = "CONN ";
    int connId = atoi(&cmd[s_nConn.size()]);
    bool isUp = cmd.find(" IS UP") != string::npos;
    LOG_INFO 
        << "doCommand " 
        << connId 
        << " " << isUp;
    if (isUp)
    {
      assert(m_nSocksConns.find(connId) == m_nSocksConns.end());
      char connName[256];
      snprintf(
        connName, 
        sizeof connName, 
        "SocksClient %d", 
        connId);
      Entry entry;
      entry.connId = connId;
      // 对命令的处理是
      // 产生一个TcpConnection并发起连接请求
      // 产生一个<id,Entry>组放入map
      entry.client.reset(
        new TcpClient(
            m_pLoop, 
            m_nSocksAddr, 
            connName));
      entry.client->setConnectionCallback(
          std::bind(
              &DemuxServer::onSocksConnection, 
              this, 
              connId, 
              _1));
      entry.client->setMessageCallback(
          std::bind(
              &DemuxServer::onSocksMessage, 
              this, 
              connId, 
              _1, 
              _2, 
              _3));
      m_nSocksConns[connId] = entry;
      entry.client->connect();
    }
    else
    {
        // 对命令的处理是
        // 找到id对应的Entry
        // 对其包含的TcpConnection执行关闭
        // 或不包含有效TcpConnection时，从map按id删除
      assert(m_nSocksConns.find(connId) != m_nSocksConns.end());
      TcpConnectionPtr& socksConn = m_nSocksConns[connId].connection;
      if (socksConn)
      {
        socksConn->shutdown();
      }
      else
      {
        m_nSocksConns.erase(connId);
      }
    }
  }

  void onSocksConnection(
    int connId, 
    const TcpConnectionPtr& conn)
  {
    assert(m_nSocksConns.find(connId) != m_nSocksConns.end());
    // 连接建立－－在对应Entry中存储shared_ptr<TcpConnection>
    // 并通过TcpConnection执行数据发送
    if (conn->connected())
    {
      m_nSocksConns[connId].connection = conn;
      Buffer& pendingData = m_nSocksConns[connId].pending;
      if (pendingData.readableBytes() > 0)
      {
        conn->send(&pendingData);
      }
    }

    else
    {
      if (m_nServerConn)
      {
        char buf[256];
        int len = snprintf(
            buf, 
            sizeof(buf), 
            "DISCONNECT %d\r\n", 
            connId);
        Buffer buffer;
        buffer.append(buf, len);
        sendServerPacket(0, &buffer);
      }
      else
      {
        m_nSocksConns.erase(connId);
      }
    }
  }

  void onSocksMessage(
    int connId, 
    const TcpConnectionPtr& conn, 
    Buffer* buf, 
    TimeStamp)
  {
    assert(m_nSocksConns.find(connId) != m_nSocksConns.end());
    while (buf->readableBytes() > s_nMaxPacketLen)
    {
      Buffer packet;
      packet.append(
        buf->peek(), 
        s_nMaxPacketLen);
      buf->retrieve(s_nMaxPacketLen);
      sendServerPacket(
        connId, 
        &packet);
    }

    if (buf->readableBytes() > 0)
    {
      sendServerPacket(connId, buf);
    }
  }

  void sendServerPacket(
    int connId, 
    Buffer* buf)
  {
    size_t len = buf->readableBytes();
    LOG_DEBUG << len;
    assert(len <= s_nMaxPacketLen);
    uint8_t header[s_nHeaderLen] = 
    {
      static_cast<uint8_t>(len),
      static_cast<uint8_t>(connId & 0xFF),
      static_cast<uint8_t>((connId & 0xFF00) >> 8)
    };
    
    buf->prepend(header, s_nHeaderLen);
    if (m_nServerConn)
    {
      m_nServerConn->send(buf);
    }
  }

private:
  EventLoop* m_pLoop;
  
  TcpServer m_nServer;
  TcpConnectionPtr m_nServerConn;
  
  const InetAddress m_nSocksAddr;
  
  std::map<int, Entry> m_nSocksConns;
};

/*int main(int argc, char* argv[])
{
  LOG_INFO 
      << "pid = " 
      << getpid();
  EventLoop loop;
  InetAddress listenAddr(s_nListenPort);
  if (argc > 1)
  {
    socksIp = argv[1];
  }

  InetAddress socksAddr(
    socksIp,
    s_nSocksPort);

  DemuxServer server(
    &loop, 
    listenAddr, 
    socksAddr);

  server.start();
  loop.loop();
}*/


