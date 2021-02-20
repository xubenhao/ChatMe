xxx = Acceptor.o Connector.o TcpClient.o \
	  TcpConnection.o TcpServer.o
all : ${xxx}

Acceptor.o : Acceptor.cc
	g++ -c Acceptor.cc

Connector.o : Connector.cc
	g++ -c Connector.cc

TcpClient.o : TcpClient.cc
	g++ -c TcpClient.cc

TcpConnection.o : TcpConnection.cc
	g++ -c TcpConnection.cc

TcpServer.o : TcpServer.cc
	g++ -c TcpServer.cc
