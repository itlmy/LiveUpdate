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

	//��ʼ��curl������Ǳ����  
	curl_easy_setopt(g_curl, CURLOPT_URL, _url);
	//���ý������ݵĻص�  
	curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, g_finish_cb);
	//curl_easy_setopt(curl, CURLOPT_INFILESIZE, lFileSize);  
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);  
	//curl_easy_setopt(curl, CURLOPT_NOBODY, 1);  
	//curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
	// �����ض����������  
	curl_easy_setopt(g_curl, CURLOPT_MAXREDIRS, 5);
	// ����301��302��ת����location  
	curl_easy_setopt(g_curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(g_curl, CURLOPT_NOPROGRESS, 0);
	//���ý��Ȼص�����  
	curl_easy_setopt(g_curl, CURLOPT_PROGRESSFUNCTION, g_process_cb);
	//curl_easy_getinfo(curl,  CURLINFO_CONTENT_LENGTH_DOWNLOAD, &lFileSize);  
	//curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, g_hDlgWnd);  
	//��ʼִ������  
	CURLcode retcCode = curl_easy_perform(g_curl);
	//�鿴�Ƿ��г�����Ϣ  
	const char* pError = curl_easy_strerror(retcCode);
	
	return true;
}