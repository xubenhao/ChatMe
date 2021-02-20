//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "muduocodec.h"

MuduoCodec::MuduoCodec()
{
}

// 每次套接字读出消息先是放入参数２的Buffer对象
// 并触发回调
// 但应用层回调里面要分析Buffer内容
// 看是否有一条完整信息取出一条或多条
// 还是本次略过，但后续累计到一条完整信息再处理
void MuduoCodec::onMessage(
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
            //m_nMessageCallback(
            //    conn,
            //    message,
            //    receiveTime);
            buf->retrieve(len);
        }
        else
        {
            break;
        }
    }
}

void MuduoCodec::send(
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
