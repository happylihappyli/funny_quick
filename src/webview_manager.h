#pragma once

#ifndef WEBVIEW_MANAGER_H
#define WEBVIEW_MANAGER_H

#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE

#include <windows.h>
#include "webview2_fix.h"  // WebView2枚举值修复
#include <WebView2.h>

// 避免直接包含有问题的WebView2EnvironmentOptions.h头文件
// 手动声明需要的接口和类型
#include <wrl/client.h>
#include <wrl/event.h>

// 手动声明WebView2环境选项接口
namespace Microsoft {
    namespace Web {
        namespace WebView2 {
            namespace Core {
                // 手动声明ICoreWebView2EnvironmentOptions接口
                MIDL_INTERFACE("2F5D5357-3C72-4A9C-96E9-9A8E2A8D6C7B")
                ICoreWebView2EnvironmentOptions : public IUnknown {
                public:
                    virtual HRESULT STDMETHODCALLTYPE get_AdditionalBrowserArguments(
                        LPWSTR* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_AdditionalBrowserArguments(
                        LPCWSTR value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE get_Language(
                        LPWSTR* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_Language(
                        LPCWSTR value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE get_TargetCompatibleBrowserVersion(
                        LPWSTR* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_TargetCompatibleBrowserVersion(
                        LPCWSTR value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE get_AllowSingleSignOnUsingOSPrimaryAccount(
                        BOOL* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_AllowSingleSignOnUsingOSPrimaryAccount(
                        BOOL value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE get_ExclusiveUserDataFolderAccess(
                        BOOL* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_ExclusiveUserDataFolderAccess(
                        BOOL value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE get_IsCustomCrashReportingEnabled(
                        BOOL* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_IsCustomCrashReportingEnabled(
                        BOOL value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE get_ReleaseChannels(
                        COREWEBVIEW2_RELEASE_CHANNELS* value) = 0;
                    virtual HRESULT STDMETHODCALLTYPE put_ReleaseChannels(
                        COREWEBVIEW2_RELEASE_CHANNELS value) = 0;
                };
                
                // 使用修复后的枚举值
                static const COREWEBVIEW2_RELEASE_CHANNELS kInternalChannel = kInternalChannel_FIXED;
                static const COREWEBVIEW2_RELEASE_CHANNELS kAllChannels = kAllChannels_FIXED;
            }
        }
    }
}
#include <string>
#include <vector>
#include <set>
#include "common.h"

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
void ShowHtmlAddBookmarkDialog();

// 网址收藏相关函数声明（在gui_main.cpp中定义）
void SaveBookmarks();

// 书签模式WebView更新函数声明
void UpdateBookmarkModeWebView();

// 简单输入框函数声明
BOOL GetSimpleInput(LPCWSTR lpCaption, LPCWSTR lpPrompt, LPCWSTR lpDefault, LPWSTR lpResult, int nResultSize);

// 多行输入对话框函数声明
BOOL GetMultiLineInput(LPCWSTR lpCaption, LPWSTR lpName, LPWSTR lpPath, LPWSTR lpComment, int nNameSize, int nPathSize, int nCommentSize);
BOOL GetPropertiesStyleInput(LPCWSTR lpCaption, LPWSTR lpName, LPWSTR lpPath, LPWSTR lpComment, LPWSTR lpIconPath, int shortcutType);

// 编辑和删除书签函数声明
void ShowEditBookmarkDialog(int index);
void DeleteBookmarkFromDisplayList(int index);
void ShowHtmlEditBookmarkDialog(int index);

// 快捷方式编辑函数声明
void ShowEditShortcutDialog(int index);
// 快捷方式添加函数声明
void ShowAddShortcutDialog();

// 快捷方式保存函数声明
void SaveShortcuts();
// 快捷方式加载函数声明
bool LoadShortcuts();

// 图标提取函数声明
BOOL ExtractShortcutIcon(const ShortcutItem& shortcut, WCHAR* iconPath, int iconPathSize);

#endif // WEBVIEW_MANAGER_H
