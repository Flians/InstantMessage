// InstantMessage.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "InstantMessage.h"
#include "IMComm.h"
#include <Commctrl.h>
#include <commdlg.h>
#include <string>
#include "im_send_file.h"

#include <map>

#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
static HWND showMe;
static HWND inputMe;
static HWND bMessage;
static HWND showIP;
static HWND inputFile;
static HWND bFile;
HANDLE hSThread;
HANDLE hRThread;
DWORD SThreadID;
DWORD RThreadID;
static bool ischeck;

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void listView();
void ThreadSend();
void ThreadRec();
void charTowchar(const char *chr, wchar_t *wchar, int size);
void checkBox();

IMComm *imcomm = NULL;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此放置代码。

	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_INSTANTMESSAGE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_INSTANTMESSAGE));

	MSG msg;

	// 主消息循环: 
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_INSTANTMESSAGE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_INSTANTMESSAGE);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	imcomm = new IMComm();
	return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	start_listen_file();
	ischeck = false;
	hInst = hInstance; // 将实例句柄存储在全局变量中

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, 930, 680, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

void DrawUI(HWND &hWnd)
{
	showMe = CreateWindowExA(0, "Edit", "", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL|ES_LEFT|ES_WANTRETURN, 10, 10, 550, 360, hWnd, (HMENU)IDC_GETTER, hInst, NULL);
	inputMe = CreateWindowExA(0, "Edit", "", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL|ES_LEFT|ES_WANTRETURN, 10, 380, 550, 180, hWnd, (HMENU)IDC_SENDER, hInst, NULL);
	bMessage = CreateWindowExA(0, "Button", "发送", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 480, 570, 80, 30, hWnd, (HMENU)IDC_BUTTON, hInst, NULL);
	showIP = CreateWindowExA(0, "SysListView32", "", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL| WS_VSCROLL, 580, 10, 310, 600, hWnd, (HMENU)IDC_USER_LIST, hInst, NULL);
	ListView_SetExtendedListViewStyle(showIP, ListView_GetExtendedListViewStyle(showIP) | LVS_EX_CHECKBOXES| LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES | LVS_EX_CHECKBOXES | LVS_EX_AUTOCHECKSELECT|LVS_SHOWSELALWAYS);
	
	//设置ListView的列
	LVCOLUMN vcl;
	vcl.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	vcl.pszText = L"A";
	vcl.iSubItem = 0;
	vcl.cx = 20;
	ListView_InsertColumn(showIP, 0, &vcl);
	// 第一列
	vcl.pszText = L"主机名";//列标题
	vcl.cx = 200;//列宽
	vcl.iSubItem = 1;//子项索引，第一列无子项
	ListView_InsertColumn(showIP, 1, &vcl);
	// 第二列
	vcl.pszText = L"IP地址";//列标题
	vcl.cx = 110;//列宽
	vcl.iSubItem = 2;//子项索引，第一列无子项
	ListView_InsertColumn(showIP, 2, &vcl);

}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		DrawUI(hWnd);
		SendMessage(GetDlgItem(hWnd, IDC_GETTER), EM_SETREADONLY, TRUE, 0);
		EndPaint(hWnd, &ps);

		//开启收广播线程
		hRThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadRec, NULL, 0, &RThreadID);
		//开启发广播线程
		hSThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadSend, NULL, 0, &SThreadID);

		return 0;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDC_BUTTON:
		{
			char textToBeSent[2048];
			GetDlgItemTextA(hWnd, IDC_SENDER, textToBeSent, 2048);
			std::string s(textToBeSent);
			imcomm->m_send(s);
			listView();
			//清空
			SetWindowTextA(inputMe, "");
			SetWindowTextA(showMe, imcomm->history.c_str());
			break;
		}
		case ID_FILE_SEND_FILE:
			{
				OPENFILENAMEA opfn={0};
				char szName[256];
				ZeroMemory(&opfn, sizeof(opfn));
				opfn.lStructSize = sizeof(opfn);
				opfn.hwndOwner = NULL;
				opfn.lpstrFile = szName;
				// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
				// use the contents of szFile to initialize itself.
				opfn.lpstrFile[0] = '\0';
				opfn.nMaxFile = sizeof(szName);
				opfn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
				opfn.nFilterIndex = 1;
				opfn.lpstrFileTitle = NULL;
				opfn.nMaxFileTitle = 0;
				opfn.lpstrInitialDir = NULL;
				opfn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
				if (GetOpenFileNameA(&opfn))
				{
					char ip[64];
					char port[16];
					SOCKADDR_IN dest_addr;
					vector<nameIP>::iterator itor;
					//从vector第一个iterator开始
					itor = imcomm->otherAddrs.begin();
					while (itor != imcomm->otherAddrs.end())
					{
						if (itor->checked)
						{
							SOCKADDR_IN teso = {0};
							teso.sin_addr = itor->addrIP;
							teso.sin_port = htons(12345);
							string tempp = "To ";
							tempp.append(itor->hostname);
							MessageBoxA(NULL, szName, tempp.c_str(), MB_OK);
							send_file_to(szName, teso);
						}
						itor++;
					}
				}
			}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_NOTIFY:
	{
		switch(LOWORD(wParam)){
		case IDC_USER_LIST:
			int iItem = ((LPNMITEMACTIVATE)lParam)->iItem;
			int iSubItem=((LPNMITEMACTIVATE)lParam)->iSubItem;
			if(((LPNMHDR)lParam)->code == LVN_COLUMNCLICK){
				if (iSubItem == 0)
				{
					if(ischeck == false){
						ischeck = true;
					}else{
						ischeck = false;
					}
					int itemNumbers = ListView_GetItemCount(showIP);
					for (int i = 0; i < itemNumbers; ++i)
					{
						ListView_SetCheckState(showIP, i, ischeck);
					}
					vector<nameIP>::iterator itor;
					//从vector第一个iterator开始
					itor = imcomm->otherAddrs.begin();
					while (itor != imcomm->otherAddrs.end())
					{
						itor->checked = ischeck;
						itor++;
					}
				}
			}else if(((LPNMHDR)lParam)->code == NM_CLICK){
				if(iSubItem == 0){
					if(ischeck == false){
						ischeck = true;
					}else{
						ischeck = false;
					}
					ListView_SetCheckState(showIP, iItem, ischeck);
					checkBox();
					listView();
				}
			}
			break;
		}
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		CloseHandle(hRThread);
		CloseHandle(hSThread);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

//将宽字节wchar_t*转化为单字节char*  
inline char* UnicodeToAnsi(const wchar_t* szStr)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
	{
		return NULL;
	}
	char* pResult = new char[nLen];
	WideCharToMultiByte(CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL);
	return pResult;
}

