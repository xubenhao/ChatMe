#ifndef MUDUO_APP_CODEC_H
#define MUDUO_APP_CODEC_H

#include "header.h"

// 消息编码格式：
// ４字节长度［长度指的是数据部分的长度］＋数据本身
// 消息解码：
// 先取出４字节
// 在取出该长度表示的数据部分
//
//
// 直接通过Tcp传递数值类型时，
// 需要
// 传递前将本机数值转换为网络字节序数值再写入
// 接收时，先取出数据内容，再将其转化为本机字节序再实际使用
//
// 如果传递的是字符串，
// 不需要上述处理
//
//
// Tcp的消息编码和解码其实就是应用层协议
class LengthHeaderCodec
{
public:
    typedef std::function<void (
          const TcpConnectionPtr&,
          const string& message,
          TimeStamp)> StringMessageCallback;

    explicit LengthHeaderCodec(
          const StringMessageCallback& cb)
    : m_nMessageCallback(cb)
    {
    }

    // 每次套接字读出消息先是放入参数２的Buffer对象
    // 并触发回调
    // 但应用层回调里面要分析Buffer内容
    // 看是否有一条完整信息取出一条或多条
    // 还是本次略过，但后续累计到一条完整信息再处理
    void onMessage(
          const TcpConnectionPtr& conn,
          Buffer* buf,
          TimeStamp receiveTime)
    {
        while (buf->readableBytes() >= s_nHeaderLen)
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
            else if (buf->readableBytes() >= len + s_nHeaderLen)
            {
                buf->retrieve(s_nHeaderLen);
                string message(
                    buf->peek(),
                    len);
                // 这里应用层将协议打包和解码抽离为一个独立的类
                // 用将解码好的消息执行应用层回调
                m_nMessageCallback(
                    conn,
                    message,
                    receiveTime);
                buf->retrieve(len);
            }
            else
            {
                break;
            }
        }
    }

    void send(
          TcpConnection* conn,
          const StringPiece& message)
    {
        Buffer buf;
        buf.append(
            message.data(),
            message.size());
        int32_t len = static_cast<int32_t>(message.size());
        int32_t be32 = hostToNetwork32(len);
        buf.prepend(
            &be32,
            sizeof be32);

        conn->send(&buf);
    }

private:
    StringMessageCallback m_nMessageCallback;
    const static size_t s_nHeaderLen = sizeof(int32_t);
};

#endif // CODEC_H
