/*
 * TcpServer.h
 *
 *  Created on: Nov 27, 2016
 *	  Author: lkl
 */

#ifndef TCPSERVER_H_
#define TCPSERVER_H_
#include <sys/epoll.h>
#include <map>
#include <thread>

class TcpConnection;

class TcpServer {
public:
	TcpServer(int port);
	virtual ~TcpServer();
	void start();
private:
	void _handleEpollEvent(TcpConnection* con, bool canRead, bool canWrite);
	void _onNewConnection(TcpConnection* newCon);
private:
	int listenFd_ = -1;
	int epollFd_ = -1;
	int curEpollFdNum_ = 0;
	std::thread* thread_ = nullptr;
};

#endif /* TCPSERVER_H_ */
