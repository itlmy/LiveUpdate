// LiveUpdate.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "main.h"
#include <Mmsystem.h>
#include <commctrl.h>
#include <windowsx.h>
#include <process.h>
#include "network.h"
#include <stdio.h>
#include <string>
#include <fstream>
#include "hashlib2plus\include\hashlibpp.h"
#include "LiveUpdate.h"
#include <shellapi.h>
#include "utility\src\utility.h"
#include "png.h"
#include "AntiMultiOpen.h"
#include <tlhelp32.h>

#if _MSC_VER>=1900
#include "stdio.h" 
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus 
extern "C"
#endif 
FILE* __cdecl __iob_func(unsigned i) {
	return __acrt_iob_func(i);
}
#endif /* _MSC_VER>=1900 */

#define LAUNCHER "Launcher.exe"
#define LAUNCHER_UNICODE L"Launcher.exe"
#define LAUNCHER_NAME "launcher"
#define LAUNCHER_NAME_UNICODE L"launcher"
#define MAX_LOADSTRING 100
#define PNGSIGSIZE  8
#define CC_BREAK_IF(cond)           if(cond) break
// premultiply alpha, or the effect will wrong when want to use other pixel format in Texture2D,
// such as RGB888, RGB5A1
#define CC_RGB_PREMULTIPLY_ALPHA(vr, vg, vb, va) \
    (unsigned)(((unsigned)((unsigned char)(vr) * ((unsigned char)(va) + 1)) >> 8) | \
    ((unsigned)((unsigned char)(vg) * ((unsigned char)(va) + 1) >> 8) << 8) | \
    ((unsigned)((unsigned char)(vb) * ((unsigned char)(va) + 1) >> 8) << 16) | \
    ((unsigned)(unsigned char)(va) << 24))

// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名
CHAR g_szDisplayCaption[MAX_LOADSTRING];
CHAR g_szDisplayMD5[MAX_LOADSTRING];
CHAR g_szDisplaySHA1[MAX_LOADSTRING];
CHAR g_szDisplayDownloadSpeed[MAX_LOADSTRING];
CHAR g_szDisplayPercentage[MAX_LOADSTRING];
HWND g_hWnd;
HWND g_hProcess;
HWND g_hShadowWnd;
HDC g_hDC;
HDC g_hBackbuffer;
HBITMAP g_bmpBg;
HBITMAP g_bmpBgFrame;
HBITMAP g_bmpProgressLeft;
HBITMAP g_bmpProgressCenter;
HBITMAP g_bmpProgressRight;
HBITMAP g_bmpSpark;
HBITMAP g_bmpMinimizeNormal;
HBITMAP g_bmpMinimizeHover;
HBITMAP g_bmpMinimizeClick;
HBITMAP g_bmpCloseNormal;
HBITMAP g_bmpCloseHover;
HBITMAP g_bmpCloseClick;
RECT g_rcBtnMinimize;
RECT g_rcBtnClose;
RECT g_rcTxtCaption;
RECT g_rcTxtSpeed;
RECT g_rcTxtPercentage;
RECT g_rcProcessbar;
HFONT g_hFontCaption = NULL;
HFONT g_hFontDetail = NULL;
int g_nWinWidth;
int g_nWinHeight;
bool g_bUpdateLobby = false;
bool g_bUpdateGame = false;
bool g_bRestartLobby = false;
int g_nProgressValue = 0;
bool g_bIsInsideMinimize = false;
bool g_bIsInsideClose = false;
bool g_bIsMinimizeDown = false;
bool g_bIsCloseDown = false;
bool g_bIsMinimizeUp = false;
bool g_bIsCloseUp = false;
int g_nBytesDownloadNow = 0;
bool g_bUpdateSuccessed = false;
DWORD g_nHostId = 0;
CAntiMultiOpen g_muti;

typedef struct
{
	const unsigned char * data;
	size_t size;
	int offset;
}tImageSource;


// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void				drawWinFrame(HDC _dc, HBITMAP _bmp);
bool				drawBitmap(HDC _dc, HBITMAP _bmp, int xPos, int yPos);
void				drawProgress(HDC _dc, int value, int xPos, int yPos);
//INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
HDC					_createGdiBackBuffer(HWND hWnd);
void				drawCanvas(HDC hdc);
void				terminate_lobby();

void displayProcessBarValue(int value)
{
	//PostMessage(g_hProcess, PBM_SETPOS, value, 0);
	//PostMessage(g_hWnd, PBM_SETPOS, value, 0);
	//g_nProgressValue = value;
	//InvalidateRect(g_hWnd, NULL, false);
}

void displayMd5Callbcak(const char* md5)
{
	sprintf(g_szDisplayMD5, "MD5:%s", md5);
}

void displaySha1Callbcak(const char* sha1)
{
	sprintf(g_szDisplaySHA1, "SHA1:%s", sha1);
}

void displayFinishFileNameCallbcak(const char* file)
{
	//sprintf(g_szDisplayCaption, "Finish:%s", file);
	//InvalidateRect(g_hWnd, NULL, true);
}

void displayValidateCallbcak(const char* file)
{
	sprintf(g_szDisplayCaption, "验证资源");
	InvalidateRect(g_hWnd, &g_rcTxtCaption, true);
}

void displayUpdateLiveUpdateCallbcak()
{
	sprintf(g_szDisplayCaption, "更新自更新程序");
	InvalidateRect(g_hWnd, &g_rcTxtCaption, true);
}

