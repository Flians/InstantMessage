#include "stdafx.h"
#include "IMComm.h"

#include <sstream>

DWORD CALLBACK MainListner(LPVOID lParam)
{

	IMComm *_this = (IMComm *)lParam;
	WSADATA wsd;
	int ret = 0;
	ret = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (ret != 0)
	{
		MessageBoxA(NULL, "初始化失败！", "错误", MB_OK);
		exit(1);
	}

	SOCKET local_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (local_socket == INVALID_SOCKET)
	{
		MessageBoxA(NULL, "初始化失败！", "错误", MB_OK);
		exit(1);
	}

	SOCKADDR_IN s_in;
	s_in.sin_family = AF_INET;
	s_in.sin_port = IMComm::TCP_PORT;
	s_in.sin_addr.s_addr = INADDR_ANY;

	if (bind(local_socket, (SOCKADDR*)&s_in, sizeof(SOCKADDR_IN)))
	{
		MessageBoxA(NULL, "初始化失败！", "错误", MB_OK);
		exit(1);
	}

	if (listen(local_socket, 10) == SOCKET_ERROR)
	{
		MessageBoxA(NULL, "初始化失败！", "错误", MB_OK);
		exit(1);
	}

	SOCKET client_sock;
	SOCKADDR_IN client_addr;
	int nAddrLen = sizeof(client_addr);
	char rcv_buf[2048];

	while (true)
	{
		OutputDebugStringA("Waiting...\n");
		client_sock = accept(local_socket, (SOCKADDR*)&client_addr, &nAddrLen);
		OutputDebugStringA("Someone connected.\n");
		if (client_sock == INVALID_SOCKET)
		{
			continue;
		}
		memset(rcv_buf, 0, sizeof(rcv_buf));
		int ret = recv(client_sock, rcv_buf, sizeof(rcv_buf), 0);
		if (ret > 0)
		{
			_this->history.append(rcv_buf);
		}
		OutputDebugStringA("Raw message: \n");
		OutputDebugStringA(rcv_buf);
		OutputDebugStringA("\n");
		OutputDebugStringA(_this->history.c_str());
	}

}


IMComm::IMComm()
{
	//发送
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	addr.sin_port = htons(PORT);

	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		printf("error:WSA启动失败!\n");
		WSACleanup();
		exit(1);
	}
	memset(&localName, 0, sizeof(localName));
	memset(&localIP, 0, sizeof(localIP));
	//获取主机名
	if (gethostname(localName, sizeof(localName)) == SOCKET_ERROR)
	{
		printf("error:本机名获取失败\n");
		exit(1);
	}
	//获取主机ip
	HOSTENT* host = gethostbyname(localName);
	if (host == NULL)
	{
		printf("error:本机IP获取失败\n");
		exit(1);
	}
	//添加本机信息
	nameIP snameIp;
	for (int i = 0; host->h_addr_list[i]; i++) {
		snameIp.sin_port = htons(PORT);
		localIP = *((IN_ADDR *)host->h_addr_list[i]);
		snameIp.addrIP = localIP;
		strcpy(snameIp.hostname, localName);
	}
	printf("<<<<<<<<<<<<<<<本机名: %s 本机IP: %s>>>>>>>>>>>>>>>>>\n", localName, inet_ntoa(localIP));
	otherAddrs.push_back(snameIp);
	memset(&snameIp, 0, sizeof(snameIp));
	WSACleanup();
	m_hMainThread = CreateThread(0, 0, MainListner, this, 0, 0);
}


IMComm::~IMComm()
{
	memset(&addr, 0, sizeof(addr));
	memset(&localaddr, 0, sizeof(localaddr));
	memset(&otherAddrs, 0, sizeof(otherAddrs));
	CloseHandle(m_hMainThread);
}


