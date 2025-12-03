// Everything.h - 动态加载版本
// 修改为不使用dllimport，支持动态加载

#ifndef _EVERYTHING_DYNAMIC_H_
#define _EVERYTHING_DYNAMIC_H_

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// if not defined, version is 1.
#define EVERYTHING_SDK_VERSION								2

#define EVERYTHING_OK										0 // no error detected
#define EVERYTHING_ERROR_MEMORY								1 // out of memory.
#define EVERYTHING_ERROR_IPC								2 // Everything search client is not running
#define EVERYTHING_ERROR_REGISTERCLASSEX					3 // unable to register window class.
#define EVERYTHING_ERROR_CREATEWINDOW						4 // unable to create listening window
#define EVERYTHING_ERROR_CREATETHREAD						5 // unable to create listening thread
#define EVERYTHING_ERROR_INVALIDINDEX						6 // invalid index
#define EVERYTHING_ERROR_INVALIDCALL						7 // invalid call
#define EVERYTHING_ERROR_INVALIDREQUEST						8 // invalid request data, request data first.
#define EVERYTHING_ERROR_INVALIDPARAMETER					9 // bad parameter.

// ... 其他定义保持不变 ...

#ifndef EVERYTHINGAPI
#define EVERYTHINGAPI __stdcall
#endif

// 修改为不使用dllimport
#ifndef EVERYTHINGUSERAPI
#define EVERYTHINGUSERAPI
#endif

// 函数声明 - 使用动态加载
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetSearchW(LPCWSTR lpString);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetSearchA(LPCSTR lpString);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetMatchPath(BOOL bEnable);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetMatchCase(BOOL bEnable);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetMatchWholeWord(BOOL bEnable);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetRegex(BOOL bEnable);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetMax(DWORD dwMax);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetOffset(DWORD dwOffset);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetReplyWindow(HWND hWnd);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetReplyID(DWORD dwId);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetSort(DWORD dwSort);
EVERYTHINGUSERAPI void EVERYTHINGAPI Everything_SetRequestFlags(DWORD dwRequestFlags);

// ... 其他函数声明保持不变 ...

#ifdef __cplusplus
}
#endif

#endif