void displayPercentageCallbcak(size_t percentage)
{
	sprintf(g_szDisplayPercentage, "下载进度：%d%%", percentage);
	g_nProgressValue = percentage;

	/*size_t bytesRecv = getCurrentDownloadSize() - g_nBytesDownloadNow;
	if (bytesRecv > 512000)
	{
		sprintf(g_szDisplayDownloadSpeed, "下载速度：%.1fMB/S", bytesRecv / 1024000 / 0.5f);
	}
	else
	{
		sprintf(g_szDisplayDownloadSpeed, "下载速度：%.1fKB/S", bytesRecv / 1024 / 0.5f);
	}*/

	InvalidateRect(g_hWnd, &g_rcTxtPercentage, true);
	InvalidateRect(g_hWnd, &g_rcProcessbar, true);
	//drawCanvas(g_hBackbuffer);
}

void displaySpeedCallbcak(size_t speed)
{
	if (speed > 1024000)
	{
		sprintf(g_szDisplayDownloadSpeed, "下载速度：%.1fMB/S", speed / 1024000.0f);
	}
	else
	{
		sprintf(g_szDisplayDownloadSpeed, "下载速度：%.1fKB/S", speed / 1024.0f);
	}

	InvalidateRect(g_hWnd, &g_rcTxtSpeed, true);
	//drawCanvas(g_hBackbuffer);
}

void showMainWindow()
{
	ShowWindow(g_hWnd, SW_SHOW);
	UpdateWindow(g_hWnd);
	ShowWindow(g_hShadowWnd, SW_SHOW);
	BringWindowToTop(g_hShadowWnd);
	BringWindowToTop(g_hWnd);
}

void onLobbyNewVersion()
{
	terminate_lobby();
	showMainWindow();
	g_bRestartLobby = true;
	g_bUpdateLobby = true;
}

void onGameNewVersion()
{
	showMainWindow();
	g_bUpdateGame = true;
}

void terminate_lobby()
{
	HANDLE hLobby = OpenProcess(PROCESS_TERMINATE, false, g_nHostId);
	if (hLobby)
	{
		TerminateProcess(hLobby, true);
	}
}

void restart_lobby()
{
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = TRUE;

	wchar_t wsLobbyExe[MAX_PATH];
	memset(wsLobbyExe, 0, sizeof(wchar_t)*MAX_PATH);

	TCHAR* strCurDirW = util_getAppDir();
	wcscat(wsLobbyExe, strCurDirW);
	wcscat(wsLobbyExe, L"/");
	wcscat(wsLobbyExe, LAUNCHER_UNICODE);
	wcscat(wsLobbyExe, L" -restart");

	CreateProcessW(NULL, wsLobbyExe, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
}

void notify_lobby_finished()
{
	COPYDATASTRUCT CopyData;
	if (g_bUpdateSuccessed)
	{
		CopyData.dwData = 666;
	}
	else
	{
		CopyData.dwData = 555;
	}
	
	CopyData.lpData = &g_nHostId;
	CopyData.cbData = sizeof(g_nHostId);
	HWND hParentWnd = getLobbyWinHandleByProcessId(g_nHostId);
	SendMessage(hParentWnd, WM_COPYDATA, (WPARAM)g_hWnd, (LPARAM)&CopyData);
}

void notify_lobby_cancel()
{
	COPYDATASTRUCT CopyData;
	CopyData.dwData = 111;
	CopyData.lpData = &g_nHostId;
	CopyData.cbData = sizeof(g_nHostId);
	HWND hParentWnd = getLobbyWinHandleByProcessId(g_nHostId);
	SendMessage(hParentWnd, WM_COPYDATA, (WPARAM)g_hWnd, (LPARAM)&CopyData);
}

void WINAPI onAutoClose(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dwl, DWORD dw2)
{
	// notify lobby live update finished.
	notify_lobby_finished();

	if (g_bRestartLobby)
	{
		restart_lobby();
	}

	// exit process.
	exit(0);
} 

void WINAPI onRefreshDownloadSpeed(UINT wTimerID, UINT msg, DWORD dwUser, DWORD dwl, DWORD dw2)
{
	size_t bytesRecv = getCurrentDownloadSize() - g_nBytesDownloadNow;
	if (bytesRecv > 512000)
	{
		sprintf(g_szDisplayDownloadSpeed, "下载速度：%.1fMB/S", bytesRecv / 1024000 / 0.5f);
	}
	else
	{
		sprintf(g_szDisplayDownloadSpeed, "下载速度：%.1fKB/S", bytesRecv / 1024 / 0.5f);
	}

	g_nBytesDownloadNow = getCurrentDownloadSize();
	InvalidateRect(g_hWnd, &g_rcTxtSpeed, false);
	//drawCanvas(g_hBackbuffer);
}

unsigned int  __stdcall ThreadProcFunc(void *arg)
{
	char* game = (char*)arg;

	if (strcmp(game, LAUNCHER_NAME) == 0)
	{
		g_bUpdateSuccessed = check_lobby_new_version();

		if (g_bUpdateLobby)
		{
			timeSetEvent(1000, 1, (LPTIMECALLBACK)onAutoClose, DWORD(1), TIME_ONESHOT);
		}
		else
		{
			timeSetEvent(1, 1, (LPTIMECALLBACK)onAutoClose, DWORD(1), TIME_ONESHOT);
		}
	}
	else
	{
		g_bUpdateSuccessed = check_game_new_version(game);

		if (g_bUpdateGame)
		{
			timeSetEvent(1000, 1, (LPTIMECALLBACK)onAutoClose, DWORD(1), TIME_ONESHOT);
		}
		else
		{
			timeSetEvent(1, 1, (LPTIMECALLBACK)onAutoClose, DWORD(1), TIME_ONESHOT);
		}
	}

	
	
	
	

	return 0;
}

HBITMAP CreateGDIBitmap(int nWid, int nHei, HDC dc, void* pSrcBits, int length)
{
	HRESULT hr = GetLastError();
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nWid;
	bmi.bmiHeader.biHeight = -nHei; // top-down image 
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;

	void * llBits = NULL;
	HBITMAP hBmp = ::CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &llBits, 0, 0);
	ReleaseDC(g_hWnd, dc);
	memcpy(llBits, pSrcBits, length);
	hr = GetLastError();
	return hBmp;
}

