#ifndef NLIB_CHANNEL_H
#define NLIB_CHANNEL_H
#include "header.h"
#include "TimeStamp.h"
#include "EventLoop.h"
    
class EventLoop;  

// 1.一个事件循环loop时占据一个线程
// 线程不停处理的就是阻塞等待描述符集合中每个描述符上事件是否发生
// 每个描述符要等待哪些事件／事件发生时各类回调指定／等待事件更新
// Channel负责处理上述事宜
// 2.Channel对象本身无互斥／同步处理．
// 存在多线程操作共享Channel对象情况时，使用者负责处理互斥／同步

// 描述符－－Channel－－EPoll
//
class Channel  
{
public: 
    typedef std::function<void()> EventCallback;  
    typedef std::function<void(TimeStamp)> ReadEventCallback;
    // 传入loop负责fd的事件监听及回调
    Channel(
        EventLoop* loop, 
        int fd);  
    ~Channel();  
    void handleEvent(
        TimeStamp receiveTime);
    void setReadCallback(
        ReadEventCallback cb)  
    {  
        m_nReadCallback = std::move(cb);  
    }
    
    void setWriteCallback(
        EventCallback cb)  
    {  
        m_nWriteCallback = std::move(cb);  
    }
    
    void setCloseCallback(
        EventCallback cb)  
    {  
        m_nCloseCallback = std::move(cb);  
    }
    
    void setErrorCallback(
        EventCallback cb)  
    {  
        m_nErrorCallback = std::move(cb);  
    }
  
    void tie(const std::shared_ptr<void>&);  
    int fd() const  
    {  
        return m_nFd;  
    }
    
    int events() const  
    {  
        return m_nEvents;  
    }
        
    // 描述符要求Poll监听一个事件集合
    // 事件发生时Poll告知发生的事件集合［监听事件的子集］
    void set_revents(int revt)  
    {  
        m_nRevents = revt;
    } 
  
    bool isNoneEvent() const  
    {
        return m_nEvents == s_nNoneEvent;
    }

    void enableReading()  
    {  
        m_nEvents |= s_nReadEvent;  
        update();  
    }
    
    void disableReading()  
    {  
        m_nEvents &= ~s_nReadEvent; 
        update();  
    }
    
    void enableWriting()
    {
        m_nEvents |= s_nWriteEvent; 
        update();  
    }
    
    void disableWriting()  
    {  
        m_nEvents &= ~s_nWriteEvent;  
        update();  
    }
    
    void disableAll()  
    {  
        m_nEvents = s_nNoneEvent;  
        update();
    }
    
    bool isWriting() const  
    {  
        return m_nEvents & s_nWriteEvent;  
    }
    
    bool isReading() const  
    {  
        return m_nEvents & s_nReadEvent;  
    }
    
    int index()  
    {  
        return m_nIndex;  
    }
    
    void set_index(int idx)  
    {  
        m_nIndex = idx;  
    }

    string reventsToString() const;  
    string eventsToString() const;  
    void doNotLogHup()  
    {  
        m_bLogHup = false;  
    }
    
    EventLoop* ownerLoop()  
    {  
        return m_pLoop;  
    }
    
    void remove();     
    static string eventsToString(int fd, int ev);
    
private:
    void update();
    void handleEventWithGuard(TimeStamp receiveTime);

private:
    static const int s_nNoneEvent;
    static const int s_nReadEvent;
    static const int s_nWriteEvent;

private:
    EventLoop* m_pLoop;
    const int  m_nFd;
    int m_nEvents;
    int m_nRevents;
    
    // 用于标识
    // 本Channel负责的描述符针对关联的事件循环的Poll
    // 是尚未监听／已经监听／．．．
    int m_nIndex;
    // 挂起时记录日志
    bool m_bLogHup;
    
    // weak_ptr<void>可存储任何weak_ptr<T>类型对象
    std::weak_ptr<void> m_pTie;
    bool m_bTied;
    bool m_bEventHandling;
    bool m_bAddedToLoop;
    
    ReadEventCallback m_nReadCallback;
    EventCallback m_nWriteCallback;
    EventCallback m_nCloseCallback;
    EventCallback m_nErrorCallback;    
};


#endif

