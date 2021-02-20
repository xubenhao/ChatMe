#include "InetAddress.h"
#include "Logging.h"
#include "Endian.h"
#include "SocketOps.h"

static const in_addr_t InAddrAny = INADDR_ANY;
static const in_addr_t InAddrLoopBack = INADDR_LOOPBACK;

InetAddress::InetAddress(
    uint16_t port_, 
    bool loopbackOnly_, 
    bool ipv6_)
{
  if (ipv6_)
  {
    memset(&m_addr6, 0, sizeof(m_addr6));
    m_addr6.sin6_family = AF_INET6;
    in6_addr _ip = loopbackOnly_ ? in6addr_loopback : in6addr_any;
    m_addr6.sin6_addr = _ip;
    m_addr6.sin6_port = hostToNetwork16(port_);
  }
  else
  {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    in_addr_t _ip = loopbackOnly_ ? InAddrLoopBack : InAddrAny;
    m_addr.sin_addr.s_addr = hostToNetwork32(_ip);
    m_addr.sin_port = hostToNetwork16(port_);
  }
}

InetAddress::InetAddress(
    StringArg ip_, 
    uint16_t port_, 
    bool ipv6_)
{
  if (ipv6_)
  {
    memset(&m_addr6, 0, sizeof(m_addr6));
    fromIpPort(ip_.c_str(), port_, &m_addr6);
  }
  else
  {
    memset(&m_addr, 0, sizeof(m_addr));
    fromIpPort(ip_.c_str(), port_, &m_addr);
  }
}

string InetAddress::toIpPort() const
{
  char buf[64] = "";
  ::toIpPort(
          buf, 
          sizeof buf, 
          getSockAddr());
  return buf;
}

string InetAddress::toIp() const
{
  char buf[64] = "";
  ::toIp(
          buf, 
          sizeof buf, 
          getSockAddr());
  return buf;
}

uint32_t InetAddress::ipNetEndian() const
{
  assert(family() == AF_INET);
  return m_addr.sin_addr.s_addr;
}

uint16_t InetAddress::toPort() const
{
  return networkToHost16(portNetEndian());
}

// 静态保证此全局变量作用域为模块内
static __thread char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(
    StringArg hostname_, 
    InetAddress* out_)
{
  assert(out_ != NULL);
  struct hostent hent;
  struct hostent* he = NULL;
  int herrno = 0;
  memset(
          &hent, 
          0, 
          sizeof(hent));
  int ret = gethostbyname_r(
          hostname_.c_str(), 
          &hent, 
          t_resolveBuffer, 
          sizeof (t_resolveBuffer), 
          &he, 
          &herrno);
  if (ret == 0 
    && he != NULL)
  {
    assert(he->h_addrtype == AF_INET 
        && he->h_length == sizeof(uint32_t));
    out_->m_addr.sin_addr = *(struct in_addr*)(he->h_addr);
    return true;
  }
  else
  {
    if (ret)
    {
      LOG_SYSERR << "InetAddress::resolve";
    }

    return false;
  }
}

void InetAddress::setScopeId(uint32_t scope_id)
{
  if (family() == AF_INET6)
  {
    m_addr6.sin6_scope_id = scope_id;
  }
}