static void pngReadCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
	tImageSource* isource = (tImageSource*)png_get_io_ptr(png_ptr);

	if ((int)(isource->offset + length) <= isource->size)
	{
		memcpy(data, isource->data + isource->offset, length);
		isource->offset += length;
	}
	else
	{
		png_error(png_ptr, "pngReaderCallback failed");
	}
}

void premultipliedAlpha(unsigned char* _data, int _width, int _height)
{
	bool isGlRender = false;
	unsigned int* fourBytes = (unsigned int*)_data;
	for (int i = 0; i < _width * _height; i++)
	{
		unsigned char* p = _data + i * 4;
		if (isGlRender)
		{
			fourBytes[i] = CC_RGB_PREMULTIPLY_ALPHA(p[0], p[1], p[2], p[3]);
		}
		else
		{
			fourBytes[i] = CC_RGB_PREMULTIPLY_ALPHA(p[2], p[1], p[0], p[3]);
		}
	}
}

unsigned char* initWithPngData(const unsigned char * data, int dataLen, int& _width, int& _height, int& _dataLen)
{
	png_byte        header[PNGSIGSIZE] = { 0 };
	png_structp     png_ptr = 0;
	png_infop       info_ptr = 0;
	unsigned char* _data = NULL;

	do
	{
		// png header len is 8 bytes
		CC_BREAK_IF(dataLen < PNGSIGSIZE);

		// check the data is png or not
		memcpy(header, data, PNGSIGSIZE);
		CC_BREAK_IF(png_sig_cmp(header, 0, PNGSIGSIZE));

		// init png_struct
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		CC_BREAK_IF(!png_ptr);

		// init png_info
		info_ptr = png_create_info_struct(png_ptr);
		CC_BREAK_IF(!info_ptr);


		// set the read call back function
		tImageSource imageSource;
		imageSource.data = (unsigned char*)data;
		imageSource.size = dataLen;
		imageSource.offset = 0;
		png_set_read_fn(png_ptr, &imageSource, pngReadCallback);

		// read png header info

		// read png file info
		png_read_info(png_ptr, info_ptr);

		_width = png_get_image_width(png_ptr, info_ptr);
		_height = png_get_image_height(png_ptr, info_ptr);
		png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
		png_uint_32 color_type = png_get_color_type(png_ptr, info_ptr);

		//CCLOG("color type %u", color_type);

		// force palette images to be expanded to 24-bit RGB
		// it may include alpha channel
		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			png_set_palette_to_rgb(png_ptr);
		}
		// low-bit-depth grayscale images are to be expanded to 8 bits
		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		{
			bit_depth = 8;
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		}
		// expand any tRNS chunk data into a full alpha channel
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		{
			png_set_tRNS_to_alpha(png_ptr);
		}
		// reduce images with 16-bit samples to 8 bits
		if (bit_depth == 16)
		{
			png_set_strip_16(png_ptr);
		}

		// Expanded earlier for grayscale, now take care of palette and rgb
		if (bit_depth < 8)
		{
			png_set_packing(png_ptr);
		}
		// update info
		png_read_update_info(png_ptr, info_ptr);
		bit_depth = png_get_bit_depth(png_ptr, info_ptr);
		color_type = png_get_color_type(png_ptr, info_ptr);

		/*switch (color_type)
		{
		case PNG_COLOR_TYPE_GRAY:
		_renderFormat = Texture2D::PixelFormat::I8;
		break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
		_renderFormat = Texture2D::PixelFormat::AI88;
		break;
		case PNG_COLOR_TYPE_RGB:
		_renderFormat = Texture2D::PixelFormat::RGB888;
		break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
		_renderFormat = Texture2D::PixelFormat::RGBA8888;
		break;
		default:
		break;
		}*/

		// read png data
		png_size_t rowbytes;
		png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * _height);

		rowbytes = png_get_rowbytes(png_ptr, info_ptr);

		_dataLen = rowbytes * _height;
		_data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
		if (!_data)
		{
			if (row_pointers != nullptr)
			{
				free(row_pointers);
			}
			break;
		}

		for (unsigned short i = 0; i < _height; ++i)
		{
			row_pointers[i] = _data + i*rowbytes;
		}
		png_read_image(png_ptr, row_pointers);

		png_read_end(png_ptr, nullptr);

		// premultiplied alpha for RGBA8888
		if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		{
			premultipliedAlpha(_data, _width, _height);
		}
		

		if (row_pointers != nullptr)
		{
			free(row_pointers);
		}

	} while (0);

	if (png_ptr)
	{
		png_destroy_read_struct(&png_ptr, (info_ptr) ? &info_ptr : 0, 0);
	}

	return _data;
}