void checkBox(){
	vector<string> hist;
	int itemNumbers = ListView_GetItemCount(showIP);
	for (int i = 0; i < itemNumbers; ++i)
	{
		if (ListView_GetCheckState(showIP, i))
		{
			WCHAR text[64];
			ListView_GetItemText(showIP, i, 1, text, 64);
			char *trans = UnicodeToAnsi(text);

			charTowchar(trans, text, sizeof(text));
			hist.push_back(trans);
		}
	}
	vector<nameIP>::iterator itor;
	//从vector第一个iterator开始
	itor = imcomm->otherAddrs.begin();
	while (itor != imcomm->otherAddrs.end())
	{
		vector<string>::iterator result = find(hist.begin(), hist.end(), itor->hostname);
		if (result != hist.end())
		{
			itor->checked = true;
		}else{
			itor->checked = false;
		}
		itor++;
	}
}


void listView() {
	//SetDlgItemTextA(showMe, IDC_GETTER, imcomm->history.c_str());
	SetWindowTextA(showMe, imcomm->history.c_str());
	SendMessage(showIP, LVM_DELETEALLITEMS, 0, 0);
	vector<nameIP>::iterator itor;
	//从vector第一个iterator开始
	itor = imcomm->otherAddrs.begin();
	int i = 0;
	char text[100];
	sprintf(text, "%d user(s) found.\n", imcomm->otherAddrs.size());
	OutputDebugStringA(text);
	while (itor != imcomm->otherAddrs.end())
	{
		LVITEM vitem = { 0 };
		vitem.mask = LVIF_TEXT;
		vitem.iItem = i;
		ListView_InsertItem(showIP, &vitem);
		
		wchar_t wchar[256];
		charTowchar(itor->hostname, wchar, sizeof(wchar));
		vitem.pszText = wchar;
		vitem.iSubItem = 1;
		ListView_SetItem(showIP, &vitem);

		vitem.iSubItem = 2;
		memset(wchar, 0, sizeof(wchar));
		charTowchar(inet_ntoa(itor->addrIP), wchar, sizeof(wchar));
		vitem.pszText = wchar;
		ListView_SetItem(showIP, &vitem);
		if (itor->checked)
		{
			ListView_SetCheckState(showIP, i, true);
		}
		int countItem = ListView_GetItemCount(showIP);
		sprintf(text, "%d\n", countItem);
		OutputDebugStringA(text);
		i++;
		itor++;
	}
}
void ThreadSend() {
	while (true) {
		//广播查找IP
		imcomm->sendOtherAddr(imcomm->otherAddrs);
		Sleep(1000);
	}
}

void ThreadRec() {
	vector<nameIP> tt = imcomm->otherAddrs;
	while (true) {
		//接收广播IP
		imcomm->getOtherAddr(imcomm->otherAddrs);
		listView();
	}
}

void charTowchar(const char *chr, wchar_t *wchar, int size)
{
	MultiByteToWideChar(CP_ACP, 0, chr, strlen(chr) + 1, wchar, size / sizeof(wchar[0]));
}