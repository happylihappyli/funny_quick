#pragma once

#ifndef WEBVIEW_MANAGER_H
#define WEBVIEW_MANAGER_H

#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <string>
#include <vector>
#include <set>

// 使用Microsoft命名空间
using namespace Microsoft::WRL;

// WebView2相关全局变量声明（在webview_manager.cpp中定义）
extern ComPtr<ICoreWebView2Environment> g_webViewEnvironment;
extern ComPtr<ICoreWebView2Controller> g_webViewController;
extern ComPtr<ICoreWebView2> g_webView;
extern HWND g_hWebView2;
extern bool g_settingsMenuMode;
extern std::set<std::wstring> g_expandedPaths;
extern std::wstring g_currentDirPath;

// WebView2初始化函数声明
void InitializeWebView2(HWND hwnd);

// 添加网址对话框函数声明
void ShowSimpleAddBookmarkDialog();

// 网址收藏相关函数声明（在gui_main.cpp中定义）
void SaveBookmarks();

// 简单输入框函数声明
BOOL GetSimpleInput(LPCWSTR lpCaption, LPCWSTR lpPrompt, LPCWSTR lpDefault, LPWSTR lpResult, int nResultSize);

#endif // WEBVIEW_MANAGER_H