#ifdef __cplusplus                     
extern "C" {
#endif

#include <stdio.h>
#include "windows.h"

#define MAX_LOADSTRING 100
//TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
//TCHAR szWindowClass ="HTG-FlashPlayer";			// 主窗口类名
TCHAR szCurrentDir[MAX_PATH] = { 0 };
char* g_strLobbyWinName = NULL;

char* getLobbyWinName()
{
	if (!g_strLobbyWinName)
	{
		g_strLobbyWinName = new char[MAX_PATH];
		sprintf(g_strLobbyWinName, "HTLobby_%d", GetCurrentProcessId());
	}

	return g_strLobbyWinName;
}

HWND getLobbyWinHandle()
{
	return FindWindowA(getLobbyWinName(), NULL);
}

HWND getLobbyWinHandleByProcessId(DWORD processId)
{
	char strWinClassName[MAX_PATH];
	sprintf(strWinClassName, "HTLobby_%d", processId);
	return FindWindowA(strWinClassName, NULL);
}

TCHAR* util_getAppDir()
{
	if (wcslen(szCurrentDir))
	{
		//wcslen()
		return szCurrentDir;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);
	GetModuleFileName(hInstance, szCurrentDir, MAX_PATH);
	LPTSTR lpInsertPos = wcsrchr(szCurrentDir, L'\\');
	//wcsrchr()
	*lpInsertPos = 0;
	return szCurrentDir;
}

void util_TcharToChar(const TCHAR * tchar, char * _char)
{
	int iLength;
	//获取字节长度   
	iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
	//将tchar值赋给_char    
	WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
}

void util_CharToTchar(const char * _char, TCHAR * _tchar)
{
	int iLength;
	//获取字节长度   
	iLength = ::MultiByteToWideChar(CP_ACP, 0, _char, -1, NULL, 0);
	//将tchar值赋给_char    
	::MultiByteToWideChar(CP_ACP, 0, _char, -1, (LPWSTR)_tchar, iLength);
}

void util_Utf8ToUnicode(const char * _char, TCHAR * _tchar)
{
	int len = ::MultiByteToWideChar(CP_UTF8, 0, _char, -1, NULL, 0);
	::MultiByteToWideChar(CP_UTF8, 0, _char, -1, _tchar, len);
}

void util_UnicodeToUtf8(const TCHAR * tchar, char * _char)
{
	int len = ::WideCharToMultiByte(CP_UTF8, 0, tchar, -1, NULL, 0, NULL, NULL);
	::WideCharToMultiByte(CP_UTF8, 0, tchar, -1, _char, len, NULL, NULL);
}

int util_win_minimize()
{
	HWND hWnd = getLobbyWinHandle();
	SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return 1;
}

int util_win_activate()
{
	HWND hWnd = getLobbyWinHandle();
	SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	return 1;
}

TCHAR* util_get_open_file()
{
	static TCHAR szFilter[] = TEXT("所有图片文件\0*.bmp;*.dib;*.jpg;*.jpeg;*.jpe;*.tiff;*.png;*.ico\0")  \
		TEXT("JPEG文件 (*.jpg;*.jpeg;*.jpe)\0*.jpg;*.jpeg;*.jpe\0") \
		TEXT("位图文件 (*.bmp;*.dib)\0*.bmp;*.dib\0") \
		TEXT("TIFF (*.tiff)\0*.tiff\0") \
		TEXT("PNG (*.png)\0*.png\0") \
		TEXT("ICO (*.ico)\0*.ico\0\0");

	HWND hWnd = getLobbyWinHandle();
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = 0;             // Set in Open and Close functions  
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = TEXT("jpg");
	ofn.lCustData = 0L;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	ofn.Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	
	TCHAR*  szFileName = (TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
	//TCHAR*	szTitleName = (TCHAR*)malloc(sizeof(TCHAR)*MAX_PATH);
	memset(szFileName, 0, sizeof(TCHAR)*MAX_PATH);
	//memset(szTitleName, 0, sizeof(TCHAR)*MAX_PATH);
	ofn.lpstrFile = szFileName;
	ofn.lpstrFileTitle = NULL;

	bool ret = GetOpenFileName(&ofn);
	HRESULT hr = GetLastError();

	return szFileName;
}

int util_win_close()
{
	PostQuitMessage(0);
	return 1;
}

int util_windowLayoutCenter()
{
	HWND hWnd = getLobbyWinHandle();
	int scrWidth, scrHeight;
	RECT rect;
	//RECT dstRect;

	//get screen size
	scrWidth = GetSystemMetrics(SM_CXSCREEN);
	scrHeight = GetSystemMetrics(SM_CYSCREEN);

	//get window size
	GetWindowRect(hWnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	int xPos = (scrWidth - width) / 2;
	int yPos = (scrHeight - height) / 2;

	//set window pos
	SetWindowPos(hWnd, HWND_TOP, xPos, yPos, width, height, SWP_SHOWWINDOW);
	return 1;
}



#ifdef __cplusplus
}
#endif