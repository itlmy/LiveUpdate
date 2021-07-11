#include "network.h"
#include <curl/curl.h>

fun_finish_cb g_finish_cb = NULL;
fun_process_cb g_process_cb = NULL;
CURL* g_curl = NULL;
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
	if (g_curl)
	{
		closeNetwork();
	}

	g_curl = curl_easy_init();
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

bool DownLoadUrl(char* _url)
{
	if (!g_curl)
	{
		return false;
	}

	//初始化curl，这个是必须的  
	curl_easy_setopt(g_curl, CURLOPT_URL, _url);
	//设置接收数据的回调  
	curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, g_finish_cb);
	//curl_easy_setopt(curl, CURLOPT_INFILESIZE, lFileSize);  
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);  
	//curl_easy_setopt(curl, CURLOPT_NOBODY, 1);  
	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
	// 设置重定向的最大次数  
	curl_easy_setopt(g_curl, CURLOPT_MAXREDIRS, 5);
	// 设置301、302跳转跟随location  
	curl_easy_setopt(g_curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(g_curl, CURLOPT_NOPROGRESS, 0);
	//设置进度回调函数  
	curl_easy_setopt(g_curl, CURLOPT_PROGRESSFUNCTION, g_process_cb);
	//curl_easy_getinfo(curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);  
	//curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, g_hDlgWnd);  
	//开始执行请求  
	CURLcode retcCode = curl_easy_perform(g_curl);
	//查看是否有出错信息  
	const char* pError = curl_easy_strerror(retcCode);
	
	return true;
}