HBITMAP loadBitmapFromRc(HDC _dc, int rcId)
{
	HBITMAP rtValue;

	HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(rcId), _T("PNG"));
	HGLOBAL hImgData = ::LoadResource(hInst, hRsrc);
	DWORD len = ::SizeofResource(hInst, hRsrc);
	LPVOID lpVoid = ::LockResource(hImgData);

	int rgbLen = 0;
	int width = 0;
	int height = 0;
	unsigned char* rgbData = initWithPngData((unsigned char*)lpVoid, len, width, height, rgbLen);
	rtValue = CreateGDIBitmap(width, height, _dc, rgbData, rgbLen);
	
	free(rgbData);

	return rtValue;
}

TCHAR* szShadowWindowClass = L"shadow window";
LRESULT CALLBACK ShadowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		//hdc = BeginPaint(hWnd, &ps);
		//// TODO:  在此添加任意绘图代码...
		//EndPaint(hWnd, &ps);
		//return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	case WM_ERASEBKGND:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		/*case WM_DESTROY:
		PostQuitMessage(0);
		break;*/
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND _createShadowWindow(int width, int height)
{
	HRESULT hr;
	HWND parent = g_hWnd;
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = ShadowWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szShadowWindowClass;
	wcex.hIconSm = NULL;

	ATOM reg = RegisterClassEx(&wcex);

	int nWid = width;
	int nHei = height;
	RECT parentRect;

	GetWindowRect(parent, &parentRect);
	int parentWidth = parentRect.right - parentRect.left;
	int parentHeight = parentRect.bottom - parentRect.top;
	int x = parentRect.left - (nWid - parentWidth) / 2;
	int y = parentRect.top - (nHei - parentHeight) / 2;

	HWND hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
		szShadowWindowClass,
		L"Shadow",
		WS_POPUP,
		x, y,
		nWid, nHei,
		NULL, // No parent window
		NULL, // No window menu
		hInst,
		NULL);
	//ShowWindow(hWnd, SW_SHOW);

	return hWnd;
}

void _makeTransparent(HWND _hwnd, HBITMAP _bmp)
{
	BITMAP bmp;
	GetObject(_bmp, sizeof(BITMAP), (LPBYTE)&bmp);

	BYTE byAlpha = 0xFF;
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, byAlpha, AC_SRC_ALPHA };

	HDC hDc = GetDC(_hwnd);
	HDC hmemdc = CreateCompatibleDC(hDc);

	HGDIOBJ hOld = ::SelectObject(hmemdc, _bmp);


	BLENDFUNCTION pb_;
	pb_.AlphaFormat = 1;
	pb_.BlendOp = 0;
	pb_.BlendFlags = 0;
	pb_.SourceConstantAlpha = 0xFF;

	RECT parentRect;
	GetWindowRect(g_hWnd, &parentRect);

	POINT  pt_;
	pt_.x = parentRect.left;
	pt_.y = parentRect.top;
	SIZE   size_;
	size_.cx = bmp.bmWidth;
	size_.cy = bmp.bmHeight;
	POINT  ptSrc;
	ptSrc.x = 0;
	ptSrc.y = 0;


	//UpdateLayeredWindow(g_hWnd, _dc, &pt_, &size_, hmemdc, &ptSrc, 0, &pb_, ULW_ALPHA);
	UpdateLayeredWindow(_hwnd, NULL, NULL, &size_, hmemdc, &ptSrc, 0, &pb_, ULW_ALPHA);
	

	::SelectObject(hmemdc, hOld);
	DeleteDC(hmemdc);
}

void onWindowsMove(int x, int y)
{
	HWND parent = g_hWnd;
	HWND child = FindWindowA("shadow window", NULL);
	//HWND child = FindWindowExA(parent, NULL, "shadow window", NULL);

	RECT parentRect;
	RECT childRect;
	GetWindowRect(parent, &parentRect);
	GetWindowRect(child, &childRect);
	int parentWidth = parentRect.right - parentRect.left;
	int parentHeight = parentRect.bottom - parentRect.top;
	int childWidth = childRect.right - childRect.left;
	int childHeight = childRect.bottom - childRect.top;

	MoveWindow(child, x - (childWidth - parentWidth) / 2, y-5, childWidth, childHeight, true);
}

bool active_enable = true;
void onWindowsActive(int action)
{
	HWND parent = g_hWnd;
	HWND child = FindWindowA("shadow window", NULL);


	if (action == WA_INACTIVE)
	{
		active_enable = true;
	}
	else
	{
		if (active_enable)
		{
			BringWindowToTop(child);
			BringWindowToTop(parent);
			active_enable = false;
		}

	}

}

//DWORD GetPidByProcessName(TCHAR* pProcess)
//{
//	HANDLE hsnapshot;
//	PROCESSENTRY32 lppe;
//	hsnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
//	if (hsnapshot == NULL)
//	{
//		return 0;
//	}
//	lppe.dwSize = sizeof(lppe);
//	if (!::Process32First(hsnapshot, &lppe))
//	{
//		return false;
//	}
//	do
//	{
//		TCHAR strprocess[128] = {};
//		wcscat(strprocess, lppe.szExeFile);
//		if (_tcscmp(strprocess, pProcess) == 0)
//		{
//			return lppe.th32ProcessID;
//		}
//	} while (::Process32Next(hsnapshot, &lppe));
//	return 1;
//}
//
//HWND GetHwndByPid(DWORD dwProcessID)
//{
//	HWND hWnd = ::GetTopWindow(0);
//	while (hWnd)
//	{
//		DWORD pid = 0;
//		DWORD dwTheardID = ::GetWindowThreadProcessId(hWnd, &pid);
//		if (dwTheardID != 0)
//		{
//			if (pid == dwProcessID)
//			{
//				return hWnd;
//			}
//		}
//		hWnd = ::GetNextWindow(hWnd, GW_HWNDNEXT);
//	}
//	return hWnd;
//}
//
//void ShowForeGround(HWND hWnd)
//{
//	if (hWnd)
//	{
//		::ShowWindow(hWnd, SW_NORMAL);
//		::SetForegroundWindow(hWnd);
//		::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//	}
//	else
//	{
//		MessageBox(NULL, L"未能找到该窗口", L"Error", MB_OK);
//	}
//}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
#ifdef _DEBUG
	MessageBoxA(0, "LiveUpdate", 0, 0);
