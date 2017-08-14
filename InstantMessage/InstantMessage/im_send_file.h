#pragma once

#include <WinSock2.h>

struct FileTPService
{
	SOCKADDR_IN addr;
	SOCKET sock;
};

struct SendFileService
{
	SOCKADDR_IN addr;
	char *content;
};

void start_listen_file();

void send_file_to(char *szFileName, SOCKADDR_IN dest);

DWORD CALLBACK FileListenerThread(LPVOID lpVoid);
DWORD CALLBACK FileRcvThread(LPVOID lpVoid);
DWORD CALLBACK SendFileThread(LPVOID lpVoid);
