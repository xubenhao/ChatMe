//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef MUDUO_APP_MUDUOCODEC_H
#define MUDUO_APP_MUDUOCODEC_H

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
class MuduoCodec
{
public:
    explicit MuduoCodec();

    // 每次套接字读出消息先是放入参数２的Buffer对象
    // 并触发回调
    // 但应用层回调里面要分析Buffer内容
    // 看是否有一条完整信息取出一条或多条
    // 还是本次略过，但后续累计到一条完整信息再处理
    void onMessage(
        const TcpConnectionPtr& conn,
        Buffer* buf,
        TimeStamp receiveTime);

    void send(
          TcpConnection* conn,
          const StringPiece& message);
private:
    const static size_t s_nHeaderLen = sizeof(int32_t);
};


#endif // MUDUOCODEC_H
