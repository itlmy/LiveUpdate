#pragma once

typedef size_t(*fun_finish_cb)(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam);
typedef size_t(*fun_process_cb)(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

bool initNetwork();
bool DownLoadUrl(char* _url);
bool closeNetwork();
bool setDownLoadFinishCallback(fun_finish_cb _cb);
bool setDownLoadProcessCallback(fun_process_cb _cb);