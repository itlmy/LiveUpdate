#include "stdafx.h"
#include "network.h"
#include <curl/curl.h>
#include "utility/src/utility.h"
#include "windows.h"
#include <mmsystem.h>

wchar_t g_szAppDir[256];
fun_finish_cb g_finish_cb = NULL;
fun_process_cb g_process_cb = NULL;
CURL* g_curl = NULL;

char* g_recvData = NULL;
size_t g_recvPos = 0;
size_t g_recvTotal = 0;

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

bool setDownLoadFinishCallback(fun_finish_cb _cb)
{
	g_finish_cb = _cb;
	return true;
}

bool setDownLoadProcessCallback(fun_process_cb _cb)
{
	g_process_cb = _cb;
	return true;
}

bool initNetwork()
{
	/*if (g_curl)
	{
	closeNetwork();
	}*/

	memset(g_szAppDir,0,sizeof(wchar_t)*256);
	wcscpy(g_szAppDir, util_getAppDir());
	//g_curl = curl_easy_init();
	curl_global_init(CURL_GLOBAL_ALL);
	return true;
}

bool closeNetwork()
{
	if (g_curl)
	{
		curl_easy_cleanup(g_curl);
		g_curl = NULL;
	}
	
	return true;
}

double _GetDownloadFileSize(const char* _url, char* error_string)
{
	if (NULL == _url)
		return -1;

	CURL* curl = curl_easy_init();
	if (NULL == curl)
		return -1;

	curl_easy_setopt(curl, CURLOPT_URL, _url);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK)
	{
		double sz = -1;
		res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &sz);
		curl_easy_cleanup(curl);
		
		return sz;
	}
	else
	{
		g_recvTotal = 0;
		g_recvPos = 0;
		const char* pError = curl_easy_strerror(res);
		sprintf(error_string, "%s", pError);
	}

	curl_easy_cleanup(curl);

	return -1;
}

int GetDownloadFileSize(const char* _url)
{
	double rtValue = -1;
	char err_code[MAX_PATH];
	int retry_cnt = 0;
	rtValue = _GetDownloadFileSize(_url, err_code);
	while (rtValue < 0)
	{
		rtValue = _GetDownloadFileSize(_url, err_code);
		retry_cnt++;
		if (retry_cnt >= 2)
		{
			char test[MAX_PATH];
			sprintf(test, "获取文件大小失败:%s", _url);
			HWND hWnd = GetHwndByPid(GetCurrentProcessId());
			ShowForeGround(hWnd);
			MessageBoxA(hWnd, test, err_code, 0);
			return rtValue;
		}
	}

	g_recvTotal = rtValue;
	g_recvPos = 0;
	if (g_recvTotal && rtValue > -0.00001)
	{
		g_recvData = new char[g_recvTotal];
	}

	return rtValue;
}
//bool DownLoadUrl(char* _url)
//{
//	if (!g_curl)
//	{
//		return false;
//	}
//
//	//初始化curl，这个是必须的  
//	curl_easy_setopt(g_curl, CURLOPT_URL, _url);
//	//设置接收数据的回调  
//	curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, g_finish_cb);
//	//curl_easy_setopt(curl, CURLOPT_INFILESIZE, lFileSize);  
//	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);  
//	//curl_easy_setopt(curl, CURLOPT_NOBODY, 1);  
//	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
//	// 设置重定向的最大次数  
//	curl_easy_setopt(g_curl, CURLOPT_MAXREDIRS, 5);
//	// 设置301、302跳转跟随location  
//	curl_easy_setopt(g_curl, CURLOPT_FOLLOWLOCATION, 1);
//	curl_easy_setopt(g_curl, CURLOPT_NOPROGRESS, 0);
//	//设置进度回调函数  
//	curl_easy_setopt(g_curl, CURLOPT_PROGRESSFUNCTION, g_process_cb);
//
//	curl_easy_setopt(g_curl, CURLOPT_NOSIGNAL, 1L);
//
//	curl_easy_setopt(g_curl, CURLOPT_ACCEPT_ENCODING, "");
//	//curl_easy_getinfo(curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);  
//	//curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, g_hDlgWnd);  
//	//开始执行请求  
//	CURLcode retcCode = curl_easy_perform(g_curl);
//	//查看是否有出错信息  
//	const char* pError = curl_easy_strerror(retcCode);
//
//	if (retcCode)
//	{
//		MessageBoxA(0, _url, pError, 0);
//	}
//	
//	return true;
//}

bool _DownLoadUrl(char* _url, double& speed, char* error_string, long resume_pt)
{
	CURL* l_curl = curl_easy_init();

	//初始化curl，这个是必须的  
	curl_easy_setopt(l_curl, CURLOPT_URL, _url);
	//设置接收数据的回调  
	curl_easy_setopt(l_curl, CURLOPT_WRITEFUNCTION, g_finish_cb);
	//curl_easy_setopt(curl, CURLOPT_INFILESIZE, lFileSize);  
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);  
	//curl_easy_setopt(curl, CURLOPT_NOBODY, 1);  
	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
	// 设置重定向的最大次数  
	curl_easy_setopt(l_curl, CURLOPT_MAXREDIRS, 5);
	// 设置301、302跳转跟随location  
	curl_easy_setopt(l_curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(l_curl, CURLOPT_NOPROGRESS, 0);
	//设置进度回调函数  
	curl_easy_setopt(l_curl, CURLOPT_PROGRESSFUNCTION, g_process_cb);

	curl_easy_setopt(l_curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(l_curl, CURLOPT_ACCEPT_ENCODING, "");

	curl_easy_setopt(l_curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(l_curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(l_curl, CURLOPT_CONNECTTIMEOUT, 5);
	curl_easy_setopt(l_curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(l_curl, CURLOPT_RESUME_FROM, resume_pt);
	//curl_easy_getinfo(curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);  
	//curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, g_hDlgWnd); 

	//开始执行请求  
	CURLcode retcCode = curl_easy_perform(l_curl);
	//查看是否有出错信息  
	const char* pError = curl_easy_strerror(retcCode);


	CURLcode code = curl_easy_getinfo(l_curl, CURLINFO_SPEED_DOWNLOAD, &speed);

	curl_easy_cleanup(l_curl);

	if (retcCode)
	{
		sprintf(error_string, "%s", pError);
		return false;
	}

	return true;
}

bool DownLoadUrl(char* _url, double& speed)
{
	bool rtValue = false;
	char err_code[MAX_PATH];
	int retry_cnt = 0;
	rtValue = _DownLoadUrl(_url, speed, err_code, 0);
	while (!rtValue)
	{
		//g_recvTotal = 0;
		//g_recvPos = 0;
		rtValue = _DownLoadUrl(_url, speed, err_code, g_recvPos);
		retry_cnt++;
		if (retry_cnt >= 30)
		{
			char test[MAX_PATH];
			sprintf(test, "下载文件失败:%s", _url);

			HWND hWnd = GetHwndByPid(GetCurrentProcessId());
			ShowForeGround(hWnd);
			MessageBoxA(hWnd, test, err_code, 0);
			break;
		}
		Sleep(10);
	}

	return rtValue;
}