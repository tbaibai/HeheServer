/*
 * TcpConnection.cpp
 *
 *  Created on: Dec 7, 2016
 *	  Author: lkl
 */

#include "TcpConnection.h"
#include "LinuxHeads.h"
using namespace std;
static const int kSocketBufSize = 64 * 1024;//linux系统默认socket缓冲区大小

TcpConnection::~TcpConnection() {
	// TODO Auto-generated destructor stub
}

void TcpConnection::beginWatchReadEvent()
{
	watchEvents_ = EPOLLIN;
	_modEpollEvent(EPOLL_CTL_ADD);
}

void TcpConnection::_watchWriteEvent(bool bWatch)
{
	if (bWatch){
		watchEvents_ |= EPOLLOUT;
	}
	else{
		watchEvents_ &= (~EPOLLOUT);
	}
	_modEpollEvent(EPOLL_CTL_MOD);
}

void TcpConnection::_watchReadEvent(bool bWatch)
{
	if (bWatch){
		watchEvents_ |= EPOLLIN;
	}
	else{
		watchEvents_ &= (~EPOLLIN);
	}
	_modEpollEvent(EPOLL_CTL_MOD);
}

void TcpConnection::_modEpollEvent(int epollOpt)
{
	//不需要主动调用EPOLL_CTL_DEL操作,close调用时内核会自动移除监听
	struct epoll_event event;
	bzero(&event, sizeof event);
	event.events = watchEvents_;
	event.data.ptr = this;//让操作系统持有了con对象的指针,则必须在con销毁时保证对应的fd已关闭
	epoll_ctl(epollFd_, epollOpt, fd_, &event);
}

void TcpConnection::handleEpollEvent(bool canRead, bool canWrite){
	if (canRead){//读数据至对应的buffer； 处理错误
		while(true){
			uint8_t buf[kSocketBufSize];
			ssize_t nread = read(fd_, buf, kSocketBufSize);
			if (nread > 0){
				uint8_t* data = (uint8_t*)malloc(nread);
				memcpy(data, buf, nread);
				BufferNode data_node;
				data_node.data = data;
				data_node.size = nread;
				//放入生产-消费队列 (必须限制队列的节点数量,以免被客户端异常大量发包耗尽内存)
				bool ret = readBufQueue_.push(data_node);
				assert(ret);
				if (readBufQueue_.isFull()){
					_watchReadEvent(false);
					break;
				}
			}
			else if (nread == 0) {//客户端关闭连接
				close(fd_);
				fd_ = -1;
				break;
			}
			else if (nread == -1) {
				if (errno == EAGAIN) {//参考redis,无视EINTR (对于非阻塞可能永远不会遇到EINTR)
					break;//数据读完了
				}
				else {//出错
					close(fd_);
					fd_ = -1;
					break;
				}
			}
		}
	}
	if (canWrite && fd_ != -1){//发送buffer的数据； 处理错误 fd可能在前面处理canRead时被关闭置-1了
		//TODO 取队列的节点,写数据,直至写满或者写完 写完时要关闭writeEvent监听 (主线程放数据到队列时发现队列空,则还需开启writeEvent监听)
		while(true){
			uint8_t* buf;
			size_t size;
			ssize_t nWrite = write(fd_, buf, size);
			if(nWrite > 0){
				if (nWrite < size){//算是一种优化,节省下一次返回EAGAIN的write调用
					//TODO 调整该节点的offset
					break;
				}
				else{
					//TODO 移除该节点
				}
			}
			else if(nWrite == 0){
				//redis不处理此处,单纯视作没空间可写 可以考虑打一条log
				//网上也有说此处应当关闭连接
				break;
			}
			else if(nWrite == -1)
			{
				if (errno == EAGAIN){
					break;
				}
				else if(errno == EINTR){
					break;//redis与处理EAGIN一致 可以考虑打一条log
				}
				else{//出错
					close(fd_);
					fd_ = -1;
					break;
				}
			}
		}
	}
}

