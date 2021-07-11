#ifndef UTILITY_H
#define UTILITY_H

#ifdef UTILITY_DLL
#ifdef UTILITY_EXPORT	/* { */
#define UTILITY_API __declspec(dllexport)
#else						/* }{ */
#define UTILITY_API __declspec(dllimport)
#endif	
#else
#ifdef UTILITY_STATIC_LIB
#define UTILITY_API extern
#else
#define UTILITY_API
#endif
#endif
/*-------------------------------------------------------------------------*\
* Initializes the library.
\*-------------------------------------------------------------------------*/
#ifdef __cplusplus                     
extern "C" {
#endif
	UTILITY_API void util_TcharToChar(const TCHAR * tchar, char * _char);
	UTILITY_API void util_CharToTchar(const char * _char, TCHAR * _tchar);
	UTILITY_API void util_Utf8ToUnicode(const char * _char, TCHAR * _tchar);
	UTILITY_API void util_UnicodeToUtf8(const TCHAR * tchar, char * _char);
	UTILITY_API TCHAR* util_getAppDir();
	UTILITY_API int util_windowLayoutCenter();
	UTILITY_API int util_win_close();
	UTILITY_API int util_win_minimize();
	UTILITY_API int util_win_activate();
	UTILITY_API TCHAR* util_get_open_file();
	UTILITY_API char* getLobbyWinName();
	UTILITY_API HWND getLobbyWinHandle();
	UTILITY_API HWND getLobbyWinHandleByProcessId(DWORD processId);
#ifdef __cplusplus
}
#endif

#endif /* UTILITY_H */
