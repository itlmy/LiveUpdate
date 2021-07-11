#include "..\..\..\cocos2d-x-3.7.1\cocos\base\CCDirector.h"
#ifdef __cplusplus                     
extern "C" {
#endif
#include "helper.h"
#include <winsock2.h>
#include <windows.h>
#include "CCImage.h"
#include "utility.h"

TCHAR* szWindowClass = L"shadow window";
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
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void onWindowsMove(int x, int y)
{
	HWND parent = FindWindowA("GLFW30", NULL);
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

	MoveWindow(child, x - (childWidth - parentWidth) / 2, y, childWidth, childHeight, true);
	//SetWindowPos(child, HWND_BOTTOM, x - (childWidth - parentWidth) / 2, y, childWidth, childHeight, SWP_NOSIZE);
	//SetWindowPos(parent, HWND_NOTOPMOST, x, y, parentWidth, parentHeight, SWP_NOSIZE);
	
}

bool active_enable = true;
void onWindowsActive(int action)
{
	HWND parent = FindWindowA("GLFW30", NULL);
	HWND child = FindWindowA("shadow window", NULL);

	/*RECT parentRect;
	RECT childRect;
	GetWindowRect(parent, &parentRect);
	GetWindowRect(child, &childRect);
	int parentWidth = parentRect.right - parentRect.left;
	int parentHeight = parentRect.bottom - parentRect.top;
	int childWidth = childRect.right - childRect.left;
	int childHeight = childRect.bottom - childRect.top;*/

	if (action==WA_INACTIVE)
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
	
	//SetWindowPos(child, HWND_BOTTOM, parentRect.left - (childWidth - parentWidth) / 2, parentRect.top, childWidth, childHeight, SWP_NOSIZE);
	/*if (action==WA_CLICKACTIVE)
	{
	SetWindowPos(child, HWND_BOTTOM, parentRect.left - (childWidth - parentWidth) / 2, parentRect.top, childWidth, childHeight, SWP_NOSIZE);
	SetWindowPos(parent, HWND_NOTOPMOST, parentRect.left, parentRect.top, parentWidth, parentHeight, SWP_NOSIZE);
	}*/
	
	//if (pp%2==0)
	//{
	//	SetWindowPos(parent, HWND_NOTOPMOST, parentRect.left, parentRect.top, parentWidth, parentHeight, SWP_NOSIZE);
	//	//cocos2d::Director::getInstance()->getOpenGLView()->setWindowActiveCallback(NULL);
	//}
	//else
	//{
	//	//cocos2d::Director::getInstance()->getOpenGLView()->setWindowActiveCallback(onWindowsActive);
	//}
	//pp++;
	//SetWindowPos(parent, HWND_NOTOPMOST, parentRect.left, parentRect.top, parentWidth, parentHeight, SWP_NOSIZE);
}

void _makeTransparent(char* filename)
{
	cocos2d::Image* image = new cocos2d::Image;
	bool success = image->initWithImageFile(filename);
	if (!success)
	{
		return;
	}

	int nWid = image->getWidth();
	int nHei = image->getHeight();

	HWND parent = FindWindowA("GLFW30", NULL);
	HWND hWnd = FindWindowA("shadow window", NULL);
	RECT parentRect;
	GetWindowRect(parent, &parentRect);
	int parentWidth = parentRect.right - parentRect.left;
	int parentHeight = parentRect.bottom - parentRect.top;

	HDC hDc = GetDC(hWnd);
	HDC hmemdc_dst = CreateCompatibleDC(hDc);
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nWid;
	bmi.bmiHeader.biHeight = -nHei; // top-down image 
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;

	LPVOID* ppBits = NULL;
	HBITMAP hBmp = CreateDIBSection(hDc, &bmi, DIB_RGB_COLORS, (void**)&ppBits, 0, 0);


	memcpy(ppBits, image->getData(), image->getDataLen());

	delete image;

	SelectObject(hmemdc_dst, hBmp);

	BLENDFUNCTION pb_;
	pb_.AlphaFormat = 1;
	pb_.BlendOp = 0;
	pb_.BlendFlags = 0;
	pb_.SourceConstantAlpha = 0xFF;

	POINT  pt_;
	pt_.x = parentRect.left - (nWid - parentWidth) / 2;
	pt_.y = parentRect.top;
	POINT  ptSrc;
	ptSrc.x = 0;
	ptSrc.y = 0;
	SIZE   size_;
	size_.cx = nWid;
	size_.cy = nHei;
	UpdateLayeredWindow(hWnd, hDc, &pt_, &size_, hmemdc_dst, &ptSrc, 0, &pb_, ULW_ALPHA);
	ReleaseDC(hWnd, hmemdc_dst);

	/*HWND parent = FindWindowA("GLFW30", NULL);
	RECT parentRect;
	GetWindowRect(parent, &parentRect);
	onWindowsMove(parentRect.left, parentRect.top);
	ShowWindow(parent, SW_SHOW);*/
}

bool _useWindowShadow(TCHAR* _name)
{
	if (wcscmp(_name, L"none") == 0)
	{
		cocos2d::Director::getInstance()->getOpenGLView()->setWindowMoveCallback(NULL);
		cocos2d::Director::getInstance()->getOpenGLView()->setWindowActiveCallback(NULL);
		return false;
	}
	else
	{
		cocos2d::Director::getInstance()->getOpenGLView()->setWindowMoveCallback(onWindowsMove);
		cocos2d::Director::getInstance()->getOpenGLView()->setWindowActiveCallback(onWindowsActive);
	}
	return true;
}

void _getShadowSize(char* filename, int &width, int &height)
{
	cocos2d::Image* image = new cocos2d::Image;
	bool success = image->initWithImageFile(filename);
	if (!success)
	{
		return;
	}

	width = image->getWidth();
	height = image->getHeight();
	//int nWid = image->getWidth();
	//int nHei = image->getHeight();

	//HWND parent = FindWindowA("GLFW30", NULL);
	//RECT parentRect;
	//GetWindowRect(parent, &parentRect);
	//int parentWidth = parentRect.right - parentRect.left;

	//x = parentRect.left - (nWid - parentWidth) / 2;
	//y = parentRect.top;

	delete image;
}

void _createShadowWindow(int width, int height)
{
	HRESULT hr;
	HWND parent = FindWindowA("GLFW30", NULL);
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	//SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_LAYERED);
	//LONG style = GetWindowLong(hWnd, GWL_STYLE) | WS_POPUP;
	//SetWindowLong(hWnd, GWL_STYLE, style);


	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = ShadowWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
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
		szWindowClass,
		L"Shadow",
		WS_POPUP,
		x, y,
		nWid, nHei,
		NULL, // No parent window
		NULL, // No window menu
		GetModuleHandleW(NULL),
		NULL);
	ShowWindow(hWnd, SW_SHOW);
}

#ifdef __cplusplus
}
#endif