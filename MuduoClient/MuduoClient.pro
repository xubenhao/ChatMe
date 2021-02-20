QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../MyMuduo/App/Chat/Client.cc \
    ../MyMuduo/App/Chat/Server.cc \
    ../MyMuduo/App/Multiplexer/Demux.cc \
    ../MyMuduo/App/Multiplexer/Multiplexer.cc \
    ../MyMuduo/Lib/Buffer.cc \
    ../MyMuduo/Lib/Channel.cc \
    ../MyMuduo/Lib/Condition.cc \
    ../MyMuduo/Lib/CountDownLatch.cc \
    ../MyMuduo/Lib/CurrentThread.cc \
    ../MyMuduo/Lib/Date.cc \
    ../MyMuduo/Lib/EPollPoller.cc \
    ../MyMuduo/Lib/EventLoop.cc \
    ../MyMuduo/Lib/EventLoopThread.cc \
    ../MyMuduo/Lib/EventLoopThreadPool.cc \
    ../MyMuduo/Lib/Exception.cc \
    ../MyMuduo/Lib/FileUtil.cc \
    ../MyMuduo/Lib/InetAddress.cc \
    ../MyMuduo/Lib/LogFile.cc \
    ../MyMuduo/Lib/LogStream.cc \
    ../MyMuduo/Lib/Logging.cc \
    ../MyMuduo/Lib/PollPoller.cc \
    ../MyMuduo/Lib/Poller.cc \
    ../MyMuduo/Lib/ProcessInfo.cc \
    ../MyMuduo/Lib/Socket.cc \
    ../MyMuduo/Lib/SocketOps.cc \
    ../MyMuduo/Lib/Thread.cc \
    ../MyMuduo/Lib/ThreadPool.cc \
    ../MyMuduo/Lib/TimeStamp.cc \
    ../MyMuduo/Lib/TimeZone.cc \
    ../MyMuduo/Lib/Timer.cc \
    ../MyMuduo/Lib/TimerQueue.cc \
    ../MyMuduo/Tcp/Acceptor.cc \
    ../MyMuduo/Tcp/Connector.cc \
    ../MyMuduo/Tcp/TcpClient.cc \
    ../MyMuduo/Tcp/TcpConnection.cc \
    ../MyMuduo/Tcp/TcpServer.cc \
    Application/main.cpp \
    Global/global.cpp \
    MuduoApp/muduoclient.cpp \
    Ui/addfrienddialog.cpp \
    Ui/chatwidget.cpp \
    Ui/mainchatwidget.cpp \
    Ui/mainwindow.cpp \
    Ui/registerandloginwidget.cpp

HEADERS += \
    ../MyMuduo/App/Chat/codec.h \
    ../MyMuduo/Lib/Atomic.h \
    ../MyMuduo/Lib/Buffer.h \
    ../MyMuduo/Lib/CallBacks.h \
    ../MyMuduo/Lib/Channel.h \
    ../MyMuduo/Lib/Condition.h \
    ../MyMuduo/Lib/CountDownLatch.h \
    ../MyMuduo/Lib/CurrentThread.h \
    ../MyMuduo/Lib/Date.h \
    ../MyMuduo/Lib/EPollPoller.h \
    ../MyMuduo/Lib/Endian.h \
    ../MyMuduo/Lib/EventLoop.h \
    ../MyMuduo/Lib/EventLoopThread.h \
    ../MyMuduo/Lib/EventLoopThreadPool.h \
    ../MyMuduo/Lib/Exception.h \
    ../MyMuduo/Lib/FileUtil.h \
    ../MyMuduo/Lib/InetAddress.h \
    ../MyMuduo/Lib/LogFile.h \
    ../MyMuduo/Lib/LogStream.h \
    ../MyMuduo/Lib/Logging.h \
    ../MyMuduo/Lib/Mutex.h \
    ../MyMuduo/Lib/PollPoller.h \
    ../MyMuduo/Lib/Poller.h \
    ../MyMuduo/Lib/ProcessInfo.h \
    ../MyMuduo/Lib/Socket.h \
    ../MyMuduo/Lib/SocketOps.h \
    ../MyMuduo/Lib/StringPiece.h \
    ../MyMuduo/Lib/Thread.h \
    ../MyMuduo/Lib/ThreadLocal.h \
    ../MyMuduo/Lib/ThreadLocalSingleton.h \
    ../MyMuduo/Lib/ThreadPool.h \
    ../MyMuduo/Lib/TimeStamp.h \
    ../MyMuduo/Lib/TimeZone.h \
    ../MyMuduo/Lib/Timer.h \
    ../MyMuduo/Lib/TimerId.h \
    ../MyMuduo/Lib/TimerQueue.h \
    ../MyMuduo/Lib/WeakCallback.h \
    ../MyMuduo/Lib/header.h \
    ../MyMuduo/Lib/lib.h \
    ../MyMuduo/Tcp/Acceptor.h \
    ../MyMuduo/Tcp/Connector.h \
    ../MyMuduo/Tcp/TcpClient.h \
    ../MyMuduo/Tcp/TcpConnection.h \
    ../MyMuduo/Tcp/TcpServer.h \
    ../MyMuduo/Tcp/lib.h \
    Application/header.h \
    Application/lib.h \
    Application/main.h \
    DataStruct/KeyAllocator.h \
    DataStruct/RedBlackTree.h \
    DataStruct/datastruct.h \
    DataStruct/doublelist.h \
    DataStruct/dynarray.h \
    DataStruct/dynqueue.h \
    DataStruct/fixqueue.h \
    DataStruct/header.h \
    DataStruct/lib.h \
    Global/global.h \
    Global/header.h \
    Global/lib.h \
    MuduoApp/codec.h \
    MuduoApp/header.h \
    MuduoApp/lib.h \
    MuduoApp/muduoclient.h \
    Ui/addfrienddialog.h \
    Ui/chatwidget.h \
    Ui/header.h \
    Ui/lib.h \
    Ui/mainchatwidget.h \
    Ui/mainwindow.h \
    Ui/registerandloginwidget.h

