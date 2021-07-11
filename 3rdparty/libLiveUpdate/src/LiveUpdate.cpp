#include "stdafx.h"
#include "LiveUpdate.h"
#include <string.h>
#include "network.h"
#include <stdio.h>
#include <direct.h>



#include "Plist.hpp"

#include "utility\src\utility.h"
#include "hashlib2plus\include\hashlibpp.h"
#include <Windows.h>
#include <mmsystem.h>
#include <Shellapi.h>
#include <stack>
#include <queue>
#include <sstream>

// global variable
#define MAX_URL_LEN 4096
#define MAX_PATH_LEN 256                                                       
#define STR_PATCH_LIST "patch.plist"
#define WSTR_PATCH_LIST L"patch.plist"
#define STR_CHCEKSUM "CHECKSUM.md5"
#define WSTR_CHCEKSUM L"CHECKSUM.md5"
#define STR_TMP_FOLDER "download"
#define WSTR_TMP_FOLDER L"download"
#ifdef _DEBUG
#define DEFAULT_PATCH_URL "http://www.hema3d.com/LiveUpdate/patch/"
#else
#define DEFAULT_PATCH_URL "http://www.hema3d.com/LiveUpdate/patch/"
#endif
char g_szServerAddr[MAX_URL_LEN];
char g_szSubPath[MAX_PATH_LEN];
wchar_t g_szFilePath[MAX_PATH_LEN];
wchar_t g_szFileName[MAX_PATH_LEN];
wchar_t g_szTmpFilePath[MAX_PATH_LEN];
size_t g_nExpectFileSize = 0;
size_t g_nTotalDownloadSize = 0;
size_t g_nCurrentDownloadSize = 0;
size_t g_nDownloadSpeed = 0;
DWORD g_nTimeElapse = 0;

// global callbacks
fun_processbar_value_cb g_fProcessbarValueCb = NULL;
fun_display_md5_cb g_fDisplayMd5Cb = NULL;
fun_display_sha1_cb g_fDisplaySha1Cb = NULL;
fun_display_finish_file_cb g_fDisplayFinishFileCb = NULL;
fun_display_validate_cb g_fDisplayValidateCb = NULL;
fun_display_speed_cb g_fDisplaySpeedCb = NULL;
fun_display_percentage_cb g_fDisplayPercentageCb = NULL;
fun_display_update_liveupdate g_fDisplayUpdateLiveUpdateCb = NULL;
fun_notify_new_version g_fOnLobbyNewVersion = NULL;
fun_notify_new_version g_fOnGameNewVersion = NULL;

// pre declare function
bool		format_slash(const wchar_t* _src, wchar_t* _dst);
bool		walk_folder(wchar_t* _path, std::vector<std::wstring> _ignoreDirs, std::vector<std::wstring> ignoreFiles, OneParamFunc _taskFunc);
bool		is_lobby_new_version(wchar_t* local_file_name, wchar_t* online_file_name);
bool		is_liveupdate_new_version(wchar_t* local_file_name, wchar_t* online_file_name);
std::string getPlistValue(wchar_t* fileName, std::queue<char*> keys);
bool		setPatchURL(wchar_t* fileName);
fpos_t		getFileSize(wchar_t* file_name);

// type pre define
typedef std::map<std::string, boost::any> PListDict;

size_t DownloadCallback(void* pBuffer, size_t nSize, size_t nMemByte, void* pParam)
{
	memcpy(g_recvData + g_recvPos, pBuffer, nSize*nMemByte);
	g_recvPos += nSize*nMemByte;

	

	return nSize*nMemByte;
}

void check_tmp_folder_exist()
{
	wchar_t newDir[MAX_PATH_LEN];
	swprintf(newDir, L"%s/%s", g_szAppDir, WSTR_TMP_FOLDER);
	int ret = _wmkdir(newDir);
}

