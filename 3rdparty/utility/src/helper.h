#ifndef UTILITY_HELPER_H
#define UTILITY_HELPER_H
#include <winsock2.h>
#include <windows.h>

#ifdef __cplusplus                     
extern "C" {
#endif
bool _useWindowShadow(TCHAR* _name);
void _getShadowSize(char* filename, int &width, int &height);
void _createShadowWindow(int width, int height);
void _makeTransparent(char* filename);
#ifdef __cplusplus
}
#endif

#endif