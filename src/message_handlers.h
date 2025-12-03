// 消息处理函数声明文件
// 用于重构WindowProc函数，将消息处理逻辑分离到独立的函数

#pragma once

#include <windows.h>

// WM_CREATE消息处理函数
LRESULT HandleWMCreate(HWND hwnd, LPCREATESTRUCTW lpCreateStruct);

// WM_HOTKEY消息处理函数
LRESULT HandleWMHotkey(HWND hwnd, WPARAM wParam);

// WM_DESTROY消息处理函数
LRESULT HandleWMDestroy(HWND hwnd);

// WM_TIMER消息处理函数
LRESULT HandleWMTimer(HWND hwnd, WPARAM wParam);

// WM_SIZE消息处理函数
LRESULT HandleWMSize(HWND hwnd, WPARAM wParam, LPARAM lParam);

// WM_EXITSIZEMOVE消息处理函数
LRESULT HandleWMExitSizeMove(HWND hwnd);

// WM_SETFOCUS消息处理函数
LRESULT HandleWMSetFocus(HWND hwnd, WPARAM wParam);

// WM_NOTIFY消息处理函数
LRESULT HandleWMNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);

// WM_COMMAND消息处理函数
LRESULT HandleWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);

// WM_KEYDOWN消息处理函数
LRESULT HandleWMKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

// WM_CONTEXTMENU消息处理函数
LRESULT HandleWMContextMenu(HWND hwnd, WPARAM wParam);

// 辅助函数声明
void HandleSettingsMenuCommands(HWND hwnd, WPARAM wParam);
void HandleTrayMenuCommands(HWND hwnd, WPARAM wParam);
void HandleBookmarkContextMenuCommands(HWND hwnd, WPARAM wParam);
void HandleExitBookmarkButton(HWND hwnd);
void HandleExitFileButton(HWND hwnd);
void HandleEditControlChange(HWND hwnd);
void HandleEditControlReturn(HWND hwnd);
void HandleListViewDoubleClick(HWND hwnd);
void HideLauncherWindow();
void ExitCalculatorMode();
void ExitBookmarkMode();
void ProcessCommand(const WCHAR* command);