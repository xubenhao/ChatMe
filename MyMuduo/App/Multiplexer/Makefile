xxx1 = Buffer.o Channel.o Condition.o CountDownLatch.o \
	  CurrentThread.o Date.o EPollPoller.o EventLoop.o \
	  EventLoopThread.o EventLoopThreadPool.o Exception.o \
	  FileUtil.o InetAddress.o Logging.o LogStream.o Poller.o \
	  ProcessInfo.o Socket.o SocketOps.o Thread.o \
	  ThreadPool.o Timer.o TimerQueue.o TimeStamp.o \
	  TimeZone.o

xxx2 = Acceptor.o Connector.o TcpClient.o \
	  TcpConnection.o TcpServer.o

all : cp1 cp2 
cp1 : 
	cp ../../Lib/*.o .

cp2 :
	cp ../../Tcp/*.o .

#Server.o : Server.cc
#	g++ -c Server.cc

#Client : ${xxx1} ${xxx2} Client.o
#	g++ -o Client ${xxx1} ${xxx2} Client.o -lstdc++ -lpthread


