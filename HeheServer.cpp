//============================================================================
// Name		: HeheServer.cpp
// Author	  : 
// Version	 :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "TcpServer.h"
#include <signal.h>
#include <thread>
#include <unistd.h>
#include "LinuxHeads.h"
using namespace std;

void initSignals()
{
	//忽略SIGPIPE， 另外若有fork子进程，还需监听SIGCHLD
	signal(SIGPIPE, SIG_IGN);

	signal(SIGHUP, SIG_IGN);
}

int main() {
	initSignals();

	cout << "Hehe Server!" << endl;
	TcpServer server(7011);
	server.start();//启动网络线程执行epoll轮询
	while(getchar() != 'q'){
		//TODO game loop
	}
	return 0;
}
