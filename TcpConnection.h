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
	TcpConnection(int fd, const std::string& ip, int port)
	: fd_(fd), ip_(ip), port_(port){

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
	void handleEpollEvent(bool canRead, bool canWrite);
private:
	int fd_ = -1;
	std::string ip_;
	int port_ = -1;
	CircularFifo<BufferNode, 128> readBufQueue_;
	CircularFifo<BufferNode, 128> writeBufQueue_;//TODO 不应该使用CircularFifo,应使用线程安全的队列或者退化的对于2线程安全的队列
};

#endif /* TCPCONNECTION_H_ */