FORMS += \
    Ui/addfrienddialog.ui \
    Ui/chatwidget.ui \
    Ui/mainchatwidget.ui \
    Ui/mainwindow.ui \
    Ui/registerandloginwidget.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../MyMuduo/App/Chat/2.txt \
    ../MyMuduo/App/Chat/Acceptor.o \
    ../MyMuduo/App/Chat/Buffer.o \
    ../MyMuduo/App/Chat/Channel.o \
    ../MyMuduo/App/Chat/Client \
    ../MyMuduo/App/Chat/Client.o \
    ../MyMuduo/App/Chat/Condition.o \
    ../MyMuduo/App/Chat/Connector.o \
    ../MyMuduo/App/Chat/CountDownLatch.o \
    ../MyMuduo/App/Chat/CurrentThread.o \
    ../MyMuduo/App/Chat/Date.o \
    ../MyMuduo/App/Chat/EPollPoller.o \
    ../MyMuduo/App/Chat/EventLoop.o \
    ../MyMuduo/App/Chat/EventLoopThread.o \
    ../MyMuduo/App/Chat/EventLoopThreadPool.o \
    ../MyMuduo/App/Chat/Exception.o \
    ../MyMuduo/App/Chat/FileUtil.o \
    ../MyMuduo/App/Chat/InetAddress.o \
    ../MyMuduo/App/Chat/Log.txt \
    ../MyMuduo/App/Chat/Log2.txt \
    ../MyMuduo/App/Chat/LogStream.o \
    ../MyMuduo/App/Chat/Logging.o \
    ../MyMuduo/App/Chat/Makefile \
    ../MyMuduo/App/Chat/Poller.o \
    ../MyMuduo/App/Chat/ProcessInfo.o \
    ../MyMuduo/App/Chat/Server \
    ../MyMuduo/App/Chat/Server.o \
    ../MyMuduo/App/Chat/Socket.o \
    ../MyMuduo/App/Chat/SocketOps.o \
    ../MyMuduo/App/Chat/TcpClient.o \
    ../MyMuduo/App/Chat/TcpConnection.o \
    ../MyMuduo/App/Chat/TcpServer.o \
    ../MyMuduo/App/Chat/Thread.o \
    ../MyMuduo/App/Chat/ThreadPool.o \
    ../MyMuduo/App/Chat/TimeStamp.o \
    ../MyMuduo/App/Chat/TimeZone.o \
    ../MyMuduo/App/Chat/Timer.o \
    ../MyMuduo/App/Chat/TimerQueue.o \
    ../MyMuduo/App/Multiplexer/Makefile \
    ../MyMuduo/Lib/Buffer.o \
    ../MyMuduo/Lib/Channel.o \
    ../MyMuduo/Lib/Condition.o \
    ../MyMuduo/Lib/CountDownLatch.o \
    ../MyMuduo/Lib/CurrentThread.o \
    ../MyMuduo/Lib/Date.o \
    ../MyMuduo/Lib/EPollPoller.o \
    ../MyMuduo/Lib/EventLoop.o \
    ../MyMuduo/Lib/EventLoopThread.o \
    ../MyMuduo/Lib/EventLoopThreadPool.o \
    ../MyMuduo/Lib/Exception.o \
    ../MyMuduo/Lib/FileUtil.o \
    ../MyMuduo/Lib/InetAddress.o \
    ../MyMuduo/Lib/LogStream.o \
    ../MyMuduo/Lib/Logging.o \
    ../MyMuduo/Lib/Makefile \
    ../MyMuduo/Lib/Poller.o \
    ../MyMuduo/Lib/ProcessInfo.o \
    ../MyMuduo/Lib/Socket.o \
    ../MyMuduo/Lib/SocketOps.o \
    ../MyMuduo/Lib/Thread.o \
    ../MyMuduo/Lib/ThreadPool.o \
    ../MyMuduo/Lib/TimeStamp.o \
    ../MyMuduo/Lib/TimeZone.o \
    ../MyMuduo/Lib/Timer.o \
    ../MyMuduo/Lib/TimerQueue.o \
    ../MyMuduo/Lib/a.out \
    ../MyMuduo/Lib/c \
    ../MyMuduo/Lib/s \
    ../MyMuduo/Tcp/Acceptor.o \
    ../MyMuduo/Tcp/Connector.o \
    ../MyMuduo/Tcp/Makefile \
    ../MyMuduo/Tcp/TcpClient.o \
    ../MyMuduo/Tcp/TcpConnection.o \
    ../MyMuduo/Tcp/TcpServer.o