#endif
	//
	//bool vm = g_muti.checkVM();
	//if (!vm)
	//{
	//	MessageBox(NULL, L"该进程不能在虚拟环境下运行", L"错误", MB_OK);
	//	//MessageBox("不能在虚拟机环境下运行");
	//	return 1;
	//}


	//bool mutiOpen = g_muti.checkMultiOpen();

	//if (!mutiOpen)
	//{
	//	DWORD dwPid = GetPidByProcessName(_T("LiveUpdate.exe"));
	//	HWND hWnd = GetHwndByPid(dwPid);
	//	ShowForeGround(hWnd);
	//	return 1;
	//}

	
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	LPWSTR *szArgList;
	int argCount;

	szArgList = CommandLineToArgvW(lpCmdLine, &argCount);
	if (szArgList == NULL)
	{
		MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
		return 10;
	}


 	// TODO:  在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_LIVEUPDATE, szWindowClass, MAX_LOADSTRING);
	LoadStringA(hInstance, IDS_DISPLAYCAPTION, g_szDisplayCaption, MAX_LOADSTRING);
	LoadStringA(hInstance, IDS_DISPLAYMD5, g_szDisplayMD5, MAX_LOADSTRING);
	LoadStringA(hInstance, IDS_DISPLAYSHA1, g_szDisplaySHA1, MAX_LOADSTRING);
	LoadStringA(hInstance, IDS_DOWNLOADSPEED, g_szDisplayDownloadSpeed, MAX_LOADSTRING);
	LoadStringA(hInstance, IDS_PERSENTAGE, g_szDisplayPercentage, MAX_LOADSTRING);
	//HBITMAP imgBg = LoadBitmap(hInstance, MAKEINTRESOURCE(IDI_SMALL));
	g_bmpBg = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG1);
	g_bmpProgressLeft = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG2);
	g_bmpProgressCenter = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG6);
	g_bmpProgressRight = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG4);
	g_bmpSpark = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG5);

	g_bmpMinimizeNormal = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG8);
	g_bmpMinimizeHover = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG9);
	g_bmpMinimizeClick = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG10);
	g_bmpCloseNormal = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG11);
	g_bmpCloseHover = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG12);
	g_bmpCloseClick = loadBitmapFromRc(GetDC(g_hWnd), IDB_PNG13);

	{
		BITMAP bmp;
		GetObject(g_bmpMinimizeNormal, sizeof(BITMAP), (LPBYTE)&bmp);
		g_rcBtnMinimize.left = 396;
		g_rcBtnMinimize.top = 0;
		g_rcBtnMinimize.right = g_rcBtnMinimize.left + bmp.bmWidth;
		g_rcBtnMinimize.bottom = g_rcBtnMinimize.top + bmp.bmHeight;

		GetObject(g_bmpCloseNormal, sizeof(BITMAP), (LPBYTE)&bmp);
		g_rcBtnClose.left = 425;
		g_rcBtnClose.top = 0;
		g_rcBtnClose.right = g_rcBtnClose.left + bmp.bmWidth;
		g_rcBtnClose.bottom = g_rcBtnClose.top + bmp.bmHeight;

		g_rcTxtCaption.left = 90;
		g_rcTxtCaption.right = 380;
		g_rcTxtCaption.top = 40;
		g_rcTxtCaption.bottom = 70;

		g_rcTxtSpeed.left = 85;
		g_rcTxtSpeed.right = 250;
		g_rcTxtSpeed.top = 105;
		g_rcTxtSpeed.bottom = 130;

		g_rcTxtPercentage.left = 270;
		g_rcTxtPercentage.right = 390;
		g_rcTxtPercentage.top = 105;
		g_rcTxtPercentage.bottom = 130;

		GetObject(g_bmpSpark, sizeof(BITMAP), (LPBYTE)&bmp);
		g_rcProcessbar.left = 72;
		g_rcProcessbar.top = 69 - bmp.bmHeight /2;
		g_rcProcessbar.right = g_rcProcessbar.left + 300;
		g_rcProcessbar.bottom = g_rcProcessbar.top + bmp.bmHeight;

	}
	BITMAP bmp;
	GetObject(g_bmpBg, sizeof(BITMAP), (LPBYTE)&bmp);
	g_nWinWidth = bmp.bmWidth;
	g_nWinHeight = bmp.bmHeight;

	MyRegisterClass(hInstance);
	

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		MessageBoxA(0, "Failed to Init Instance!", 0, 0);
		return FALSE;
	}

	GetObject(g_bmpBgFrame, sizeof(BITMAP), (LPBYTE)&bmp);
	HWND hwnd = _createShadowWindow(bmp.bmWidth, bmp.bmHeight);
	g_bmpBgFrame = loadBitmapFromRc(GetDC(hwnd), IDB_PNG7);
	_makeTransparent(hwnd, g_bmpBgFrame);
	g_hShadowWnd = hwnd;
	RECT parentRect;
	GetWindowRect(g_hWnd, &parentRect);
	onWindowsMove(parentRect.left, parentRect.top);
	onWindowsActive(1);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LIVEUPDATE));

	MMRESULT timerId;
	std::wstring wstrServerAddr;
	std::wstring wstrDst;
	std::wstring wstrName;
	std::wstring wstrHostId;
	for (int i = 0; i < argCount; i++)
	{
		std::wstring str = szArgList[i];
		std::wstring::size_type idx = str.find(L"server=");
		if (idx != std::wstring::npos)
		{
			wstrServerAddr = str.substr(idx + strlen("server="));
		}
		idx = str.find(L"dst=");
		if (idx != std::wstring::npos)
		{
			wstrDst = str.substr(idx + strlen("dst="));
		}
		idx = str.find(L"name=");
		if (idx != std::wstring::npos)
		{
			wstrName = str.substr(idx + strlen("name="));
		}
		idx = str.find(L"host_id=");
		if (idx != std::wstring::npos)
		{
			wstrHostId = str.substr(idx + strlen("host_id="));
		}
	}

	g_nHostId = _wtoi(wstrHostId.c_str());

	char strName[MAX_PATH];
	memset(strName, 0, sizeof(char)*MAX_PATH);
	util_TcharToChar(wstrName.c_str(), strName);

	char strDst[MAX_PATH];
	memset(strDst, 0, sizeof(char)*MAX_PATH);
	util_TcharToChar(wstrDst.c_str(), strDst);
	
	
	if (strcmp(strDst, LAUNCHER_NAME) == 0)
	{
		sprintf(g_szDisplayCaption, "布佳更新中");
	}
	else
	{
		//sprintf(g_szDisplayCaption, "《%s》更新中", strName);
		sprintf(g_szDisplayCaption, "布佳更新中");
	}
	
	InvalidateRect(g_hWnd, &g_rcTxtCaption, false);
	/*std::string strDst(wstrDst.length(), ' ');
	std::copy(wstrDst.begin(), wstrDst.end(), strDst.begin());*/
	

	std::string strServerAddr(wstrServerAddr.length(), ' ');
	std::copy(wstrServerAddr.begin(), wstrServerAddr.end(), strServerAddr.begin());

	//set_server_addr(strServerAddr.c_str());
	//set_display_md5_cb(displayMd5Callbcak);
	//set_display_sha1_cb(displaySha1Callbcak);
	//set_display_finish_file_cb(displayFinishFileNameCallbcak);
	//set_display_processbar_cb(displayProcessBarValue);
	set_display_validate_cb(displayValidateCallbcak);
	set_display_update_liveupdate(displayUpdateLiveUpdateCallbcak);
	set_display_percentage_cb(displayPercentageCallbcak);
	set_display_speed_cb(displaySpeedCallbcak);
	set_on_lobby_new_version_cb(onLobbyNewVersion);
	set_on_game_new_version_cb(onGameNewVersion);
	

	initNetwork();
	showMainWindow();


