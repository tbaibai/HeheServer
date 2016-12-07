/*
 * TcpServer.cpp
 *
 *  Created on: Nov 27, 2016
 *	  Author: lkl
 */

#include "TcpServer.h"
#include "LinuxHeads.h"
#include <vector>
#include <iostream>
#include "TcpConnection.h"
using namespace std;


TcpServer::TcpServer(int port) {
	//创建非阻塞的tcp套接字 SOCK_CLOEXEC表明close in exec，not in fork
	int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (listen_fd < 0){
		exit(0);
	}
	//设置reuseaddr选项
	int optval = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));

	//bind地址
	struct sockaddr_in addr;
	bzero(&addr, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htobe32(INADDR_ANY);//或者INADDR_LOOPBACK 或者本机的某个特定ip
	addr.sin_port = htobe16(port);
	int ret = bind(listen_fd, (struct sockaddr *)(&addr), sizeof(addr));
	if (ret < 0){
		exit(0);
	}

	//listen
	ret = listen(listen_fd, SOMAXCONN);
	if (ret < 0){
		exit(0);
	}

	listenFd_ = listen_fd;

	int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if(epoll_fd < 0) {
		exit(0);
	}
	epollFd_ = epoll_fd;

	_beginWatchReadEvent(nullptr);
}

TcpServer::~TcpServer() {
	// TODO Auto-generated destructor stub
	if (thread_ && thread_->joinable())
	{
		thread_->join();
	}
}

void TcpServer::start(){
	thread_ = new thread([]{
		while(true){
			vector<struct epoll_event> eventsResult(curEpollFdNum_);
			int numEvents = epoll_wait(epollFd_, (epoll_event*)&(eventsResult[0]), curEpollFdNum_, -1);
			if (numEvents > 0){
				for(int i =0; i < numEvents; ++i){
					int kk = eventsResult[i].data.fd;
					int ss = eventsResult[i].events;
					cout << kk << " " << ss <<endl;
					auto con = (TcpConnection*)eventsResult[i].data.ptr;
					int events = eventsResult[i].events;
					//参考redis
					bool canRead = (events & EPOLLIN);
					bool canWrite = ((events & EPOLLOUT) || (events & EPOLLERR) || (events & EPOLLHUP));
					_handleEpollEvent(con, canRead, canWrite);
				}
			}
			else if (numEvents == 0){//wait -1时，应该不会出现此结果
				//LOG_TRACE << "nothing happended";
			}
			else{//此处参考chenshuo的muduo库的做法； linux手册范例是直接exit； redis3.2源码是完全不处理numEvents<=0的情况
				if (errno != EINTR){
					//LOG_SYSERR << "EPollPoller::poll()";
				}
			}
		}
	});
}

void TcpServer::_handleEpollEvent(TcpConnection* con, bool canRead, bool canWrite)
{
	if (con != nullptr){
		con->handleEpollEvent(canRead, canWrite);
		return;
	}
	//参考redis
	if (canRead){//处理新连接
		int max = 1000;//redis里的MAX_ACCEPTS_PER_CALL宏
		while(max--) {
			struct sockaddr_in addr;
			bzero(&addr, sizeof addr);
			socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
			int connfd = accept4(listenFd_, (sockaddr*)(&addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (connfd < 0) {
				if (errno != EWOULDBLOCK){//EWOULDBLOCK == EAGAIN
					 //LOG_TRACE
				}
				break;
			}
			else{
				int optval = 1;
				setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));

				char buf[32];
				inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(sizeof buf));
				auto newCon = new TcpConnection(connfd, buf, be16toh(addr.sin_port));
				_beginWatchReadEvent(newCon);
				_onNewConnection(newCon);
			}
		}
	}
}

void TcpServer::_onNewConnection(TcpConnection* newCon)
{
	cout << "new con " << newCon->getIp() << ":"<< newCon->getPort() << endl;
}

void TcpServer::_beginWatchReadEvent(TcpConnection* con)
{
	int fd;
	if (con == nullptr){
		fd = listenFd_;
	}
	else{
		fd = con->getFd();
	}
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = EPOLLIN;
	event.data.ptr = con;//让操作系统持有了con对象的指针,则必须在con销毁时保证对应的fd已关闭
	epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event);
	++curEpollFdNum_;
}

void TcpServer::_watchWriteEvent(TcpConnection* con, bool bWatch)
{
	if (con == nullptr){
		//LogError(...)
		return;
	}
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = bWatch ? (EPOLLIN | EPOLLOUT) : EPOLLIN;
	event.data.ptr = con->getFd();
	epoll_ctl(epollFd_, EPOLL_CTL_MOD, con->getFd(), &event);
}
