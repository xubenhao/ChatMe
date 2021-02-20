#ifndef NLIB_LOGSTREAM_H
#define NLIB_LOGSTREAM_H

#include "StringPiece.h"
const int s_nSmallBuffer = 4000;
const int s_nLargeBuffer = 4000*1000;
template<int SIZE>
class FixedBuffer 
{
public:
    FixedBuffer()
        : m_pCur(m_arrData)
    {
        setCookie(cookieStart);
    }
    
    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }
    
    void append(
        const char* buf, 
        size_t len)
    {
        // 剩余空间可以容纳参数指定内容
        if ((size_t)(avail()) > len)
        {
            memcpy(m_pCur, buf, len);
            m_pCur += len;
            // m_pCur指向下一可用字节位置
        }
    }

    const char* data() const 
    { 
        return m_arrData; 
    }

    // 返回已经存储数据长度
    int length() const 
    { 
        return (int)(m_pCur - m_arrData); 
    }

    // 返回下一可用字节位置指针
    char* current() 
    { 
        return m_pCur; 
    }

    // 返回剩余可用空间大小
    int avail() const 
    { 
        return (int)(end() - m_pCur); 
    }

    // 无条件移动
    void add(size_t len) 
    { 
        m_pCur += len; 
    }
    
    // 重置
    void reset() 
    { 
        m_pCur = m_arrData; 
    }
    
    // 缓冲区内容置０
    void bzero() 
    { 
        memset(m_arrData, 0, sizeof(m_arrData)); 
    }
    
    const char* debugString();
    // 设置函数指针
    void setCookie(void (*cookie)()) 
    { 
        m_pCookie = cookie; 
    }

    // 类型转换，转化为string类型
    string toString() const 
    { 
        return string(m_arrData, length()); 
    }

    // 转化为StringPiece类型
    StringPiece toStringPiece() const 
    { 
        return StringPiece(m_arrData, length()); 
    }

private:
    // 移动到缓冲区尾后字节位置
    const char* end() const 
    { 
        return m_arrData + sizeof(m_arrData); 
    }
    
    static void cookieStart();
    static void cookieEnd();
    void (*m_pCookie)();
    char m_arrData[SIZE];
    char* m_pCur;
};

class LogStream 
{
    typedef LogStream Self;
public:
    // Buffer是一个固定大小的缓冲区
    typedef FixedBuffer<s_nSmallBuffer> Buffer;
    // LogStream支持<<操作符
    Self& operator<<(bool v)
    {
        // 往缓冲区写入bool类型对象
        m_nBuffer.append(v ? "1" : "0", 1);
        return *this;
    }
    
    Self& operator<<(short);
    Self& operator<<(unsigned short);
    Self& operator<<(int);
    Self& operator<<(unsigned int);
    Self& operator<<(long);
    Self& operator<<(unsigned long);
    Self& operator<<(long long);
    Self& operator<<(unsigned long long);
    Self& operator<<(const void*);
    Self& operator<<(float v)
    {
        *this << (double)(v);
        return *this;
    }
    
    Self& operator<<(double);
    Self& operator<<(char v)
    {
        m_nBuffer.append(&v, 1);
        return *this;
    }
    
    Self& operator<<(const char* str)
    {
        if (str)
        {
          m_nBuffer.append(str, strlen(str));
        }
        else
        {
          m_nBuffer.append("(null)", 6);
        }
    
        return *this;
    }

    Self& operator<<(const unsigned char* str)
    {
        return operator<<((const char*)(str));
    }

    Self& operator<<(const string& v)
    {
        m_nBuffer.append(v.c_str(), v.size());
        return *this;
    }
    
    Self& operator<<(const StringPiece& v)
    {
        m_nBuffer.append(v.data(), v.size());
        return *this;
    }

    Self& operator<<(const Buffer& v)
    {
        *this << v.toStringPiece();
        return *this;
    }
    
    void append(const char* data, int len) 
    { 
        m_nBuffer.append(data, len); 
    }
    
    const Buffer& buffer() const 
    { 
        return m_nBuffer; 
    }

    void resetBuffer() 
    { 
        m_nBuffer.reset(); 
    }

private:
    void staticCheck();
    template<typename T>
    void formatInteger(T);
    Buffer m_nBuffer;
    static const int s_nMaxNumericSize = 32;
};


class Fmt
{
public:
    template<typename T>
    Fmt(const char* fmt, T val);
    
    const char* data() const 
    { 
        return m_arrBuf; 
    }

    int length() const 
    { 
        return m_nLength; 
    }
private:
    char m_arrBuf[32];
    int m_nLength;
};


inline LogStream& operator<<(
    LogStream& s, 
    const Fmt& fmt)
{
    s.append(fmt.data(), fmt.length());   
    return s;
}

string formatSI(int64_t n);
string formatIEC(int64_t n);
#endif  