//#ifdef _DEBUG
//	g_bUpdateSuccessed = true;
//	timeSetEvent(10000, 1, (LPTIMECALLBACK)onAutoClose, DWORD(1), TIME_ONESHOT);
//#else
	unsigned int thread_id;
	_beginthreadex(NULL, 0, ThreadProcFunc, (void*)strDst, 0, (unsigned*)&thread_id);
//#endif

	/*unsigned int thread_id;
	_beginthreadex(NULL, 0, ThreadProcFunc, (void*)strDst, 0, (unsigned*)&thread_id);*/

	
	//g_hBackbuffer = _createGdiBackBuffer(g_hWnd);
	//drawCanvas(g_hBackbuffer);
	//timeSetEvent(500, 1, (LPTIMECALLBACK)onRefreshDownloadSpeed, DWORD(1), TIME_PERIODIC);

	// 主消息循环: 
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	//timeKillEvent(timerId);
	closeNetwork();

	return (int) msg.wParam;
}



//
//  函数:  MyRegisterClass()
//
//  目的:  注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIVEUPDATE));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数:  InitInstance(HINSTANCE, int)
//
//   目的:  保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

   hInst = hInstance; // 将实例句柄存储在全局变量中

   //get screen size
   int scrWidth = GetSystemMetrics(SM_CXSCREEN);
   int scrHeight = GetSystemMetrics(SM_CYSCREEN);
   //g_nWinWidth = 476;
   //g_nWinHeight = 175;
   int winPosX = (scrWidth - g_nWinWidth) / 2;
   int winPosY = (scrHeight - g_nWinHeight) / 2;

   g_hWnd = CreateWindow(szWindowClass, szTitle, WS_POPUP,
	   winPosX, winPosY, g_nWinWidth, g_nWinHeight, NULL, NULL, hInstance, NULL);

   //g_hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
   /*g_hWnd = CreateWindowExW(WS_EX_LAYERED ,
	   szWindowClass,
	   szTitle,
	   WS_POPUP,
	   winPosX, winPosY,
	   g_nWinWidth, g_nWinHeight,
	   NULL, // No parent window
	   NULL, // No window menu
	   hInstance,
	   NULL);*/


   if (!g_hWnd)
   {
	  MessageBoxA(0, "Failed to create window", 0, 0);
      return FALSE;
   }

  

   return TRUE;
}