void IMComm::sendMessageTo(nameIP tar, std::string msg)
{
	WSADATA wsd;
	int ret;
	ret = WSAStartup(MAKEWORD(2, 2), &wsd);
	SOCKET skt = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	
	SOCKADDR_IN clientsock_in;
	clientsock_in.sin_family = AF_INET;
	clientsock_in.sin_addr = tar.addrIP;//htonl(tar.addrIP.S_un.S_addr);
	clientsock_in.sin_port = IMComm::TCP_PORT;


	if (connect(skt, (SOCKADDR*)&clientsock_in, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		char test[1000];
		sprintf(test, "发送消息时出现了错误！%X: %u\n", clientsock_in.sin_addr.s_addr, clientsock_in.sin_port);
		MessageBoxA(NULL, test, "", MB_OK);
		closesocket(skt);
		WSACleanup();
		return;
	}
	const char *sentBuf = msg.c_str();
	std::string datagram;
	datagram.append("From ");
	datagram.append(localName);
	datagram.append(":\r\n");
	datagram.append(msg);
	datagram.append("\r\n");

	//char test[1024];
	//sprintf(test, "Send:%s\n", sentBuf);
	//OutputDebugStringA(test);
	send(skt, datagram.c_str(), datagram.size(), 0);

	closesocket(skt);
	WSACleanup();

}
//获取局域网内IP
void IMComm::getOtherAddr(vector<nameIP> &otherAddrs) {
	//接收
	memset(&localaddr, 0, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = 0;
	localaddr.sin_port = htons(PORT);
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	int ret = 0;
	int sock = 0;
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0) {
		WSACleanup();
		exit(1);
	}
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket建立失败");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	//设置套接字为广播类型,允许发送广播消息
	bool so_broadcast = true;
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
	if (ret < 0) {
		printf("set socket broadcast error:%d\n", errno);
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	//设置套接字 发送/接收缓冲区2K
	const int nSendBuf = 2 * 1024;
	ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&nSendBuf, sizeof(int));
	if (ret < 0)
	{
		printf("setsockopt SO_SNDBUF error\n");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	const int nRcvBuf = 2 * 1024;
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&nRcvBuf, sizeof(int));
	if (ret < 0)
	{
		printf("setsockopt SO_RCVBUF error\n");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}


	//绑定socket，便于接收消息
	if (bind(sock, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
		perror("socket绑定失败");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	char buff[1024] = "";
	int addr_len = sizeof(struct sockaddr_in);
	nameIP snameIp;
	int time = 50;
	//while(time--){
	memset(buff, 0, sizeof(buff) / sizeof(char));
	memset(&snameIp, 0, sizeof(snameIp));

	//从广播地址接收消息，注意用来绑定的地址和接收消息的地址是不一样的
	ret = recvfrom(sock, buff, sizeof(buff), 0, (struct sockaddr *)&localaddr, &addr_len);
	stringstream ss(buff);
	std::string header, port;
	ss >> header;
	OutputDebugStringA(header.c_str());
	OutputDebugStringA("\n");
	if (ret != -1/* && localaddr.sin_addr.S_un.S_addr != localIP.S_un.S_addr*/) {
		//判断添加
		if (header == "_HelloFH_") {
			for (vector<nameIP>::iterator it(otherAddrs.begin()); it != otherAddrs.end() && !otherAddrs.empty(); it++)
			{
				if (it->addrIP.s_addr == localaddr.sin_addr.s_addr)
				{
					it->last = clock() / CLOCKS_PER_SEC;
					goto EXISTED;
				}
			}
			snameIp.addrIP = localaddr.sin_addr;
			snameIp.sin_port = PORT;
			HOSTENT *hostinfo;
			hostinfo = gethostbyaddr((const char*)&localaddr.sin_addr, sizeof(localaddr.sin_addr), AF_INET);
			strcpy(snameIp.hostname, hostinfo->h_name);
			OutputDebugStringA(snameIp.hostname);
			OutputDebugStringA(" sent.\n");
			snameIp.last = clock() / CLOCKS_PER_SEC;
			otherAddrs.push_back(snameIp);
			//显示client端的网络地址和收到的字符串消息
			printf("Received a string from client %s, name:%s, string is: %s\n", inet_ntoa(localaddr.sin_addr), snameIp.hostname, buff);
		EXISTED:
			memset(&snameIp, 0, sizeof(snameIp));

		} 
		else
		{
			OutputDebugStringA("....\n");
		}
	}
	memset(buff, 0, sizeof(buff) / sizeof(char));
	//}
	double time_now = clock() / CLOCKS_PER_SEC;
	for (vector<nameIP>::iterator it = otherAddrs.begin(); it != otherAddrs.end() && !otherAddrs.empty(); )
	{
		if ((time_now - it->last) > 3)
		{
			it = otherAddrs.erase(it);
		}
		else
		{
			it++;
		}
	}
	closesocket(sock);
	WSACleanup();
}

//局域网内发广播IP
void IMComm::sendOtherAddr(vector<nameIP> otherAddrs) {
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	int ret = 0;
	int sock = 0;
	ret = WSAStartup(wVersionRequested, &wsaData);
	if (ret != 0) {
		WSACleanup();
		exit(1);
	}
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket建立失败");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	//设置套接字为广播类型,允许发送广播消息
	bool so_broadcast = true;
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&so_broadcast, sizeof(so_broadcast));
	if (ret < 0) {
		printf("set socket broadcast error:%d\n", errno);
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	//设置套接字 发送/接收缓冲区2K
	const int nSendBuf = 2 * 1024;
	ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&nSendBuf, sizeof(int));
	if (ret < 0)
	{
		printf("setsockopt SO_SNDBUF error\n");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	const int nRcvBuf = 2 * 1024;
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&nRcvBuf, sizeof(int));
	if (ret < 0)
	{
		printf("setsockopt SO_RCVBUF error\n");
		closesocket(sock);
		WSACleanup();
		exit(1);
	}

	char buff[100] = "";
	int addr_len = sizeof(struct sockaddr_in);
	nameIP snameIp;
	strcpy(buff, "_HelloFH_");
	ret = sendto(sock, buff, strlen(buff), 0, (struct sockaddr *)&addr, addr_len);
	if (ret != SOCKET_ERROR) {
		printf("send... %d\n", WSAGetLastError());
	}
	memset(buff, 0, sizeof(buff) / sizeof(char));
	memset(&snameIp, 0, sizeof(snameIp));
	closesocket(sock);
	WSACleanup();
}

void IMComm::m_send(std::string msg)
{
	char text[1024];
	sprintf(text, "There are %d users.\n", otherAddrs.size());
	OutputDebugStringA(text);
	for (vector<nameIP>::iterator i(otherAddrs.begin()); i != otherAddrs.end(); ++i)
	{
		if (i->checked) {
			
			sprintf(text, "Send message to %s, %X\n", i->hostname, i->addrIP.s_addr);
			OutputDebugStringA(text);
			sendMessageTo(*i, msg);
		}
	}
}

//查找IP是否已记录,true已记录
bool IMComm::hasIp(vector<nameIP> otherAddrs, IN_ADDR addrIP) {
	return find_if(otherAddrs.begin(), otherAddrs.end(), finder_t(addrIP)) != otherAddrs.end();
}
