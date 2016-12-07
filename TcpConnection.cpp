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
TcpConnection::TcpConnection() {
	// TODO Auto-generated constructor stub

}

TcpConnection::~TcpConnection() {
	// TODO Auto-generated destructor stub
}

void TcpConnection::handleEpollEvent(bool canRead, bool canWrite){
	if (canRead){//读数据至对应的buffer； 处理错误
		while(true){
			uint8_t buf[kSocketBufSize];
			size_t nread = read(fd_, buf, kSocketBufSize);
			if (nread > 0){
				uint8_t* data = (uint8_t*)malloc(nread);
				memcpy(data, buf, nread);
				auto data_node = new BufferNode();
				data_node->data = data;
				data_node->size = nread;
				//TODO 放入生产-消费队列 (必须限制队列的节点数量,以免被客户端异常大量发包耗尽内存)

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
			size_t nWrite = write(fd_, buf, size);
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

