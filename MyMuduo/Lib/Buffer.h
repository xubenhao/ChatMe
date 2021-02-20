#ifndef NLIB_BUFFER_H
#define NLIB_BUFFER_H
#include "header.h"
#include "StringPiece.h"
#include "Endian.h"
#include "Logging.h"
// Buffer的设计：
// １．容量动态变化
// ２．默认无锁／同步
// ３．锁定区＋已经处理区＋尚未处理区＋可操作区
//
class Buffer 
{
public:
    static const size_t s_nCheapPrepend = 8;
    static const size_t s_nInitialSize = 1024;
    explicit Buffer(
        size_t initialSize = s_nInitialSize)
        : m_nBuffer(s_nCheapPrepend + initialSize),
        m_nReaderIndex(s_nCheapPrepend),
        m_nWriterIndex(s_nCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == s_nCheapPrepend);
    }
    
    void swap(Buffer& rhs)
    {
        m_nBuffer.swap(rhs.m_nBuffer);
        std::swap(m_nReaderIndex, rhs.m_nReaderIndex);
        std::swap(m_nWriterIndex, rhs.m_nWriterIndex);
    }

    size_t readableBytes() const
    {
        return m_nWriterIndex - m_nReaderIndex;
    }
    
    size_t writableBytes() const
    {
        return m_nBuffer.size() - m_nWriterIndex;
    }

    size_t prependableBytes() const
    {
        return m_nReaderIndex;
    }
    
    const char* peek() const
    {
        return begin() + m_nReaderIndex;
    }

    // 从首个可读到首个可写范围区间内搜索s_nCRLF
    const char* findCRLF() const
    {
        const char* crlf = std::search(
                peek(), 
                beginWrite(), 
                s_nCRLF, 
                s_nCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }


    // 指定一个首个可读到首个可写中间某字节位置为起始查找位置
    const char* findCRLF(
            const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(
                start, 
                beginWrite(), 
                s_nCRLF, 
                s_nCRLF+2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // 从首个可读位置开始，针对可读部分，查找\n
    const char* findEOL() const
    {
        const void* eol = memchr(
                peek(), 
                '\n', 
                readableBytes());
        return static_cast<const char*>(eol);
    }
    
    // 从指定的首个可读位置到首个可写位置间某个位置，针对可读部分
    const char* findEOL(
            const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const void* eol = memchr(
                start, 
                '\n', 
                beginWrite() - start);
        return static_cast<const char*>(eol);
    }

    // 使可读部分指定长度标记为已经读取
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            m_nReaderIndex += len;
        }
        else
        {
            retrieveAll();
        }
    }

    // 指定首个可读到首个可写之间某个字节为中止字节
    // 将该中止字节前可读部分，全部标记为已经读取．
    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    
    void retrieveInt64()
    {
        retrieve(sizeof(int64_t));
    }
    
    void retrieveInt32()
    {
        retrieve(sizeof(int32_t));
    }
    
    void retrieveInt16()
    {
        retrieve(sizeof(int16_t));
    }

    void retrieveInt8()
    {
        retrieve(sizeof(int8_t));
    }

    void retrieveAll()
    {
        m_nReaderIndex = s_nCheapPrepend;
        m_nWriterIndex = s_nCheapPrepend;
    }
    
    // 将可读部分全部读取
    // 读取内容以一个string对象返回
    string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    
    string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }
    
    // 将可读部分全部读取
    // 读取内容以一个StringPiece对象返回
    StringPiece toStringPiece() const
    {
        return StringPiece(
                peek(), 
                static_cast<int>(readableBytes()));
    }

    void append(const StringPiece& str)
    {
        append(
            str.data(), 
            str.size());
    }
    
    void append(
            const char* /*restrict*/ data, 
            size_t len)
    {
        ensureWritableBytes(len);
        std::copy(
            data, 
            data+len, 
            beginWrite());
        hasWritten(len);
    }

    // 往缓冲区写入一个指定长度对象
    void append(
            const void*  data, 
            size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void ensureWritableBytes(size_t len)
    {
        // 如果缓冲区可写字节数少于要求数量
        if (writableBytes() < len)
        {
            // 想办法扩大缓冲区可写字节数量
            makeSpace(len);
        }
        
        assert(writableBytes() >= len);
    }
    
    char* beginWrite()
    { 
        return begin() + m_nWriterIndex; 
    }
    
    const char* beginWrite() const
    { 
        return begin() + m_nWriterIndex; 
    }

    // 将可写部分指定长度内容标记为已经写入
    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        m_nWriterIndex += len;
    }

    // 撤销对最近某个指定字节长度的写入
    // 将首个可写字节前指定长度部分标记为未写入
    // 更新首个可写字节位置索引
    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        m_nWriterIndex -= len;
    }

    void appendInt64(int64_t x)
    {
        int64_t be64 = hostToNetwork64(x);
        append(&be64, sizeof be64);
    }

    void appendInt32(int32_t x)
    {
        int32_t be32 = hostToNetwork32(x);
        append(&be32, sizeof be32);
    }

    void appendInt16(int16_t x)
    {
        int16_t be16 = hostToNetwork16(x);
        append(&be16, sizeof be16);
    }
    
    void appendInt8(int8_t x)
    {
        append(&x, sizeof x);
    }
    
    int64_t readInt64()
    {
        int64_t result = peekInt64();
        retrieveInt64();
        return result;
    }
    
    int32_t readInt32()
    {
        int32_t result = peekInt32();
        retrieveInt32();
        return result;
    }
    
    int16_t readInt16()
    {
        int16_t result = peekInt16();
        retrieveInt16();
        return result;
    }

    int8_t readInt8()
    {
        int8_t result = peekInt8();
        retrieveInt8();
        return result;
    }

    // 从缓冲区读出一个Int64对象
    // 但读出部分未被标记为已经读取
    int64_t peekInt64() const
    {
        assert(readableBytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(
            &be64, 
            peek(), 
            sizeof be64);
        return networkToHost64(be64);
    }

    int32_t peekInt32() const
    {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);
        return networkToHost32(be32);
    }
    
    int16_t peekInt16() const
    {
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof be16);
        return networkToHost16(be16);
    }

    int8_t peekInt8() const
    {
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = *peek();
        return x;
    }

    void prependInt64(int64_t x)
    {
        int64_t be64 = hostToNetwork64(x);
        prepend(&be64, sizeof be64);
    }
    
    void prependInt32(int32_t x)
    {
        int32_t be32 = hostToNetwork32(x);
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x)
    {
        int16_t be16 = hostToNetwork16(x);
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x)
    {
        prepend(&x, sizeof x);
    }
    
    // 在已经读取区间内末尾len大小重新设置为未被读取
    // 并将此部分用一个指定长度为len的对象填充
    // 更新首个可读字节位置索引
    void prepend(
            const void* /*restrict*/ data, 
            size_t len)
    {
        assert(len <= prependableBytes());
        m_nReaderIndex -= len;
        const char* d = 
            static_cast<const char*>(data);
        
        // 往可读位置放入指定len大小的由
        // data指向的对象内容
        std::copy(
            d, 
            d+len, 
            begin()+m_nReaderIndex);
    }
    
    void shrink(size_t reserve)
    {
        Buffer other;
        other.ensureWritableBytes(readableBytes()+reserve);
        other.append(toStringPiece());
        swap(other);
    }

    size_t internalCapacity() const
    {
        return m_nBuffer.capacity();
    }
    
    ssize_t readFd(
        int fd, 
        int* savedErrno);
