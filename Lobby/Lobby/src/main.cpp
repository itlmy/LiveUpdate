#include "main.h"
#include <stdio.h>
#include "AntiMultiOpen.h"
#include "tlhelp32.h"
#include <shellapi.h>
#include <string>
//#include "..\app\AppDelegate.h"
//#include "..\..\utility\src\utility.h"
//#include "base\CCDirector.h"
//#include "base\ZipUtils.h"

// uncomment below line, open debug console
//#define USE_WIN32_CONSOLE
#define LAUNCHER "Launcher.exe"
#define LAUNCHER_NAME "launcher"
#define LAUNCHER_NAME_UNICODE L"launcher"
TCHAR szCurrentDir[MAX_PATH] = { 0 };
HANDLE g_hCopyFileEvent = NULL;
typedef void (*EntryFunc)();
int run_update(const char* _name, const char* _subDir, BOOL _terminate_host);

bool is_file_exist(wchar_t* file_name)
{
	FILE* fp = NULL;
	_wfopen_s(&fp, file_name, L"r");
	if (!fp)
	{
		return false;
	}

	fclose(fp);
	return true;
}

DWORD CALLBACK CopyFileRoutine(
	_In_     LARGE_INTEGER TotalFileSize,
	_In_     LARGE_INTEGER TotalBytesTransferred,
	_In_     LARGE_INTEGER StreamSize,
	_In_     LARGE_INTEGER StreamBytesTransferred,
	_In_     DWORD         dwStreamNumber,
	_In_     DWORD         dwCallbackReason,
	_In_     HANDLE        hSourceFile,
	_In_     HANDLE        hDestinationFile,
	_In_opt_ LPVOID        lpData
	)
{
	if (TotalFileSize.QuadPart == TotalBytesTransferred.QuadPart)
	{
		
		

		ResetEvent(g_hCopyFileEvent);
		
	}

	return PROGRESS_CONTINUE;
}

void moveLiveUpdate()
{
	bool ret = CopyFileEx(L"./download/LiveUpdate.exe", L"./LiveUpdate.exe", CopyFileRoutine, NULL, false, COPY_FILE_OPEN_SOURCE_FOR_WRITE);
	if (ret)
	{
		g_hCopyFileEvent = CreateEvent(NULL, true, true, L"copy_file_event");
		WaitForSingleObject(g_hCopyFileEvent, INFINITE);

		bool ret = DeleteFile(L"./download/LiveUpdate.exe");
		if (!ret)
		{
			char error[128];
			sprintf_s(error, "Delete LiveUpdate.exe failed.Error Code:%d", GetLastError());
			MessageBoxA(0, error, 0, 0);
		}
	}
	else
	{
		// LiveUpdate.exe is not dead,wait 10ms and retry.
		Sleep(10);
		ResetEvent(g_hCopyFileEvent);
		CloseHandle(g_hCopyFileEvent);
		moveLiveUpdate();
	}
}

DWORD GetPidByProcessName(TCHAR* pProcess)
{
	HANDLE hsnapshot;
	PROCESSENTRY32 lppe;
	hsnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hsnapshot == NULL)
	{
		return 0;
	}
	lppe.dwSize = sizeof(lppe);
	if (!::Process32First(hsnapshot, &lppe))
	{
		return false;
	}
	do 
	{
		TCHAR strprocess[128] = {};
		wcscat(strprocess, lppe.szExeFile);
		if (_tcscmp(strprocess, pProcess) == 0)
		{
			return lppe.th32ProcessID;
		}
	} while (::Process32Next(hsnapshot, &lppe));
	return 0;
}

HWND GetHwndByPid(DWORD dwProcessID)
{
	HWND hWnd = ::GetTopWindow(0);
	while (hWnd)
	{
		DWORD pid = 0;
		DWORD dwTheardID = ::GetWindowThreadProcessId(hWnd, &pid);
		if (dwTheardID != 0)
		{
			if (pid == dwProcessID)
			{
				return hWnd;
			}
		}
		hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);
	}
	return hWnd;
}