bool drawBitmap(HDC _dc, HBITMAP _bmp, int xPos, int yPos)
{
	BYTE byAlpha = 0xFF;
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, byAlpha, AC_SRC_ALPHA };

	HDC hmemdc = CreateCompatibleDC(_dc);
	HGDIOBJ hOld = ::SelectObject(hmemdc, _bmp);


	BITMAP bmp;
	GetObject(_bmp, sizeof(BITMAP), (LPBYTE)&bmp);


	BOOL bOK = ::AlphaBlend(_dc, xPos, yPos, bmp.bmWidth, bmp.bmHeight, hmemdc, 0, 0, bmp.bmWidth, bmp.bmHeight, bf);
	HRESULT hr = GetLastError();
	//UpdateLayeredWindow(g_hWnd, _dc, &pt_, &size_, hmemdc_dst, &ptSrc, 0, &pb_, ULW_ALPHA);

	::SelectObject(hmemdc, hOld);
	DeleteDC(hmemdc);
	return bOK;
}

void drawBitmap_xbias(HDC _dc, HBITMAP _bmp, int xPos, int yPos, int xbias)
{
	BYTE byAlpha = 0xFF;
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, byAlpha, AC_SRC_ALPHA };

	HDC hmemdc = CreateCompatibleDC(_dc);
	HGDIOBJ hOld = ::SelectObject(hmemdc, _bmp);


	BITMAP bmp;
	GetObject(_bmp, sizeof(BITMAP), (LPBYTE)&bmp);


	BOOL bOK = ::AlphaBlend(_dc, xPos, yPos, bmp.bmWidth - xbias, bmp.bmHeight, hmemdc, xbias, 0, bmp.bmWidth - xbias, bmp.bmHeight, bf);
	HRESULT hr = GetLastError();
	//UpdateLayeredWindow(g_hWnd, _dc, &pt_, &size_, hmemdc_dst, &ptSrc, 0, &pb_, ULW_ALPHA);

	::SelectObject(hmemdc, hOld);
	DeleteDC(hmemdc);
}

void drawBitmap(HDC _dc, HBITMAP _bmp, int xPos, int yPos, int width)
{
	BYTE byAlpha = 0xFF;
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, byAlpha, AC_SRC_ALPHA };

	HDC hmemdc = CreateCompatibleDC(_dc);
	HGDIOBJ hOld = ::SelectObject(hmemdc, _bmp);


	BITMAP bmp;
	GetObject(_bmp, sizeof(BITMAP), (LPBYTE)&bmp);


	BOOL bOK = ::AlphaBlend(_dc, xPos, yPos, width, bmp.bmHeight, hmemdc, 0, 0, width, bmp.bmHeight, bf);
	HRESULT hr = GetLastError();

	::SelectObject(hmemdc, hOld);
	DeleteDC(hmemdc);
}

void drawProgress(HDC _dc, int value, int xPos, int yPos)
{

	BITMAP bmpCenter;
	GetObject(g_bmpProgressCenter, sizeof(BITMAP), (LPBYTE)&bmpCenter);

	BITMAP bmpLeft;
	GetObject(g_bmpProgressLeft, sizeof(BITMAP), (LPBYTE)&bmpLeft);

	BITMAP bmpRight;
	GetObject(g_bmpProgressRight, sizeof(BITMAP), (LPBYTE)&bmpRight);

	BITMAP bmpSpark;
	GetObject(g_bmpSpark, sizeof(BITMAP), (LPBYTE)&bmpSpark);

	int centerWidth = bmpCenter.bmWidth;
	int barWidth = centerWidth * (value / 100.0f);
	//int xbias = barWidth < bmpSpark.bmWidth ? (bmpSpark.bmWidth - barWidth-16) : 0;
	int xbias = bmpSpark.bmWidth - barWidth - 16;
	if (xbias<0)
	{
		xbias = 0;
	}

	drawBitmap(_dc, g_bmpProgressLeft, xPos, yPos);
	drawBitmap(_dc, g_bmpProgressCenter, bmpLeft.bmWidth + xPos, yPos, barWidth);
	drawBitmap(_dc, g_bmpProgressRight, bmpLeft.bmWidth + barWidth + xPos, yPos);
	drawBitmap_xbias(_dc, g_bmpSpark, bmpLeft.bmWidth + barWidth + bmpRight.bmWidth + xPos - (bmpSpark.bmWidth - xbias), yPos - (bmpSpark.bmHeight - bmpCenter.bmHeight) / 2 - 1, xbias);
}

