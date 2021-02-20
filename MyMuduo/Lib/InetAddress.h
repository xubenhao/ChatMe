#ifndef NLIB_INETADDRESS_H
#define NLIB_INETADDRESS_H

#include "header.h"
#include "StringPiece.h"

const struct sockaddr* sockaddr_cast(
    const struct sockaddr_in6* addr_);

class InetAddress    
{
public:  
    explicit InetAddress(
        uint16_t port_ = 0, 
        bool loopbackOnly_ = false, 
        bool ipv6_ = false);
    InetAddress(
        StringArg ip_, 
        uint16_t port_, 
        bool ipv6_ = false);
    explicit InetAddress(
        const struct sockaddr_in& addr_)
        : m_addr(addr_)  
    {   
    }
  
    explicit InetAddress(
        const struct sockaddr_in6& addr_)
        : m_addr6(addr_)  
    {   
    }
  
    sa_family_t family() const  
    {  
        return m_addr.sin_family;  
    }

    string toIp() const;
    string toIpPort() const;
    uint16_t toPort() const;
    const struct sockaddr* getSockAddr() const
    {
        return (struct sockaddr*)(&m_addr6);
    }

    void setSockAddrInet6(
        const struct sockaddr_in6& addr6_)
    {
        m_addr6 = addr6_;
    }

    uint32_t ipNetEndian() const;
    uint16_t portNetEndian() const
    {
        return m_addr.sin_port;
    }
          
    static bool resolve(
        StringArg hostname_, 
        InetAddress* result_);
    void setScopeId(uint32_t scope_id);

private:
    union
    {
        struct sockaddr_in m_addr;
        struct sockaddr_in6 m_addr6;
    };
};

#endif

