#pragma once

typedef size_t(*fun_finish_cb)(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam);
typedef size_t(*fun_process_cb)(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

bool initNetwork();
int GetDownloadFileSize(const char* _url);
bool DownLoadUrl(char* _url, double& speed);
bool closeNetwork();
bool setDownLoadFinishCallback(fun_finish_cb _cb);
bool setDownLoadProcessCallback(fun_process_cb _cb);
extern wchar_t g_szAppDir[256];
extern char* g_recvData;
extern size_t g_recvPos ;
extern size_t g_recvTotal;