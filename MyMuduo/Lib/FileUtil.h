#ifndef NLIB_FILEUTIL_H
#define NLIB_FILEUTIL_H

#include "header.h"
#include "StringPiece.h"

// 通过类的接口来实现将文件内容读入缓冲区
class ReadSmallFile 
{
public:
    ReadSmallFile(
        StringArg filename);
    ~ReadSmallFile();
    
    template<typename T>
    int readToString(
        int maxSize,
        T* content,
        int64_t* fileSize,
        int64_t* modifyTime,
        int64_t* createTime);
          
    int readToBuffer(
        int* size);
    const char* buffer() const 
    { 
        return m_strBuf; 
    }

    static const int s_nBufferSize = 64*1024;

private:
    int m_nFd;
    int m_nErr;
    char m_strBuf[s_nBufferSize];
};

template<typename T>
int readFile(
    StringArg filename,
    int maxSize,
    T* content,
    int64_t* fileSize = NULL,
    int64_t* modifyTime = NULL,
    int64_t* createTime = NULL)
{
    ReadSmallFile file(filename);
    return file.readToString(
            maxSize, 
            content, 
            fileSize, 
            modifyTime, 
            createTime);
}

// 通过类接口，可以不断以追加方式向文件写入内容 
class AppendFile 
{
public:
    explicit AppendFile(
        StringArg filename);
    ~AppendFile();
    void append(
        const char* logline, 
        size_t len);
    
    void flush();
    off_t writtenBytes() const 
    { 
        return m_nWrittenBytes; 
    }
    
private:
    size_t write(
        const char* logline, 
        size_t len);
    
private:
    FILE* m_pFp;
    char m_strBuffer[64*1024];
    off_t m_nWrittenBytes;
};

#endif  

