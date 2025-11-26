#ifndef COMMON_H
#define COMMON_H

#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <vector>
#include <string>

// 前向声明
struct ShortcutItem;
struct CalculationRecord;

// 全局变量声明（在gui_main.cpp中定义）
extern HINSTANCE g_hInstance;
extern HWND g_hMainWindow;
extern HWND g_hEdit;
extern HWND g_hListView;
extern HWND g_hExitCalcButton;
extern HWND g_hSettingsButton;
extern HWND g_hExitBookmarkButton;
extern HWND g_hCalcMenuButton;
extern HWND g_hInputHintLabel;
extern HWND g_hAddBookmarkButton;
extern HFONT g_hFont;

extern bool g_ignoreNextReturn;
extern bool g_windowInitializing;
extern bool g_calculatorMode;
extern bool g_bookmarkMode;
extern bool g_updatingEditBox;

extern std::vector<ShortcutItem> g_shortcuts;
extern std::vector<ShortcutItem> g_searchResults;
extern WCHAR g_currentSearch[1024];
extern std::vector<CalculationRecord> g_calculationHistory;
extern std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks;
extern std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults;
extern std::wstring g_cachedHelpHtml;  // 缓存的帮助信息HTML
extern std::wstring g_cachedSettingsHtml;  // 缓存的设置菜单HTML
extern bool g_helpHtmlCached;  // 帮助信息是否已缓存
extern bool g_settingsHtmlCached;  // 设置菜单是否已缓存

// 常量定义
#define IDC_EDIT 1001
#define IDC_LISTVIEW 1002
#define IDC_EXIT_CALC_BUTTON 1003
#define IDC_SETTINGS_BUTTON 1004
#define IDC_EXIT_BOOKMARK_BUTTON 1013
#define HOTKEY_ID 1
#define HOTKEY_ID_CTRL_F1 2
#define HOTKEY_ID_CTRL_F2 3
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW 1005
#define ID_TRAY_EXIT 1006
#define ID_CONTEXT_DELETE_ITEM 1007
#define ID_CONTEXT_CLEAR_ALL 1008
#define ID_ADD_BOOKMARK_BUTTON 1009
#define ID_SYNC_CHROME_BUTTON 1010
#define ID_CONTEXT_DELETE_BOOKMARK 1011
#define ID_CONTEXT_SYNC_CHROME 1012
#define ID_CONTEXT_COPY 1013
#define ID_SETTINGS_BOOKMARK 1014
#define ID_SETTINGS_EXIT 1015

#ifndef EN_RETURN
#define EN_RETURN 0x0100
#endif

// 函数声明
void InitializeWebView2(HWND hwnd);  // 初始化 WebView2
void UpdateWebView2Content(const WCHAR* htmlContent);  // 更新 WebView2 内容
void CreateWebView2HTML(const std::vector<ShortcutItem>& items, const std::vector<std::wstring>& hints, std::wstring& html);  // 创建 HTML 内容
void UpdateCalculatorModeWebView();  // 更新计算模式下的 WebView2 显示
void ShowBasicUsage();  // 显示基本用法界面

// 类型定义
struct ShortcutItem {
    WCHAR name[256];
    WCHAR path[256];
    int type; // 0 = directory, 1 = URL, 2 = application
    int usageCount;
};

struct CalculationRecord {
    std::wstring expression;
    std::wstring result;
    std::wstring comment;
};

#endif // COMMON_H

