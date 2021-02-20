#include "FileUtil.h"
#include "Logging.h"


// 造成模板类的实例化?
template int readFile(
    StringArg filename,
    int maxSize,
    string* content,
    int64_t*,
    int64_t*,
    int64_t*);

template int ReadSmallFile::readToString(
    int maxSize,
    string* content,
    int64_t*, 
    int64_t*, 
    int64_t*);

ReadSmallFile::ReadSmallFile(
    StringArg filename)
  : m_nFd(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
    m_nErr(0)
{
  m_strBuf[0] = '\0';
  if (m_nFd < 0)
  {
    m_nErr = errno;
  }
}

ReadSmallFile::~ReadSmallFile()
{
  if (m_nFd >= 0)
  {
    ::close(m_nFd);
  }
}

template<typename T>
int ReadSmallFile::readToString(
    int maxSize,    
    T* content,
    int64_t* fileSize,
    int64_t* modifyTime,
    int64_t* createTime)
{
  static_assert(
    sizeof(off_t) == 8, 
    "_FILE_OFFSET_BITS = 64");
  int err = m_nErr;
  if (m_nFd >= 0)
  {
    // 默认T要支持clear-----不好
    content->clear();
    if (fileSize)
    {
      struct stat statbuf;
      if (::fstat(m_nFd, &statbuf) == 0)
      {
          // 普通文件
        if (S_ISREG(statbuf.st_mode))
        {
            // 记录文件大小
          *fileSize = statbuf.st_size;
          // 默认T要支持reserve----不好．一般T只有string才会这样．
          // 这样就没必要用模板
          content->reserve(
            (int)(std::min(
                (int64_t)(maxSize),     
                *fileSize)));// 缓冲区大小取maxSize和文件大小较小者
        }
        else if (S_ISDIR(statbuf.st_mode))
        {
          err = EISDIR;
        }

        // 最后修改时间
        if (modifyTime)
        {
          *modifyTime = statbuf.st_mtime;
        }

        // 创建时间
        if (createTime)
        {
          *createTime = statbuf.st_ctime;
        }
      }
      else
      {
        err = errno;
      }
    }

    // maxSize限定了缓冲中可缓冲内容大小
    // 要么将缓冲区填满
    // 要么将文件内容全部读到缓冲区
    while (content->size() < (size_t)(maxSize))
    {
        // 读取
      size_t toRead = 
          std::min((size_t)(
            maxSize) - content->size(),       
            sizeof(m_strBuf));
      ssize_t n = 
          ::read(m_nFd, m_strBuf, toRead);
      if (n > 0)
      {
          // 添加
          content->append(m_strBuf, n);
      }
      else
      {
        if (n < 0)
        {
          err = errno;
        }

        break;
      }
    }
  }

  return err;
}

// 按缓冲区大小只读一次
int ReadSmallFile::readToBuffer(
    int* size)
{
  int err = m_nErr;
  if (m_nFd >= 0)
  {
      // 以原子方式
      // 指定读取位置＋实际读取
    ssize_t n = ::pread(
        m_nFd, 
        m_strBuf, 
        sizeof(m_strBuf)-1, 
        0);
    if (n >= 0)
    {
      if (size)
      {
        *size = (int)(n);
      }

      m_strBuf[n] = '\0';
    }
    else
    {
      err = errno;
    }
  }

  return err;
}



AppendFile::AppendFile(
    StringArg filename)
  : m_pFp(::fopen(filename.c_str(), "ae")), 
    m_nWrittenBytes(0)
{
    assert(m_pFp);
    // 将变量的一个数据成员指定为I/O缓冲区
    // 未指定时，系统也会安排一个缓冲区
    ::setbuffer(
        m_pFp, 
        m_strBuffer, 
        sizeof(m_strBuffer));
}

AppendFile::~AppendFile()
{
  ::fclose(m_pFp);
}

void AppendFile::append(
    const char* logline, 
    const size_t len)
{
    // 写文件
  size_t n = write(logline, len);
  size_t remain = len - n;
  // 可以写多次，
  // 保证一次调用把len长度的内容全部写完
  while (remain > 0)
  {
    size_t x = write(
            logline + n, 
            remain);
    if (x == 0)
    {
      int err = ferror(m_pFp);
      if (err)
      {
        fprintf(
            stderr, 
            "AppendFile::append() failed %s\n", 
            strerror_tl(err));
      }

      break;
    }

    n += x;
    remain = len - n; 
  }

  m_nWrittenBytes += len;
}

// 让缓冲数据立即写入磁盘
void AppendFile::flush()
{
  ::fflush(m_pFp);
}

// 写文件
size_t AppendFile::write(
    const char* logline, 
    size_t len)
{
  return ::fwrite_unlocked(
    logline, 
    1, 
    len, 
    m_pFp);
}



