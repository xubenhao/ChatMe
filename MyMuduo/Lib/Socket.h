#ifndef NLIB_SOCKET_H
#define NLIB_SOCKET_H

struct tcp_info;
class InetAddress;

class Socket 
{
public:
    explicit Socket(int sockfd)
        : m_nSockFd(sockfd)
    { }

    ~Socket();
    
    int fd() const
    {
        return m_nSockFd;
    }

    bool getTcpInfo(struct tcp_info*) const;
    bool getTcpInfoString(char* buf, int len) const;
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);
    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int m_nSockFd;
};

#endif

