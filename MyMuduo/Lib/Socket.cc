#include "Socket.h"
#include "Logging.h"
#include "InetAddress.h"
#include "SocketOps.h"

Socket::~Socket()
{
  close(m_nSockFd);
}

bool Socket::getTcpInfo(
        struct tcp_info* tcpi) const
{
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(
          m_nSockFd, 
          SOL_TCP, 
          TCP_INFO, 
          tcpi, 
          &len) == 0;
}

bool Socket::getTcpInfoString(
        char* buf, 
        int len) const
{
  struct tcp_info tcpi;
  bool ok = getTcpInfo(&tcpi);
  if (ok)
  {
    snprintf(
        buf, 
        len, 
        "unrecovered=%u "
        "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
        "lost=%u retrans=%u rtt=%u rttvar=%u "
        "sshthresh=%u cwnd=%u total_retrans=%u",
        // Number of unrecovered [RTO] timeouts
        tcpi.tcpi_retransmits,  
        // Retransmit timeout in usec
        tcpi.tcpi_rto,          
        // Predicted tick of soft clock in usec
        tcpi.tcpi_ato,          
        tcpi.tcpi_snd_mss,
        tcpi.tcpi_rcv_mss,
        // Lost packets
        tcpi.tcpi_lost,         
        // Retransmitted packets out
        tcpi.tcpi_retrans,      
        // Smoothed round trip time in usec
        tcpi.tcpi_rtt,          
        // Medium deviation
        tcpi.tcpi_rttvar,       
        tcpi.tcpi_snd_ssthresh,
        tcpi.tcpi_snd_cwnd,
        // Total retransmits for entire connection
        tcpi.tcpi_total_retrans);  
  }

  return ok;
}

void Socket::bindAddress(const InetAddress& addr)
{
  bindOrDie(
          m_nSockFd, 
          addr.getSockAddr());
}

void Socket::listen()
{
  listenOrDie(m_nSockFd);
}

int Socket::accept(InetAddress* peeraddr)
{
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof addr);
  int connfd = ::accept(m_nSockFd, &addr);
  if (connfd >= 0)
  {
    peeraddr->setSockAddrInet6(addr);
  }

  return connfd;
}

void Socket::shutdownWrite()
{
  ::shutdownWrite(m_nSockFd);
}

void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(
    m_nSockFd, 
    IPPROTO_TCP, 
    TCP_NODELAY,
    &optval, 
    (socklen_t)(sizeof optval));
}

void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(
    m_nSockFd, 
    SOL_SOCKET, 
    SO_REUSEADDR,
    &optval, 
    (socklen_t)(sizeof optval));
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(
    m_nSockFd, 
    SOL_SOCKET, 
    SO_REUSEPORT,
    &optval, 
    (socklen_t)(sizeof optval));
  if (ret < 0 && on)
  {
    LOG_SYSERR << "SO_REUSEPORT failed.";
  }
#else
  if (on)
  {
    LOG_ERROR << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(
    m_nSockFd, 
    SOL_SOCKET, 
    SO_KEEPALIVE,
    &optval, 
    (socklen_t)(sizeof optval));
}