void ShowForeGround(HWND hWnd)
{
	if (hWnd)
	{
		::ShowWindow(hWnd, SW_NORMAL);
		::SetForegroundWindow(hWnd);
		::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else
	{
		MessageBox(NULL, L"未能找到该窗口", L"Error", MB_OK);
	}
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
#ifdef _DEBUG
	MessageBoxA(0, "launcher", 0, 0);
#endif
	LPWSTR *szArgList;
	int argCount;

	bool is_restart_lobby = false;
	bool is_restart_lobby_by_liveupdate = false;
	szArgList = CommandLineToArgvW(lpCmdLine, &argCount);
	if (szArgList != NULL)
	{
		std::wstring wstrPID;
		for (int i = 0; i < argCount; i++)
		{
			std::wstring str = szArgList[i];
			std::wstring::size_type idx = str.find(L"restart_pid=");
			if (idx != std::wstring::npos)
			{
				wstrPID = str.substr(idx + strlen("restart_pid="));
			}
			idx = str.find(L"-restart");
			if (idx != std::wstring::npos)
			{
				is_restart_lobby_by_liveupdate = true;
			}
			
		}

		if (!wstrPID.empty())
		{
			DWORD pid = _wtoi(wstrPID.c_str());
			HANDLE hLobby = OpenProcess(PROCESS_TERMINATE, false, pid);
			if (hLobby)
			{
				TerminateProcess(hLobby, true);
				is_restart_lobby = true;
			}
		}
	}

	DWORD livepid = GetPidByProcessName(_T("LiveUpdate.exe"));
	if (livepid && !is_restart_lobby_by_liveupdate)
	{
		return 0;
	}

	CAntiMultiOpen muti;
	bool vm = muti.checkVM();
	if (!vm)
	{
		MessageBox(NULL, L"该进程不能在虚拟环境下运行", L"错误", MB_OK);
		//MessageBox("不能在虚拟机环境下运行");
		return 1;
	}

	if (is_restart_lobby)
	{
		muti.minusProcessCount();
	}

	bool mutiOpen = muti.checkMultiOpen();

	if (!mutiOpen )
	{
		DWORD dwPid = GetPidByProcessName(_T(LAUNCHER));
		HWND hWnd = GetHwndByPid(dwPid);
		ShowForeGround(hWnd);
		return 1;
	}

	//MessageBoxA(0, 0, "Lobby", 0);
	//MessageBeep(0);
    UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	// check new patch
	{
		TCHAR strCommond[MAX_PATH];
		memset(strCommond, 0, sizeof(TCHAR)*MAX_PATH);
		wcscat(strCommond, L"LiveUpdate.exe");

		wchar_t wsHostId[MAX_PATH];
		memset(wsHostId, 0, sizeof(wchar_t)*MAX_PATH);
		swprintf(wsHostId, L" host_id=%d", GetCurrentProcessId());

		//wcscat(strCommond, L"127.0.0.1");
		wcscat(strCommond, L" dst=");
		wcscat(strCommond, LAUNCHER_NAME_UNICODE);
		wcscat(strCommond, L" name=大厅更新中");
		wcscat(strCommond, wsHostId);


		if (is_file_exist(L"./download/LiveUpdate.exe"))
		{
			// copy to root 
			moveLiveUpdate();
		}

		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = TRUE;

		
		BOOL rt = CreateProcessW(NULL, strCommond, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
			
		if (WaitForSingleObject(pi.hProcess, INFINITE))
		{
			MessageBoxA(0, "Running LiveUpdate Failed", 0, 0);
		}
		
	}
	
	run_update("XHome","hippo/EHome",FALSE);

	// start main process
	{
		if (is_file_exist(L"./hippo/EHome/XHome.exe"))
		{
			TCHAR strCommond[MAX_PATH];
			memset(strCommond, 0, sizeof(TCHAR)*MAX_PATH);
			wcscat(strCommond, L"./hippo/EHome/XHome.exe");

			STARTUPINFO si = { sizeof(si) };
			PROCESS_INFORMATION pi;
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = TRUE;


			BOOL rt = CreateProcessW(NULL, strCommond, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

			if (WaitForSingleObject(pi.hProcess, INFINITE))
			{
				MessageBoxA(0, "Running LiveUpdate Failed", 0, 0);
			}
		}

		
	}
	/*HMODULE hModule = LoadLibrary(L"libcocos2d.dll");
	EntryFunc func = (EntryFunc)GetProcAddress(hModule, "HTEntry");
	if (func)
	{
		func();
	}
	else
	{
		MessageBoxA(0, "asdasd", 0, 0);
	}*/
	
    return 1;
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

void util_CharToTchar(const char * _char, TCHAR * _tchar)
{
	int iLength;
	//获取字节长度   
	iLength = ::MultiByteToWideChar(CP_ACP, 0, _char, -1, NULL, 0);
	//将tchar值赋给_char    
	::MultiByteToWideChar(CP_ACP, 0, _char, -1, (LPWSTR)_tchar, iLength);
}

int run_update(const char* _name, const char* _subDir, BOOL _terminate_host)
{
	wchar_t wsLiveUpdate[MAX_PATH];
	memset(wsLiveUpdate, 0, sizeof(wchar_t)*MAX_PATH);

	wchar_t wsCommond[MAX_PATH];
	memset(wsCommond, 0, sizeof(wchar_t)*MAX_PATH);

	//wchar_t wsPatchIp[MAX_PATH];
	//memset(wsPatchIp, 0, sizeof(wchar_t)*MAX_PATH);

	wchar_t wsSubDir[MAX_PATH];
	memset(wsSubDir, 0, sizeof(wchar_t)*MAX_PATH);

	wchar_t wsGameName[MAX_PATH];
	memset(wsGameName, 0, sizeof(wchar_t)*MAX_PATH);

	wchar_t wsHostId[MAX_PATH];
	memset(wsHostId, 0, sizeof(wchar_t)*MAX_PATH);
	swprintf(wsHostId, L"%d", GetCurrentProcessId());

	//util_CharToTchar(_patch_ip, wsPatchIp);
	util_CharToTchar(_subDir, wsSubDir);
	util_CharToTchar(_name, wsGameName);

	TCHAR* strCurDirW = util_getAppDir();
	wcscat(wsLiveUpdate, strCurDirW);
	wcscat(wsLiveUpdate, L"/");
	wcscat(wsLiveUpdate, L"LiveUpdate.exe");

	wcscat(wsCommond, wsLiveUpdate);
	//wcscat(wsCommond, wsPatchIp);
	wcscat(wsCommond, L" dst=");
	wcscat(wsCommond, wsSubDir);
	wcscat(wsCommond, L" name=");
	wcscat(wsCommond, wsGameName);
	wcscat(wsCommond, L" host_id=");
	wcscat(wsCommond, wsHostId);

	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = TRUE;

	if (_terminate_host)
	{
		
		BOOL rt = CreateProcessW(NULL, wsCommond, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (WaitForSingleObject(pi.hProcess, INFINITE))
		{
			MessageBoxA(0, "Running LiveUpdate Failed", 0, 0);
		}
		//output_debug("run_update:CreateProcessW");
		exit(0);
	}
	else
	{
		BOOL rt = CreateProcessW(NULL, wsCommond, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		if (WaitForSingleObject(pi.hProcess, INFINITE))
		{
			MessageBoxA(0, "Running LiveUpdate Failed", 0, 0);
		}
		//output_debug("run_update:CreateProcessW");
		/*if (!WaitForSingleObject(pi.hProcess, INFINITE))
		{
		return 1;
		}*/
	}

	return 1;
}