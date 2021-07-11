#ifndef LIVEUPDATE_H
#define LIVEUPDATE_H
#include <string>
#include <vector>
#include <hash_map>
#include <functional>
#include <map>
// type definition
typedef void(*fun_processbar_value_cb)(int value);
typedef void(*fun_display_md5_cb)(const char* md5);
typedef void(*fun_display_sha1_cb)(const char* sha1);
typedef void(*fun_display_finish_file_cb)(const char* file);
typedef void(*fun_display_validate_cb)(const char* file);
typedef void(*fun_display_speed_cb)(size_t speed);
typedef void(*fun_display_percentage_cb)(size_t percentage);
typedef void(*fun_display_update_liveupdate)();
typedef void(*fun_notify_new_version)();
typedef stdext::hash_map<std::wstring, int> FileMap;
typedef std::map<std::string, std::string> CheckSumMap;
typedef std::function<void(wchar_t*)> OneParamFunc;

bool set_server_addr(const char* _addr);
bool set_display_md5_cb(fun_display_md5_cb _cb);
bool set_display_sha1_cb(fun_display_sha1_cb _cb);
bool set_display_finish_file_cb(fun_display_finish_file_cb _cb);
bool set_display_processbar_cb(fun_processbar_value_cb _cb);
bool set_display_validate_cb(fun_display_validate_cb _cb);
bool set_display_speed_cb(fun_display_speed_cb _cb);
bool set_display_percentage_cb(fun_display_percentage_cb _cb);
bool set_display_update_liveupdate(fun_display_update_liveupdate _cb);
bool set_on_lobby_new_version_cb(fun_notify_new_version _cb);
bool set_on_game_new_version_cb(fun_notify_new_version _cb);
bool download_patch_list();
bool download_checksum(const char* sub_path);
bool download_single_file(const char* sub_path, const char* file_name, size_t expect_size=0);
bool check_game_new_version(const char* game_sub_path);
bool check_lobby_new_version();
bool generate_local_patch_list();
bool validate_local(const wchar_t* checksum, std::vector<std::wstring> ignoreDirs, std::vector<std::wstring> ignoreFiles, CheckSumMap& undownloadFiles);
bool validate_downloaded(const wchar_t* checksum, std::vector<std::wstring> ignoreDirs, std::vector<std::wstring> ignoreFiles);
size_t getTotalDownloadSize();
size_t getCurrentDownloadSize();
size_t getDownloadSpeed();
#endif