void drawCanvas(HDC hdc)
{
	// window
	bool ret = drawBitmap(hdc, g_bmpBg, 0, 0);

	// minimize button
	if (g_bIsInsideMinimize)
	{
		ret = drawBitmap(hdc, g_bmpMinimizeNormal, g_rcBtnMinimize.left, g_rcBtnMinimize.top);
	}
	else
	{
		ret = drawBitmap(hdc, g_bmpMinimizeHover, g_rcBtnMinimize.left, g_rcBtnMinimize.top);
	}


	// close button
	if (g_bIsInsideClose)
	{
		ret = drawBitmap(hdc, g_bmpCloseNormal, g_rcBtnClose.left, g_rcBtnClose.top);
	}
	else
	{
		ret = drawBitmap(hdc, g_bmpCloseHover, g_rcBtnClose.left, g_rcBtnClose.top);
	}

	//MessageBox(0, 0, 0, 0);
	drawProgress(hdc, g_nProgressValue, 72, 69);

	SetBkMode(hdc, TRANSPARENT);

	if (!g_hFontCaption)
	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -MulDiv(14, GetDeviceCaps(hdc, LOGPIXELSY), 72);;
		lf.lfWeight = FW_NORMAL;
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei");
		g_hFontCaption = CreateFontIndirect(&lf);
	}

	if (!g_hFontDetail)
	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(lf));
		lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);;
		lf.lfWeight = FW_NORMAL;
		wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei");
		g_hFontDetail = CreateFontIndirect(&lf);
	}

	SelectObject(hdc, g_hFontCaption);
	SetTextColor(hdc, 0xffffff);

	// caption
	ret = DrawTextA(hdc, g_szDisplayCaption, strlen(g_szDisplayCaption), &g_rcTxtCaption, DT_CENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

	// change font
	SelectObject(hdc, g_hFontDetail);
	SetTextColor(hdc, 0xb7b7b7);

	// speed
	ret = DrawTextA(hdc, g_szDisplayDownloadSpeed, strlen(g_szDisplayDownloadSpeed), &g_rcTxtSpeed, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);

	// percentage
	ret = DrawTextA(hdc, g_szDisplayPercentage, strlen(g_szDisplayPercentage), &g_rcTxtPercentage, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
}

HDC _createGdiBackBuffer(HWND hWnd)
{
	RECT rect;
	//get the client area
	GetClientRect(hWnd, &rect);
	int cxClient = rect.right;
	int cyClient = rect.bottom;
	//now to create the back buffer
	//create a memory device context

	HDC hdcBackBuffer = CreateCompatibleDC(NULL);

	//get the DC for the front buffer
	HDC hdc = GetDC(hWnd);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc,
		cxClient,
		cyClient);

	//select the bitmap into the memory device context
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcBackBuffer, hBitmap);

	//don't forget to release the DC!
	ReleaseDC(hWnd, hdc);

	return hdcBackBuffer;
}

//
//  函数:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	RECT rcText;
	


	switch (message)
	{
	case WM_CREATE:
		/*g_hProcess = CreateWindowEx(0, PROGRESS_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
			20, 20, 352, 17,
			hWnd, NULL, hInst, NULL);*/
		break;
	case WM_COMMAND:
		break;
	/*case PBM_SETPOS:
		g_nProgressValue = lParam;
		break;*/
	case WM_MOVE:
		
		
		onWindowsMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_ACTIVATE:
		onWindowsActive(LOWORD(wParam));
		break;
	case WM_LBUTTONDOWN:
		if (g_bIsInsideMinimize)
		{
			//InvalidateRect(hWnd, NULL, false);
			SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			HWND hParentWnd = getLobbyWinHandleByProcessId(g_nHostId);
			SendMessage(hParentWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
			g_bIsInsideMinimize = false;
		}
		if (g_bIsInsideClose)
		{
			//notify_lobby_finished();
			notify_lobby_cancel();
			PostQuitMessage(0);
		}
		g_bIsMinimizeDown = true;
		g_bIsCloseDown = true;
		break;
	case WM_LBUTTONUP:
		
		g_bIsMinimizeDown = false;
		g_bIsCloseDown = false;
		break;
	case WM_MOUSEMOVE:
	{
		
		//drawProgress(hdc, rand() % 100, 77, 86);
		const int x = GET_X_LPARAM(lParam);
		const int y = GET_Y_LPARAM(lParam);
		POINT mouse;
		mouse.x = x;
		mouse.y = y;


		BITMAP bmp;
		GetObject(g_bmpMinimizeNormal, sizeof(BITMAP), (LPBYTE)&bmp);

		RECT rcMinimize;
		rcMinimize.left = 389;
		rcMinimize.top = 0;
		rcMinimize.right = rcMinimize.left + bmp.bmWidth;
		rcMinimize.bottom = rcMinimize.top + bmp.bmHeight;
		bool isInMinimize = PtInRect(&rcMinimize, mouse);


		GetObject(g_bmpCloseNormal, sizeof(BITMAP), (LPBYTE)&bmp);

		RECT rcClose;
		rcClose.left = 418;
		rcClose.top = 0;
		rcClose.right = rcClose.left + bmp.bmWidth;
		rcClose.bottom = rcClose.top + bmp.bmHeight;
		bool isInClose = PtInRect(&rcClose, mouse);

		if (isInMinimize != g_bIsInsideMinimize)
		{
			g_bIsInsideMinimize = isInMinimize;
			
			InvalidateRect(hWnd, &g_rcBtnMinimize, false);
		}

		if (isInClose != g_bIsInsideClose)
		{
			g_bIsInsideClose = isInClose;

			InvalidateRect(hWnd, &g_rcBtnClose, false);
		}

		break;
	}
	case WM_ERASEBKGND:
		return true;
		break;
	case WM_PAINT:
	//case WM_ERASEBKGND:
	{
		//displayPercentageCallbcak(rand() % 100);
		HDC hdc = BeginPaint(hWnd, &ps);
		g_hDC = hdc;

		//g_hBackbuffer = _createGdiBackBuffer(hWnd);
		

		drawCanvas(hdc);
		/*RECT rect;
		GetClientRect(hWnd, &rect);
		int cxClient = rect.right;
		int cyClient = rect.bottom;

		BOOL ret = BitBlt(hdc,
		0,
		0,
		cxClient,
		cyClient,
		g_hBackbuffer,
		0,
		0,
		SRCCOPY);

		if (ret==false)
		{
		HRESULT hr = GetLastError();
		int d = 1000;
		}*/
		
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_NCHITTEST:
	{
		RECT rect;
		GetWindowRect(hWnd, &rect);

		RECT rc;
		rc.left = 0;
		rc.top = 0;
		rc.right = rect.right - rect.left - 70;
		rc.bottom = 40;

		POINT pt;
		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);
		ScreenToClient(hWnd, &pt);

		if (PtInRect(&rc, pt))
		{
			return HTCAPTION;
		}
		return HTCLIENT;
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
