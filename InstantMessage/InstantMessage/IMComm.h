#pragma once

#include <vector>
#include <WinSock2.h>
#include <list>
#include <algorithm>
#include <Windows.h>
#include <string.h>
#include <cstdio>
#include <errno.h>
#include <time.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

//定义信息结构体
struct nameIP{
	nameIP(): checked(false){}
	unsigned short int sin_port;
	IN_ADDR addrIP;
	char hostname[256];
	clock_t last;
	bool checked;
};

//定义find_if元素查找
typedef struct finder_t
{
	finder_t(IN_ADDR n) : addrIP(n) {}
	bool operator()(nameIP p){
		return (addrIP.S_un.S_addr == p.addrIP.S_un.S_addr); 
	}IN_ADDR addrIP;
}finder_t;

class IMComm
{
private:
	HANDLE m_hMainThread;
public:
	IMComm();
	~IMComm();

	void sendMessageTo(nameIP, std::string);
	bool hasIp(vector<nameIP> otherAddrs, IN_ADDR addrIP);
	//获取局域网内IP
	void getOtherAddr(vector<nameIP> &otherAddrs);
	void sendOtherAddr(vector<nameIP> otherAddrs);
	
	const static UINT16 TCP_PORT = 10000;
	const static int PORT = 9999;
	vector<struct nameIP> otherAddrs;
	//本机名
	char localName[256];
	//本机IP
	IN_ADDR localIP;
	//发送，填写sockaddr_in 结构
	struct sockaddr_in addr;
	//接受
	struct sockaddr_in localaddr;

	void m_send(std::string);

	void setChecked(int index, bool state) { this->otherAddrs.at(index).checked = true; }

	std::string history;

};

