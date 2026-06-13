#ifndef GUI_MAIN_H
#define GUI_MAIN_H

#include <windows.h>

// 全局变量声明
extern HWND g_hMainWindow;
extern HWND g_hEdit;
extern HWND g_hListView;
extern HINSTANCE g_hInstance;

// 全局变量定义 - 工具栏按钮句柄
extern HWND g_hHomeBtn;
extern HWND g_hBookmarkBtn;
extern HWND g_hCalculatorBtn;
extern HWND g_hDirBtn;
extern HWND g_hFileBtn;      // 文件搜索模式按钮
extern HWND g_hShortcutBtn;  // 快捷方式管理模式按钮
extern HFONT g_hFont;

// 函数声明
void ShowShortcutManagementDialog(); // 显示快捷方式管理对话框

#endif // GUI_MAIN_H