#include "stdafx.h"
#include "im_send_file.h"

#include <WinSock2.h>
#include <cstdio>
#include <CommDlg.h>


#pragma comment(lib, "ws2_32.lib")

DWORD CALLBACK FileRcvThread(LPVOID lpVoid);
DWORD CALLBACK FileListenerThread(LPVOID lpVoid);

void start_listen_file()
{
	CreateThread(0, 0, FileListenerThread, NULL, 0, NULL);
}

DWORD CALLBACK SendFileThread(LPVOID lpVoid)
{
	SendFileService *sfs = (SendFileService *)lpVoid;
	char *szFileName = sfs->content;
	SOCKADDR_IN dest = sfs->addr;
	delete sfs;
	WSADATA wsd;
	WSAStartup(MAKEWORD(2, 2), &wsd);

	SOCKET send_file_sock = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN dest_addr;
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr = dest.sin_addr;
	dest_addr.sin_port = dest.sin_port;

	int ret;
	FILE *file_to_be_sent;
	fopen_s(&file_to_be_sent, szFileName, "r");
	if (file_to_be_sent == NULL)
	{
		MessageBoxA(NULL, "文件不存在！", "", MB_OK);
		fclose(file_to_be_sent);
		closesocket(send_file_sock);
		WSACleanup();
		return 0;
	}

	ret = connect(send_file_sock, (SOCKADDR*)&dest_addr, sizeof(SOCKADDR));
	OutputDebugStringA("Sending...\n");
	if (ret == SOCKET_ERROR)
	{
		MessageBoxA(NULL, "无法连接到目标主机！", "", MB_OK);
		fclose(file_to_be_sent);
		closesocket(send_file_sock);
		WSACleanup();
		return 0;
	}

	string szPureFileName(szFileName);
	int p = szPureFileName.find_last_of('\\', 0) + 1;
	szPureFileName = string(szPureFileName.begin() + p, szPureFileName.end());
	char *cszPureFileName = new char[szPureFileName.size() + 1];
	strcpy(cszPureFileName, szPureFileName.c_str());
	cszPureFileName[szPureFileName.size()] = '\0';
	char l = strlen(cszPureFileName);
	send(send_file_sock, &l, sizeof(char), 0);
	send(send_file_sock, cszPureFileName, l, 0);

	char buf[2048];
	int len = 0;
	while (true)
	{

		len = fread(buf, sizeof(unsigned char), sizeof(buf), file_to_be_sent);
		
		if (len <=0 || len > 2048)
		{

			MessageBoxA(NULL, "文件发送完毕!", "", MB_OK);
			break;
		}
		ret = send(send_file_sock, buf, len, 0);
	}
	delete[] cszPureFileName;
	//delete[] szFileName;
	fclose(file_to_be_sent);
	closesocket(send_file_sock);
	WSACleanup();
}

void send_file_to(char *szFileName, SOCKADDR_IN dest)
{
	SendFileService *s = new SendFileService;
	s->addr = dest;
	s->content = new char[strlen(szFileName)];
	strcpy(s->content, szFileName);
	CreateThread(0, 0, SendFileThread, s, 0, 0);
}

DWORD CALLBACK FileRcvThread(LPVOID lpVoid)
{
	FileTPService *msg = (FileTPService *)lpVoid;

	OPENFILENAMEA opfn={0};
	char szName[256] = {0};
	//ZeroMemory(&opfn, sizeof(opfn));
	opfn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	//opfn.hwndOwner = NULL;
	opfn.lpstrFile = szName;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	opfn.lpstrFile[0] = '\0';
	opfn.nMaxFile = MAX_PATH;
	opfn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	opfn.nFilterIndex = 0;
	opfn.lpstrFileTitle = NULL;
	opfn.nMaxFileTitle = 0;
	opfn.lpstrInitialDir = "D:\\";
	opfn.Flags = 0;
	
	
	char file_buf[2048];
	int len = 0;

	memset(szName, 0, sizeof(szName));
	char cFileNameLength = 0;
	recv(msg->sock, &cFileNameLength, sizeof(char), 0);
	recv(msg->sock, szName, cFileNameLength, 0);
	char debug_text[4096];
	sprintf(debug_text, "szName length: %d %s\n", strlen(szName), szName);
	OutputDebugStringA(debug_text);
	if (GetSaveFileNameA(&opfn)==0)
	{
		OutputDebugStringA(szName);
		OutputDebugStringA("不要了\n");
		
		delete msg;
		closesocket(msg->sock);
		WSACleanup();
		return 1;
	}
	
	FILE *fileSaved = fopen(szName, "w");


	while (true)
	{
		memset(file_buf, 0, sizeof(file_buf));
		len = recv(msg->sock, file_buf, sizeof(file_buf), 0);

		if (len <=0 || len >sizeof(file_buf))
		{
			MessageBoxA(NULL, "文件接收完毕!", "", MB_OK);
			break;
		}
		fwrite(file_buf, 1, len, fileSaved);
	}
	fclose(fileSaved);
	closesocket(msg->sock);
	WSACleanup();
	delete msg;
	return 0;
}

DWORD CALLBACK FileListenerThread(LPVOID lpVoid)
{
	WSADATA wsd;
	WSAStartup(MAKEWORD(2, 2), &wsd);

	SOCKET rcv_file_sock = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN local_listener;
	local_listener.sin_family = AF_INET;
	local_listener.sin_port = htons(12345);
	local_listener.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(rcv_file_sock, (SOCKADDR*)&local_listener, sizeof(SOCKADDR_IN)) < 0)
	{
		MessageBoxA(NULL, "完了", "", MB_OK);
		exit(1);
	}

	listen(rcv_file_sock, 3);
	SOCKADDR_IN client_addr;
	SOCKET service_sock;
	while (true)
	{
		OutputDebugStringA("File comming...");
		int len = sizeof(SOCKADDR);
		service_sock = accept(rcv_file_sock, (SOCKADDR*)&client_addr, &len);
		OutputDebugStringA("Someone connected.\n");
		struct FileTPService *msg = new FileTPService;

		msg->addr = client_addr;
		msg->sock = service_sock;

		CreateThread(0, 0, FileRcvThread, msg, 0, NULL);
		
	}

	return 0;
}