private:
    char* begin()
    {
        return &*m_nBuffer.begin();
    }
    
    const char* begin() const
    {
        return &*m_nBuffer.begin();
    }
    
    
    // 空间扩展的方法：
    // 一个是将已经读取部分腾出来
    // 一个是分配一个更大的缓冲区
    // 空间扩展
    // 缓冲区
    // 预留部分＋已经读取部分＋可读取部分＋可写入部分
    void makeSpace(size_t len)
    {
        // 可写部分＋已经读取部分
        if (writableBytes() + prependableBytes() < 
                len + s_nCheapPrepend)
        {
            // 申请新空间
            // 保持已经读取部分＋可读部分不变下
            // 让可写部分长度变为len
            m_nBuffer.resize(m_nWriterIndex+len);
        }
        else
        {
            // 去掉预留部分来腾出额外可写空间
            assert(s_nCheapPrepend < m_nReaderIndex);
            // 可读部分
            size_t readable = readableBytes();
            // 将可读部分往已经读取部分挪动
            // 这样空出来的部分将变为可写
            // 进而扩大了可写空间
            // 前面s_nCheapPrepend一般固定预留
            std::copy(begin()+m_nReaderIndex,
                    begin()+m_nWriterIndex,
                    begin()+s_nCheapPrepend);
            // 更新首个可读字节位置索引
            m_nReaderIndex = s_nCheapPrepend;
            // 更新首个可写字节位置索引
            m_nWriterIndex = m_nReaderIndex + readable;
            assert(readable == readableBytes());
        }
    }
    
private:
    // 此Buffer对象自身不提供多线程互斥／同步
    // 需要时，在多线程共享同一Buffer对象访问时自行添加
    std::vector<char> m_nBuffer;
    size_t m_nReaderIndex;
    size_t m_nWriterIndex;
    static const char s_nCRLF[];
};


#endif

