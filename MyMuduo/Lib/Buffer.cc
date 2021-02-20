#include "Buffer.h"
#include "SocketOps.h"
#include "Logging.h"

const char Buffer::s_nCRLF[] = "\r\n";
const size_t Buffer::s_nCheapPrepend;
const size_t Buffer::s_nInitialSize;

ssize_t Buffer::readFd(
    int fd, 
    int* savedErrno)
{
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  LOG_INFO 
      << "writable num:"
      << writable;
  // 这样设计的原因：
  // 可以允许每个Buffer对象初始缓冲区大小较小
  // 但同时不会因此而无法接收大数据
  //
  // 初始Buffer容量控制是因为每个已经连接套接字均有２个Buffer对象
  // 并发访问下，可能同时存在很多Buffer对象
  // 此时控制Buffer预先分配容量，
  // 可以减轻内存压力
  vec[0].iov_base = begin()+m_nWriterIndex;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  LOG_INFO
      << "iovcnt "
      << iovcnt;

  const ssize_t n = readv(fd, vec, iovcnt);
  if (n < 0)
  {
    *savedErrno = errno;
  }
  else if ((size_t)(n) <= writable)
  {
    m_nWriterIndex += n;
  }
  else
  {
    m_nWriterIndex = m_nBuffer.size();
    // 会触发Buffer容量增大
    append(
        extrabuf, 
        n - writable);
  }
  
  return n;
}

