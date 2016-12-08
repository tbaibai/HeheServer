/*
 * TcpConnection.h
 *
 *  Created on: Dec 7, 2016
 *	  Author: lkl
 */

#ifndef TCPCONNECTION_H_
#define TCPCONNECTION_H_
#include <string>
#include "CircularFifo.h"

struct BufferNode{
	uint8_t* data = nullptr;
	size_t offset = 0;
	size_t size = 0;
};

class TcpConnection {
public:
	TcpConnection(){

	};
	TcpConnection(int fd, int epollFd, const std::string& ip, int port)
	: fd_(fd), epollFd_(epollFd), ip_(ip), port_(port){

	}
	virtual ~TcpConnection();
	int getFd(){
		return fd_;
	}
	const std::string& getIp(){
		return ip_;
	}
	int getPort(){
		return port_;
	}
	void beginWatchReadEvent();
	void handleEpollEvent(bool canRead, bool canWrite);
private:
	void _watchWriteEvent(bool bWatch);
	void _watchReadEvent(bool bWatch);
	void _modEpollEvent(int epollOpt);
private:
	int fd_ = -1;
	int epollFd_ = -1;
	uint32_t watchEvents_ = 0;
	std::string ip_;
	int port_ = -1;
	CircularFifo<BufferNode, 128> readBufQueue_;
	CircularFifo<BufferNode, 128> writeBufQueue_;//TODO 不应该使用CircularFifo,应使用线程安全的队列或者退化的对于2线程安全的队列
};

#endif /* TCPCONNECTION_H_ */