void create_sub_folder_if_not_exist(wchar_t* fullpath)
{
	wchar_t wsFormatedPath[MAX_PATH_LEN];
	memset(wsFormatedPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	format_slash(fullpath, wsFormatedPath);

	wchar_t* head = wsFormatedPath;
	wchar_t dir[MAX_PATH_LEN];
	wchar_t newDir[MAX_PATH_LEN];
	memset(dir, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	memset(newDir, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	while (wchar_t* substr = wcschr(head, L'/'))
	{
		int len = substr - wsFormatedPath;
		wcsncpy(dir, wsFormatedPath, len);

		int ret = _wmkdir(dir);
		memset(newDir, 0, sizeof(wchar_t)*MAX_PATH_LEN);
		head = ++substr;
	}
}

void remove_tmp_sub_dir(wchar_t* sub_path)
{
	wchar_t wsSubPath[MAX_PATH + 2];
	swprintf(wsSubPath, L"%s/%s/%s%c%c", g_szAppDir, WSTR_TMP_FOLDER, sub_path, L'\0', L'\0');


	SHFILEOPSTRUCT FileOp;
	ZeroMemory(&FileOp, sizeof(SHFILEOPSTRUCT));
	FileOp.fFlags |= FOF_SILENT;        /*不显示进度*/
	FileOp.fFlags |= FOF_NOERRORUI;        /*不报告错误信息*/
	FileOp.fFlags |= FOF_NOCONFIRMATION;/*直接删除，不进行确认*/
	FileOp.hNameMappings = NULL;
	FileOp.hwnd = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.wFunc = FO_DELETE;
	FileOp.pFrom = wsSubPath;            /*要删除的目录，必须以2个\0结尾*/
	FileOp.pTo = NULL;
	FileOp.fFlags &= ~FOF_ALLOWUNDO; /*直接删除，不放入回收站*/


	/*删除目录*/
	if (!SHFileOperation(&FileOp))
	{
		HRESULT hr = GetLastError();
	}
}

void saveDataToDisk()
{
	FILE* fp = NULL;

	_wfopen_s(&fp, g_szTmpFilePath, L"wb");
	
	if (!fp)
	{
		// make new folder if not exist.
		wchar_t wsSubPath[MAX_PATH_LEN];
		memset(wsSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
		util_CharToTchar(g_szSubPath, wsSubPath);
		//wchar_t wsPureSubPath[MAX_PATH_LEN];
		//memset(wsPureSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
		

		//wcscat(wsSubPath, g_szFileName);
		//format_slash(wsSubPath, wsPureSubPath);

		//create_sub_folder_if_not_exist(wsPureSubPath);
		create_sub_folder_if_not_exist(g_szTmpFilePath);

		_wfopen_s(&fp, g_szTmpFilePath, L"wb");
	}

	if (fp)
	{
		size_t nWrite = fwrite(g_recvData, 1, g_recvTotal, fp);
		fclose(fp);
	}
}

size_t ProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	if (dltotal > -0.1 && dltotal < 0.1)
		return 0;
	//int nPos = (int)((dlnow / dltotal) * 100);
	int nPos = (int)((g_recvPos / g_recvTotal) * 100);
	if (g_fProcessbarValueCb)
	{
		g_fProcessbarValueCb(nPos);
	}

	// calculate download percentage.
	if (g_fDisplayPercentageCb)
	{
		//g_fDisplayPercentageCb((g_nCurrentDownloadSize + dlnow) / float(g_nTotalDownloadSize) * 100);
		g_fDisplayPercentageCb((g_nCurrentDownloadSize + g_recvPos) / float(g_nTotalDownloadSize) * 100);
	}
	
	return 0;
}

bool set_server_addr(const char* _addr)
{
	sprintf(g_szServerAddr, "%s", _addr);
	return true;
}

bool set_display_md5_cb(fun_display_md5_cb _cb)
{
	g_fDisplayMd5Cb = _cb;
	return true;
}

bool set_display_sha1_cb(fun_display_sha1_cb _cb)
{
	g_fDisplaySha1Cb = _cb;
	return true;
}

bool set_display_finish_file_cb(fun_display_finish_file_cb _cb)
{
	g_fDisplayFinishFileCb = _cb;
	return true;
}

bool set_display_processbar_cb(fun_processbar_value_cb _cb)
{
	g_fProcessbarValueCb = _cb;
	return true;
}

bool set_display_validate_cb(fun_display_validate_cb _cb)
{
	g_fDisplayValidateCb = _cb;
	return true;
}

bool set_display_speed_cb(fun_display_speed_cb _cb)
{
	g_fDisplaySpeedCb = _cb;
	return true;
}

bool set_display_percentage_cb(fun_display_percentage_cb _cb)
{
	g_fDisplayPercentageCb = _cb;
	return true;
}

bool set_display_update_liveupdate(fun_display_update_liveupdate _cb)
{
	g_fDisplayUpdateLiveUpdateCb = _cb;
	return true;
}

bool set_on_lobby_new_version_cb(fun_notify_new_version _cb)
{
	g_fOnLobbyNewVersion = _cb;
	return true;
}

bool set_on_game_new_version_cb(fun_notify_new_version _cb)
{
	g_fOnGameNewVersion = _cb;
	return true;
}

void fix_blank(const char* src, char* dst)
{
	int len = strlen(src);
	for (int i = 0; i < len; i++)
	{
		
		if (src[i] == ' ')
		{
			*dst = '%';
			++dst;
			*dst = '2';
			++dst;
			*dst = '0';
			++dst;
			/**dst = '\\';
			++dst;
			*dst = src[i];
			++dst;*/
		}
		else
		{
			*dst = src[i];
			++dst;
		}
	}
}

bool download_single_file(const char* sub_path, const char* file_name, size_t expect_size)
{
	char fixed_file_name[MAX_PATH];
	memset(fixed_file_name, 0, sizeof(char)*MAX_PATH);
	fix_blank(file_name, fixed_file_name);

	g_nExpectFileSize = expect_size;
	char strAddr[MAX_URL_LEN];
	memset(strAddr, 0, sizeof(char)*MAX_URL_LEN);
	if (strlen(sub_path))
	{
		sprintf(g_szSubPath, "%s/", sub_path);
	}

	wchar_t wsSubPath[MAX_PATH_LEN];
	memset(wsSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	util_CharToTchar(sub_path, wsSubPath);

	wchar_t wsFileName[MAX_PATH_LEN];
	memset(wsFileName, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	util_CharToTchar(file_name, wsFileName);

	if (wcslen(wsSubPath))
	{
		swprintf(g_szFilePath, L"%s/%s/%s", g_szAppDir, wsSubPath, wsFileName);
		swprintf(g_szTmpFilePath, L"%s/%s/%s/%s", g_szAppDir, WSTR_TMP_FOLDER, wsSubPath, wsFileName);
	}
	else
	{
		swprintf(g_szFilePath, L"%s/%s", g_szAppDir, wsFileName);
		swprintf(g_szTmpFilePath, L"%s/%s/%s", g_szAppDir, WSTR_TMP_FOLDER, wsFileName);
	}
	
	swprintf(g_szFileName, L"%s", wsFileName);
	strcat(strAddr, g_szServerAddr);
	strcat(strAddr, g_szSubPath);
	strcat(strAddr, fixed_file_name);

	//MessageBoxA(0, 0, 0, 0);
	int size = GetDownloadFileSize(strAddr);
	if (size < 0)
	{
		return false;
	}

	//int ret = _wremove(g_szFilePath);

	setDownLoadFinishCallback(DownloadCallback);
	setDownLoadProcessCallback(ProgressCallback);

	double speed = 0.0;
	bool success = DownLoadUrl(strAddr, speed);
	if (!success)
	{
		return false;
	}

	g_nCurrentDownloadSize += g_recvTotal;
	g_nDownloadSpeed = speed;

	if (g_fDisplaySpeedCb)
	{
		g_fDisplaySpeedCb(speed);
	}

	saveDataToDisk();

	char strFilePath[MAX_PATH_LEN];
	memset(strFilePath, 0, sizeof(char)*MAX_PATH_LEN);
	util_TcharToChar(g_szFilePath, strFilePath);

	/*hashwrapper *md5Wrapper = new md5wrapper();
	std::string md5 = md5Wrapper->getHashFromFile(strFilePath);
	delete md5Wrapper;

	hashwrapper *sha1Wrapper = new sha1wrapper();
	std::string sha1 = sha1Wrapper->getHashFromFile(strFilePath);
	delete sha1Wrapper;

	if (g_fDisplayMd5Cb)
	{
	g_fDisplayMd5Cb(md5.c_str());
	}

	if (g_fDisplaySha1Cb)
	{
	g_fDisplaySha1Cb(sha1.c_str());
	}*/

	// finish callback is last invoke.because window refresh need to be done here.
	if (g_fDisplayFinishFileCb)
	{
		char strFileDownloaded[MAX_PATH_LEN];
		memset(strFileDownloaded, 0, sizeof(char)*MAX_PATH_LEN);
		strcat(strFileDownloaded, sub_path);
		strcat(strFileDownloaded, "/");
		strcat(strFileDownloaded, file_name);
		g_fDisplayFinishFileCb(strFileDownloaded);
	}

	if (g_recvData)
	{
		delete[] g_recvData;
	}

	return true;
}

bool download_patch_list()
{
	return download_single_file("", STR_PATCH_LIST);
}

bool download_checksum(const char* sub_path)
{
	return download_single_file(sub_path, STR_CHCEKSUM);
}

bool download_whole_from_chechsum(const char* game_sub_path, wchar_t* wsCheckSumFile, std::vector<std::string> vExclude)
{
	std::map<std::string, boost::any> dict;
	Plist::readPlist(wsCheckSumFile, dict);

	std::map<std::string, boost::any>::iterator it = dict.begin();
	std::map<std::string, boost::any>::iterator iten = dict.end();
	while (it != iten)
	{
		std::string file_name = it->first;

		bool isExcludeFile = false;
		for (unsigned int i = 0; i < vExclude.size(); i++)
		{
			if (vExclude[i].find(file_name) != std::string::npos)
			{
				isExcludeFile = true;
			}
		}

		if (isExcludeFile)
		{
			++it;
			continue;
		}

		std::string file_detail = boost::any_cast<std::string>(it->second);
		int pos = file_detail.find_first_of(':');
		if (pos!=std::string::npos)
		{
			int file_size = 0;
			std::string strFileSize = file_detail.substr(pos+1);
			file_size = atoi(strFileSize.c_str());
			bool success = download_single_file(game_sub_path, file_name.c_str(), file_size);
			if (!success)
			{
				return false;
			}
		}

		++it;
	}

	return true;
}

bool download_diff_from_chechsum(const char* game_sub_path, std::vector<std::string> vExclude, CheckSumMap& _diff)
{
	CheckSumMap::iterator itor = _diff.begin();
	CheckSumMap::iterator iten = _diff.end();
	while (itor != iten)
	{
		std::string fileName = itor->first;
		std::string fileDetail = itor->second;

		bool isExcludeFile = false;
		for (unsigned int i = 0; i < vExclude.size(); i++)
		{
			if (vExclude[i].find(fileName) != std::string::npos)
			{
				isExcludeFile = true;
			}
		}

		if (isExcludeFile)
		{
			++itor;
			continue;
		}

		int pos = fileDetail.find_first_of(':');
		if (pos != std::string::npos)
		{
			int file_size = 0;
			std::string strFileSize = fileDetail.substr(pos + 1);
			file_size = atoi(strFileSize.c_str());
			bool success = download_single_file(game_sub_path, fileName.c_str(), file_size);
			if (!success)
			{
				return false;
			}
		}
		++itor;
	}
	

	return true;
}

CheckSumMap check_single_checksum(wchar_t* full_path)
{
	std::map<std::string, std::string> vLocalChecksum;

	std::map<std::string, boost::any> dict;
	try
	{
		Plist::readPlist(full_path, dict);
	}
	catch (std::runtime_error RunTimeError)
	{
		return vLocalChecksum;
	}


	std::map<std::string, boost::any>::iterator it = dict.begin();
	std::map<std::string, boost::any>::iterator iten = dict.end();
	while (it != iten)
	{
		std::string fileName = boost::any_cast<std::string>(it->first);
		std::string fileDetail = boost::any_cast<std::string>(it->second);
		size_t pos = fileDetail.find(':');
		std::string fileMd5 = fileDetail.substr(0, pos);
		std::string fileLen = fileDetail.substr(pos);
		vLocalChecksum[fileName] = fileDetail;
		++it;
	}

	return vLocalChecksum;
}

CheckSumMap check_local_checksum(const char* sub_path)
{
	wchar_t wsCheckSumFile[MAX_PATH_LEN];
	memset(wsCheckSumFile, 0, sizeof(wchar_t)*MAX_PATH_LEN);

	wchar_t wsSubPath[MAX_PATH_LEN];
	memset(wsSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	util_CharToTchar(sub_path, wsSubPath);

	if (wcslen(wsSubPath))
	{
		swprintf(wsCheckSumFile, L"%s/%s/CHECKSUM.md5", g_szAppDir, wsSubPath);
	}
	else
	{
		swprintf(wsCheckSumFile, L"%s/CHECKSUM.md5", g_szAppDir);
	}

	return check_single_checksum(wsCheckSumFile);
}

CheckSumMap check_online_checksum(const char* sub_path)
{
	wchar_t wsCheckSumFile[MAX_PATH_LEN];
	memset(wsCheckSumFile, 0, sizeof(wchar_t)*MAX_PATH_LEN);

	wchar_t wsSubPath[MAX_PATH_LEN];
	memset(wsSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	util_CharToTchar(sub_path, wsSubPath);

	if (wcslen(wsSubPath))
	{
		swprintf(wsCheckSumFile, L"%s/%s/%s/CHECKSUM.md5", g_szAppDir, WSTR_TMP_FOLDER, wsSubPath);
	}
	else
	{
		swprintf(wsCheckSumFile, L"%s/%s/CHECKSUM.md5", g_szAppDir, WSTR_TMP_FOLDER);
	}

	return check_single_checksum(wsCheckSumFile);
}

CheckSumMap generateLiveUpdateDiffFile(wchar_t* local_file_name, wchar_t* online_file_name)
{
	CheckSumMap rtValue;

	//std::string local_fileDetail;
	std::string online_fileDetail;

	std::queue<char*> keys;
	keys.push("quickcheck");
	keys.push("LiveUpdate.exe");

	//local_fileDetail = getPlistValue(local_file_name, keys);
	online_fileDetail = getPlistValue(online_file_name, keys);

	/*if (local_fileDetail.compare(online_fileDetail))
	{
		rtValue["LiveUpdate.exe"] = online_fileDetail;
	}*/

	wchar_t wsLiveUpdate[MAX_PATH_LEN];
	swprintf(wsLiveUpdate, L"%s/LiveUpdate.exe", g_szAppDir);

	char cLiveUpdateFile[MAX_PATH];
	util_TcharToChar(wsLiveUpdate, cLiveUpdateFile);
	hashwrapper *md5Wrapper = new md5wrapper();
	std::string md5 = md5Wrapper->getHashFromFile(cLiveUpdateFile);
	std::string fileDetail = md5;
	std::string fileSize;
	
	std::stringstream ssm;
	ssm << (long)getFileSize(wsLiveUpdate);
	ssm >> fileSize;

	fileDetail += ":";
	fileDetail += fileSize;

	if (fileDetail.compare(online_fileDetail)!=0)
	{
		rtValue["LiveUpdate.exe"] = online_fileDetail;
	}

	return rtValue;
}
CheckSumMap generateDiffFiles(CheckSumMap& _local, CheckSumMap& _online)
{
	CheckSumMap rtValue;

	CheckSumMap::iterator itor = _online.begin();
	CheckSumMap::iterator iten = _online.end();
	CheckSumMap::iterator iten_l = _local.end();
	while (itor != iten)
	{
		std::string fileName = itor->first;
		std::string fileDetail = itor->second;

		CheckSumMap::iterator itLocal = _local.find(fileName);
		if (itLocal != iten_l)
		{
			// local checksum have this file, check md5
			if (fileDetail.compare(itLocal->second) != 0 )
			{
				rtValue[fileName] = fileDetail;
			}
		}
		else
		{
			// local checksum have no such file, add that file.
			rtValue[fileName] = fileDetail;
		}

		++itor;
	}

	return rtValue;
}

void download_pre_start()
{
	g_nTimeElapse = timeGetTime();
	g_nTotalDownloadSize = 0;
	g_nCurrentDownloadSize = 0;
}

size_t calculate_total_download_size(wchar_t* wsCheckSumFile, std::vector<std::string> vExclude)
{
	size_t rtValue = 0;

	std::map<std::string, boost::any> dict;
	Plist::readPlist(wsCheckSumFile, dict);

	std::map<std::string, boost::any>::iterator it = dict.begin();
	std::map<std::string, boost::any>::iterator iten = dict.end();
	while (it != iten)
	{
		std::string file_name = it->first;

		bool isExcludeFile = false;
		for (unsigned int i = 0; i < vExclude.size(); i++)
		{
			if (vExclude[i].find(file_name) != std::string::npos)
			{
				isExcludeFile = true;
			}
		}

		if (isExcludeFile)
		{
			++it;
			continue;
		}

		std::string file_detail = boost::any_cast<std::string>(it->second);
		int pos = file_detail.find_first_of(':');
		if (pos != std::string::npos)
		{
			int file_size = 0;
			std::string strFileSize = file_detail.substr(pos + 1);
			file_size = atoi(strFileSize.c_str());
			rtValue += file_size;
		}

		++it;
	}

	return rtValue;
}

size_t calculate_total_download_size(CheckSumMap& _diff, std::vector<std::string> vExclude)
{
	size_t rtValue = 0;

	CheckSumMap::iterator itor = _diff.begin();
	CheckSumMap::iterator iten = _diff.end();
	while (itor != iten)
	{
		std::string fileName = itor->first;
		std::string fileDetail = itor->second;

		bool isExcludeFile = false;
		for (unsigned int i = 0; i < vExclude.size(); i++)
		{
			if (vExclude[i].find(fileName) != std::string::npos)
			{
				isExcludeFile = true;
			}
		}

		if (isExcludeFile)
		{
			++itor;
			continue;
		}

		int pos = fileDetail.find_first_of(':');
		if (pos != std::string::npos)
		{
			int file_size = 0;
			std::string strFileSize = fileDetail.substr(pos + 1);
			file_size = atoi(strFileSize.c_str());
			rtValue += file_size;
		}
		++itor;
	}

	return rtValue;
}

bool move_tmp_files_to_dest_folder(const char* game_sub_path)
{
	wchar_t search_path[MAX_PATH];
	
	wchar_t dst_path[MAX_PATH];
	
	wchar_t wsSubPath[MAX_PATH_LEN];
	memset(wsSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	util_CharToTchar(game_sub_path, wsSubPath);

	swprintf(search_path, L"%s/%s/%s", g_szAppDir, WSTR_TMP_FOLDER, wsSubPath);
	
	swprintf(dst_path, L"%s/%s", g_szAppDir, wsSubPath);

	std::vector<std::wstring> vExcludeFiles;
	std::vector<std::wstring> vExcludeDirs;
	walk_folder(search_path, vExcludeDirs, vExcludeFiles, [=](wchar_t* filePath){
		// copy tmp folder's total files to real path
		wchar_t dst_file_name[MAX_PATH];
		wchar_t tmp_path[MAX_PATH];
		swprintf(tmp_path, L"%s/%s", g_szAppDir, WSTR_TMP_FOLDER);
		wchar_t* real_file_path = filePath + wcslen(tmp_path);
		swprintf(dst_file_name, L"%s/%s", g_szAppDir, real_file_path);
		create_sub_folder_if_not_exist(dst_file_name);
		CopyFile(filePath, dst_file_name, false);
	});

	return true;
}

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

fpos_t getFileSize(wchar_t* file_name)
{
	FILE* fp = NULL;
	_wfopen_s(&fp, file_name, L"r");
	if (!fp)
	{
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	fpos_t pos;
	fgetpos(fp, &pos);
	fclose(fp);
	return pos;
}

bool setPatchURL(wchar_t* fileName)
{
	std::queue<char*> keys;
	keys.push("channels");
	keys.push("dev_desktop");
	keys.push("patch_base_url");
	std::string url = getPlistValue(fileName, keys);
	set_server_addr(url.c_str());

	return true;
}

bool check_game_new_version(const char* game_sub_path)
{
	//MessageBoxA(0,0,0,0);
	wchar_t wsCheckSumFile[MAX_PATH_LEN];
	memset(wsCheckSumFile, 0, sizeof(wchar_t)*MAX_PATH_LEN);

	wchar_t wsPatchList[MAX_PATH_LEN];
	swprintf(wsPatchList, L"%s/%s", g_szAppDir, WSTR_PATCH_LIST);

	wchar_t wsSubPath[MAX_PATH_LEN];
	memset(wsSubPath, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	util_CharToTchar(game_sub_path, wsSubPath);

	wcscat(wsCheckSumFile,g_szAppDir);
	wcscat(wsCheckSumFile, L"/");
	wcscat(wsCheckSumFile, wsSubPath);
	wcscat(wsCheckSumFile, L"/");
	wcscat(wsCheckSumFile, WSTR_CHCEKSUM);

	check_tmp_folder_exist();

	// get patch server url.
	if (is_file_exist(L"patch.plist"))
	{
		setPatchURL(wsPatchList);
	}
	else
	{
		set_server_addr(DEFAULT_PATCH_URL);
	}

	// check CHECKSUM.md5 exist.
	if (!is_file_exist(wsCheckSumFile))
	{
		// if we don't have CHECKSUM.md5.local,download whole files
		if (!download_checksum(game_sub_path))
		{
			return false;
		}

		wchar_t wsTmpCheckSumFile[MAX_PATH_LEN];
		swprintf(wsTmpCheckSumFile, L"%s/%s/%s/%s", g_szAppDir, WSTR_TMP_FOLDER, wsSubPath, WSTR_CHCEKSUM);

		// if download CHECKSUM.md5 failed, return
		if (!is_file_exist(wsTmpCheckSumFile))
		{
			return false;
		}

		// notify game new version.
		if (g_fOnGameNewVersion)
		{
			g_fOnGameNewVersion();
		}

		std::vector<std::string> vExclude;
		std::vector<std::wstring> vExcludeFiles;
		std::vector<std::wstring> vExcludeDirs;

		download_pre_start();
		g_nTotalDownloadSize = calculate_total_download_size(wsTmpCheckSumFile, vExclude);
		
		bool ret = download_whole_from_chechsum(game_sub_path, wsTmpCheckSumFile, vExclude);
		if (!ret)
		{
			return false;
		}
		if (move_tmp_files_to_dest_folder(game_sub_path))
		{
			// when online-files move to correct place, remove the temp files.
			remove_tmp_sub_dir(wsSubPath);
			
			// remove none online-files from old version folders.
			return validate_downloaded(wsCheckSumFile, vExcludeDirs, vExcludeFiles);
		}
		
		return true;
	}
	

	// if we have CHECKSUM.md5.local, check md5 and sha1 with patch.plist to find whether a new version exist.
	// TODO :
	{
		//download_checksum(game_sub_path);
		//_wfopen_s(&fp, wsCheckSumFile, L"r");

		//MessageBoxA(0, 0, 0, 0);
		char strGamePath[MAX_PATH_LEN];
		memset(strGamePath, 0, sizeof(char)*MAX_PATH_LEN);
		strcat(strGamePath, game_sub_path);
		strcat(strGamePath, "/");

		std::map<std::string, boost::any> dict;
		Plist::readPlist(wsPatchList, dict);

		std::map<std::string, boost::any>::iterator it = dict.find("quickcheck");
		std::map<std::string, boost::any>::iterator iten = dict.end();
		if (it != iten)
		{
			std::map<std::string, boost::any> quickchecks = boost::any_cast<std::map<std::string, boost::any>>(it->second);
			std::map<std::string, boost::any>::iterator it_quickcheck = quickchecks.find(strGamePath);
			std::map<std::string, boost::any>::iterator iten_quickcheck = quickchecks.end();
			if (it_quickcheck != iten_quickcheck)
			{
				// server md5 and sha1 value
				std::string md5sha1 = boost::any_cast<std::string>(it_quickcheck->second);

				char cCheckSumFile[MAX_PATH];
				util_TcharToChar(wsCheckSumFile, cCheckSumFile);

				// calculate local CHECKSUM.md5's md5 and sha1
				hashwrapper *md5Wrapper = new md5wrapper();
				std::string md5 = md5Wrapper->getHashFromFile(cCheckSumFile);
				delete md5Wrapper;

				hashwrapper *sha1Wrapper = new sha1wrapper();
				std::string sha1 = sha1Wrapper->getHashFromFile(cCheckSumFile);
				delete sha1Wrapper;

				// a new version exist?
				if (md5sha1.compare(md5 + sha1)==0)
				{
					// do we have some missing files?
					std::vector<std::wstring> vExcludeFiles;
					std::vector<std::wstring> vExcludeDirs;
					std::vector<std::string> vExclude;
					CheckSumMap undownloadFiles;
					if (validate_local(wsCheckSumFile, vExcludeDirs, vExcludeFiles, undownloadFiles))
					{
						// notify game new version.
						if (g_fOnGameNewVersion)
						{
							g_fOnGameNewVersion();
						}

						download_pre_start();
						g_nTotalDownloadSize = calculate_total_download_size(undownloadFiles, vExclude);
						bool ret = download_diff_from_chechsum(game_sub_path, vExclude, undownloadFiles);
						if (!ret)
						{
							return false;
						}
						if (move_tmp_files_to_dest_folder(game_sub_path))
						{
							// when online-files move to correct place, remove the temp files.
							remove_tmp_sub_dir(wsSubPath);

							// remove none online-files from old version folders.
							return validate_downloaded(wsCheckSumFile, vExcludeDirs, vExcludeFiles);
						}
					}

					// we can finish live update.
					return true;
				}
				
				// yes! do a diff liveupdate.

				// check local CHECKSUM.md5 to find what files we already have.
				CheckSumMap localChecksum;
				localChecksum = check_local_checksum(game_sub_path);

				// download online checksum.md5
				if (!download_checksum(game_sub_path))
				{
					return false;
				}

				// check online CHECKSUM.md5 to find diff files.
				CheckSumMap onlineChecksum;
				onlineChecksum = check_online_checksum(game_sub_path);

				// get diff files
				CheckSumMap diffMap = generateDiffFiles(localChecksum, onlineChecksum);

				// notify game new version.
				if (diffMap.size())
				{
					if (g_fOnGameNewVersion)
					{
						g_fOnGameNewVersion();
					}
				}
				

				std::vector<std::string> vExclude;
				std::vector<std::wstring> vExcludeFiles;
				std::vector<std::wstring> vExcludeDirs;
				download_pre_start();
				g_nTotalDownloadSize = calculate_total_download_size(diffMap, vExclude);
				
				bool ret = download_diff_from_chechsum(game_sub_path, vExclude, diffMap);
				if (!ret)
				{
					return false;
				}
				if (move_tmp_files_to_dest_folder(game_sub_path))
				{
					// when online-files move to correct place, remove the temp files.
					remove_tmp_sub_dir(wsSubPath);

					// remove none online-files from old version folders.
					return validate_downloaded(wsCheckSumFile, vExcludeDirs, vExcludeFiles);
				}
			}
		}
	}

	return true;
}

bool check_lobby_new_version()
{
	//MessageBoxA(0, 0, 0, 0);
	//MessageBeep(0);
	wchar_t wsCheckSumFile[MAX_PATH_LEN];
	memset(wsCheckSumFile, 0, sizeof(wchar_t)*MAX_PATH_LEN);
	wchar_t wsPatchList[MAX_PATH_LEN];
	swprintf(wsPatchList, L"%s/%s", g_szAppDir, WSTR_PATCH_LIST);
	wchar_t wsTmpPatchList[MAX_PATH_LEN];
	swprintf(wsTmpPatchList, L"%s/%s/%s", g_szAppDir, WSTR_TMP_FOLDER, WSTR_PATCH_LIST);

	wcscat(wsCheckSumFile, g_szAppDir);
	wcscat(wsCheckSumFile, L"/");
	wcscat(wsCheckSumFile, WSTR_CHCEKSUM);

	check_tmp_folder_exist();

	// get patch server url.
	if (is_file_exist(L"patch.plist"))
	{
		setPatchURL(wsPatchList);
	}
	else
	{
		set_server_addr(DEFAULT_PATCH_URL);
	}

	// fetch online version info.
	if (!download_patch_list())
	{
		return false;
	}

	// check local patch.plist
	if (!is_file_exist(L"patch.plist"))
	{
		// copy to root 
		CopyFile(wsTmpPatchList, wsPatchList, false);
	}

	// read new patch server url.
	setPatchURL(wsPatchList);

	// check myself need update.
	bool bNewLiveUpdate = is_liveupdate_new_version(wsPatchList, wsTmpPatchList);
	if (bNewLiveUpdate)
	{
		CheckSumMap diffMap = generateLiveUpdateDiffFile(wsPatchList, wsTmpPatchList);
		if (diffMap.size())
		{
			if (g_fDisplayUpdateLiveUpdateCb)
			{
				g_fDisplayUpdateLiveUpdateCb();
			}

			// notify lobby new version.
			if (g_fOnLobbyNewVersion)
			{
				g_fOnLobbyNewVersion();
			}

			download_pre_start();
			std::vector<std::string> vExclude;
			g_nTotalDownloadSize = calculate_total_download_size(diffMap, vExclude);

			return download_diff_from_chechsum("", vExclude, diffMap);
		}
		

		// when program execute here, cause the lobby auto run the second time.we already have online version of liveupdate.exe
		// just do the following work.
	}

	// check new version exist.
	bool bNewLobby = is_lobby_new_version(wsPatchList, wsTmpPatchList);

	if (!bNewLiveUpdate && !bNewLobby)
	{
		// copy online patch list to root folder anyway 
		CopyFile(wsTmpPatchList, wsPatchList, false);
		return true;
	}

	// notify lobby new version.
	if (g_fOnLobbyNewVersion)
	{
		g_fOnLobbyNewVersion();
	}

	// check local CHECKSUM.md5 to find what files we already have.
	CheckSumMap localChecksum = check_local_checksum("");

	// download online checksum.md5
	if (!download_checksum(""))
	{
		return false;
	}

	// check online CHECKSUM.md5 to find diff files.
	CheckSumMap onlineChecksum;
	onlineChecksum = check_online_checksum("");

	// get diff files
	CheckSumMap diffMap = generateDiffFiles(localChecksum, onlineChecksum);

	//// check CHECKSUM.md5 exist.
	//if (!is_file_exist(wsCheckSumFile))
	//{
	//	MessageBoxA(0,"Update Lobby Failed! patch.plist missing!",0,0);
	//	return false;
	//}


	// if we have CHECKSUM.md5.local, check md5 and sha1 with patch.plist to find whether a new version exist.
	{
		// download from root folder
		download_pre_start();
		std::vector<std::string> vExclude;
		g_nTotalDownloadSize = calculate_total_download_size(diffMap, vExclude);
		
		
		bool ret = download_diff_from_chechsum("", vExclude, diffMap);
		if (!ret)
		{
			return false;
		}
		if (move_tmp_files_to_dest_folder(""))
		{
			std::vector<std::wstring> vIgnoreDirs;
			std::vector<std::wstring> vIgnoreFiles;

			wchar_t ignoreDir[MAX_PATH];
			swprintf(ignoreDir, L"%s/%s", g_szAppDir, L"hippo");
			format_slash(ignoreDir, ignoreDir);
			vIgnoreDirs.push_back(ignoreDir);
		
			
			wchar_t ignoreFile[MAX_PATH];
			memset(ignoreFile, 0, sizeof(wchar_t)*MAX_PATH);
			format_slash(g_szAppDir, ignoreFile);
			wcscat(ignoreFile, L"/LiveUpdate.exe");
			vIgnoreFiles.push_back(ignoreFile);
			memset(ignoreFile, 0, sizeof(wchar_t)*MAX_PATH);
			format_slash(g_szAppDir, ignoreFile);
			wcscat(ignoreFile, L"/patch.plist");
			vIgnoreFiles.push_back(ignoreFile);
			swprintf(ignoreFile, L"%s/%s/%s", g_szAppDir, WSTR_TMP_FOLDER, L"LiveUpdate.exe");
			format_slash(ignoreFile, ignoreFile);
			vIgnoreFiles.push_back(ignoreFile);

			// when online-files move to correct place, remove the temp files.
			remove_tmp_sub_dir(L"src");
			remove_tmp_sub_dir(L"res");

			/*if (diffMap.find("LiveUpdate.exe") == diffMap.end())
			{
			remove_tmp_sub_dir(L"");
			}*/
			remove_tmp_sub_dir(L"");

			// remove none online-files from old version folders.
			return validate_downloaded(wsCheckSumFile, vIgnoreDirs, vIgnoreFiles);
		}
	}

	return true;
}

std::string _getPlistValue(PListDict& dict, std::queue<char*> keys)
{
	std::string rtValue;
	if (!keys.size())
	{
		return rtValue;
	}

	PListDict::iterator it = dict.begin();
	PListDict::iterator iten = dict.end();
	PListDict::iterator itor = dict.find(keys.front());
	keys.pop();
	if (itor != iten)
	{
		try
		{
			rtValue = boost::any_cast<std::string>(itor->second);
		}
		catch (boost::bad_any_cast & e)
		{
			try
			{
				PListDict subDict = boost::any_cast<PListDict>(itor->second);
				if (!subDict.empty())
				{
					rtValue = _getPlistValue(subDict, keys);
				}
			}
			catch (boost::bad_any_cast & e2)
			{
				return rtValue;
			}
		}
	}
	else
	{
		return rtValue;
		
		
	}

	return rtValue;
}

std::string getPlistValue(wchar_t* fileName, std::queue<char*> keys)
{
	PListDict dict;
	Plist::readPlist(fileName, dict);	

	return _getPlistValue(dict, keys);
}

bool is_lobby_new_version(wchar_t* local_file_name, wchar_t* online_file_name)
{
	std::string local_md5sha1;
	std::string online_md5sha1;

	std::queue<char*> keys;
	keys.push("quickcheck");
	keys.push("launcher");

	local_md5sha1 = getPlistValue(local_file_name, keys);
	online_md5sha1 = getPlistValue(online_file_name, keys);

	return local_md5sha1.compare(online_md5sha1);
}

bool is_liveupdate_new_version(wchar_t* local_file_name, wchar_t* online_file_name)
{
	std::string local_md5sha1;
	std::string online_md5sha1;

	std::queue<char*> keys;
	keys.push("quickcheck");
	keys.push("LiveUpdate.exe");

	local_md5sha1 = getPlistValue(local_file_name, keys);
	online_md5sha1 = getPlistValue(online_file_name, keys);

	return local_md5sha1.compare(online_md5sha1);
}

// change '\\' to '/'
bool format_slash(const wchar_t* _src, wchar_t* _dst)
{
	int size = wcslen(_src);
	for (int i = 0; i < size; i++)
	{
		if (_src[i] ==L'\\')
		{
			_dst[i] = L'/';
			continue;
		}

		_dst[i] = _src[i];
	}

	return true;
}

//bool walk_folder(wchar_t* _path, wchar_t* _ignoreDir, std::vector<std::wstring> ignoreFiles, FileMap& _downloaded)
bool walk_folder(wchar_t* _path, std::vector<std::wstring> _ignoreDirs, std::vector<std::wstring> ignoreFiles, OneParamFunc _taskFunc)
{
	WIN32_FIND_DATA wfd;
	wchar_t search_path[MAX_PATH];
	memset(search_path, 0, sizeof(wchar_t)*MAX_PATH);
	wcscat(search_path, _path);
	wcscat(search_path, L"/*.*");
	HANDLE hFind = FindFirstFile(search_path, &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// walk folder tree
	while (FindNextFile(hFind, &wfd))
	{
		if (wcscmp(wfd.cFileName, L"..")==0)
		{
			continue;
		}

		if (wfd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			wchar_t search_sub_path[MAX_PATH];
			memset(search_sub_path, 0, sizeof(wchar_t)*MAX_PATH);
			wcscat(search_sub_path, _path);
			wcscat(search_sub_path, L"/");
			wcscat(search_sub_path, wfd.cFileName);
			/*if (wcscmp(search_sub_path, _ignoreDir) == 0)
			{
			continue;
			}*/
			std::vector<std::wstring>::iterator itIgnore = std::find(_ignoreDirs.begin(), _ignoreDirs.end(), search_sub_path);
			if (itIgnore != _ignoreDirs.end())
			{
				continue;
			}
			
			walk_folder(search_sub_path, _ignoreDirs, ignoreFiles, _taskFunc);
		}
		else
		{
			wchar_t filePath[MAX_PATH];
			memset(filePath, 0, sizeof(wchar_t)*MAX_PATH);
			wcscat(filePath, _path);
			wcscat(filePath, L"/");
			wcscat(filePath, wfd.cFileName);

			std::vector<std::wstring>::iterator itIgnore = std::find(ignoreFiles.begin(), ignoreFiles.end(), filePath);
			if (itIgnore != ignoreFiles.end())
			{
				continue;
			}

			
			
			_taskFunc(filePath);
			

		}
	}

	FindClose(hFind);
	return false;
}

bool validate_local(const wchar_t* checksum, std::vector<std::wstring> ignoreDirs, std::vector<std::wstring> ignoreFiles, CheckSumMap& undownloadFiles)
{
	bool rtValue = false;

	wchar_t* checksum_fixed = new wchar_t[MAX_PATH];
	memset(checksum_fixed, 0, sizeof(wchar_t)*MAX_PATH);
	format_slash(checksum, checksum_fixed);

	wchar_t* validete_root = new wchar_t[MAX_PATH];
	memset(validete_root, 0, sizeof(wchar_t)*MAX_PATH);

	// get download root path.
	const wchar_t* last_folder = wcsrchr(checksum_fixed, L'/');
	int len = last_folder - checksum_fixed;
	wcsncpy(validete_root, checksum_fixed, len);

	// parse checksum.md5
	std::map<std::string, boost::any> dict;
	Plist::readPlist(checksum_fixed, dict);
	//stdext::hash_map<std::wstring, int> undownloadedFileMap;

	std::map<std::string, boost::any>::iterator it = dict.begin();
	std::map<std::string, boost::any>::iterator iten = dict.end();
	while (it != iten)
	{
		std::string file_name = it->first;
		std::string file_detail = boost::any_cast<std::string>(it->second);
		int pos = file_detail.find_first_of(':');
		if (pos != std::string::npos)
		{
			int file_size = 0;
			std::string strFileSize = file_detail.substr(pos);
			file_size = atoi(strFileSize.c_str());

			std::wstring file_path;
			std::wstring wFileName(file_name.begin(), file_name.end());

			wchar_t wsFileName[MAX_PATH_LEN];
			memset(wsFileName, 0, sizeof(wchar_t)*MAX_PATH_LEN);
			util_CharToTchar(file_name.c_str(), wsFileName);

			file_path += validete_root;
			file_path += L"/";
			file_path += wsFileName;

			wchar_t filePath[MAX_PATH];
			memset(filePath, 0, sizeof(wchar_t)*MAX_PATH);
			format_slash(file_path.c_str(), filePath);

			FILE* fp = NULL;

			_wfopen_s(&fp, filePath, L"r");

			if (!fp)
			{
				undownloadFiles[file_name.c_str()] = file_detail;
				rtValue = true;
			}
			else
			{
				char cFileName[MAX_PATH];
				memset(cFileName, 0, sizeof(char)*MAX_PATH);
				util_TcharToChar(filePath, cFileName);

				// calculate local file's md5 and sha1
				hashwrapper *md5Wrapper = new md5wrapper();
				std::string md5 = md5Wrapper->getHashFromFile(cFileName);
				delete md5Wrapper;

				std::string online_md5 = file_detail.substr(0, pos);

				/*hashwrapper *sha1Wrapper = new sha1wrapper();
				std::string sha1 = sha1Wrapper->getHashFromFile(cFileName);
				delete sha1Wrapper;*/

				// same as server?
				if (online_md5.compare(md5) != 0)
				{
					undownloadFiles[file_name.c_str()] = file_detail;
					rtValue = true;
				}

				fclose(fp);
			}

			++it;
		}

	}
	

	return rtValue;
}

bool validate_downloaded(const wchar_t* checksum, std::vector<std::wstring> ignoreDirs, std::vector<std::wstring> ignoreFiles)
{
	wchar_t* checksum_fixed = new wchar_t[MAX_PATH];
	memset(checksum_fixed, 0, sizeof(wchar_t)*MAX_PATH);
	format_slash(checksum, checksum_fixed);

	wchar_t* validete_root = new wchar_t[MAX_PATH];
	memset(validete_root, 0, sizeof(wchar_t)*MAX_PATH);

	// get download root path.
	const wchar_t* last_folder = wcsrchr(checksum_fixed, L'/');
	int len = last_folder - checksum_fixed;
	wcsncpy(validete_root, checksum_fixed, len);

	// parse checksum.md5
	std::map<std::string, boost::any> dict;
	Plist::readPlist(checksum_fixed, dict);
	stdext::hash_map<std::wstring, int> downloadedFileMap;

	std::map<std::string, boost::any>::iterator it = dict.begin();
	std::map<std::string, boost::any>::iterator iten = dict.end();
	while (it != iten)
	{
		std::string file_name = it->first;
		std::string file_detail = boost::any_cast<std::string>(it->second);
		int pos = file_detail.find_first_of(':');
		if (pos != std::string::npos)
		{
			int file_size = 0;
			std::string strFileSize = file_detail.substr(pos);
			file_size = atoi(strFileSize.c_str());

			std::wstring file_path;
			std::wstring wFileName(file_name.begin(), file_name.end());
			file_path += validete_root;
			file_path += L"/";
			file_path += wFileName;

			wchar_t filePath[MAX_PATH];
			memset(filePath, 0, sizeof(wchar_t)*MAX_PATH);
			format_slash(file_path.c_str(), filePath);
			downloadedFileMap[filePath] = file_size;

			++it;
		}

	}
	// checksum.md5 should not be deleted too!
	downloadedFileMap[checksum_fixed] = 1;

	wchar_t search_path[MAX_PATH];
	memset(search_path, 0, sizeof(wchar_t)*MAX_PATH);
	wcscat(search_path, validete_root);

	walk_folder(search_path, ignoreDirs, ignoreFiles, [=](wchar_t* filePath){
		// if our disk contain files not exist in _downloaded(from checksum.md5), delete it.
		wchar_t* assets = wcsstr(filePath, L"XHome/Assets");//排除Assets目录
		stdext::hash_map<std::wstring, int>::const_iterator itor = downloadedFileMap.find(filePath);
		if (itor == downloadedFileMap.end() && assets==NULL)
		{
			if (g_fDisplayValidateCb)
			{
				char cFileName[MAX_PATH];
				memset(cFileName, 0, sizeof(char)*MAX_PATH);
				util_TcharToChar(filePath, cFileName);
				g_fDisplayValidateCb(cFileName);
			}
			DeleteFile(filePath);
		}
	});

	delete[] validete_root;

	return true;
}

size_t getTotalDownloadSize()
{
	return g_nTotalDownloadSize;
}

size_t getCurrentDownloadSize()
{
	return g_nCurrentDownloadSize;
}

size_t getDownloadSpeed()
{
	return g_nDownloadSpeed;
}