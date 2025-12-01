#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>  // 用于GET_X_LPARAM和GET_Y_LPARAM宏
#include <tchar.h>
#include <imm.h>
#include <vector>
#include <algorithm>
#include <set>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <shlobj.h>
#include <strsafe.h>
#include <functional>
#include <commctrl.h>  // ListView控件相关API
#include <basetsd.h>   // For INT_PTR definition
#include "resource.h"
#include "logger.h"
#include "webview_manager.h"  // WebView2 管理功能
#include "dir_mode_manager.h"  // 目录浏览模式管理功能

// WebView2 相关头文件
#include <WebView2.h>
#include <wrl.h>  // 用于 Microsoft::WRL::Callback
#include <wrl/event.h>  // 用于事件处理器
using namespace Microsoft::WRL;

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// Define notification codes if not defined
#ifndef EN_RETURN
#define EN_RETURN 0x0100
#endif

// Global variables
HINSTANCE g_hInstance = NULL;
HWND g_hMainWindow = NULL;
HWND g_hEdit = NULL;
HWND g_hListView = NULL;  // ListView控件（保留用于兼容）
HWND g_hExitCalcButton = NULL;  // 退出计算模式按钮
HWND g_hSettingsButton = NULL;   // 设置按钮
HWND g_hExitBookmarkButton = NULL;  // 退出网址收藏模式按钮
HWND g_hCalcMenuButton = NULL;  // 计算模式操作菜单按钮
// Flag to ignore EN_RETURN notifications triggered by focus changes
bool g_ignoreNextReturn = false;
// WebView2 HTML内容缓存
std::wstring g_cachedHelpHtml;  // 缓存的帮助信息HTML
std::wstring g_cachedSettingsHtml;  // 缓存的设置菜单HTML
bool g_helpHtmlCached = false;  // 帮助信息是否已缓存
bool g_settingsHtmlCached = false;  // 设置菜单是否已缓存
bool g_windowInitializing = false;  // 窗口是否正在初始化，防止自动执行

// 字体相关变量
HFONT g_hFont = NULL;  // 全局字体句柄

// 系统托盘相关变量
NOTIFYICONDATA g_notifyIconData = {0};
bool g_trayIconAdded = false;
HMENU g_trayMenu = NULL;

// Subclassing procedure pointer for edit control
WNDPROC g_originalEditProc = NULL;

// Edit control subclassing procedure declaration
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// Constants
#define IDC_EDIT 1001
#define IDC_LISTVIEW 1002  // ListView控件ID，支持双列显示
#define IDC_EXIT_CALC_BUTTON 1003  // 退出计算模式按钮ID
#define IDC_SETTINGS_BUTTON 1004    // 设置按钮ID
#define IDC_EXIT_BOOKMARK_BUTTON 1013  // 退出网址收藏模式按钮ID
#define IDC_CALC_MENU_BUTTON 1016  // 计算模式操作菜单按钮ID
#define HOTKEY_ID 1
#define HOTKEY_ID_CTRL_F1 2
#define HOTKEY_ID_CTRL_F2 3

// 系统托盘相关常量
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW 1005
#define ID_TRAY_EXIT 1006

// 右键菜单常量
#define ID_CONTEXT_DELETE_ITEM 1007  // 删除单个计算结果
#define ID_CONTEXT_CLEAR_ALL 1008    // 清空所有历史记录

// 网址收藏相关常量
#define ID_ADD_BOOKMARK_BUTTON 1009  // 添加网址按钮ID
#define ID_SYNC_CHROME_BUTTON 1010   // 同步Chrome书签按钮ID
#define ID_CONTEXT_DELETE_BOOKMARK 1011  // 删除单个网址
#define ID_CONTEXT_SYNC_CHROME 1012     // 同步Chrome书签
#define ID_CONTEXT_COPY 1013            // 复制选中项
#define ID_SETTINGS_BOOKMARK 1014       // 设置菜单-网址设置
#define ID_SETTINGS_EXIT 1015           // 设置菜单-退出程序

// Types
struct ShortcutItem {
    WCHAR name[256];
    WCHAR path[256];
    int type; // 0 = directory, 1 = URL, 2 = application
    int usageCount;
};

// Global data
std::vector<ShortcutItem> g_shortcuts;
std::vector<ShortcutItem> g_searchResults;
WCHAR g_currentSearch[1024] = {0};

// 计算记录结构体，支持注释功能
struct CalculationRecord {
    std::wstring expression;  // 计算表达式
    std::wstring result;      // 计算结果
    std::wstring comment;     // 注释内容
};

// 计算模式相关变量
bool g_calculatorMode = false;  // 是否处于计算模式
bool g_updatingEditBox = false;  // 是否正在更新编辑框内容，防止触发EN_CHANGE
std::vector<CalculationRecord> g_calculationHistory;  // 计算历史记录

// 网址收藏模式相关变量
bool g_bookmarkMode = false;  // 是否处于网址收藏模式
std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks;  // 网址收藏列表 (名称, URL)
std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults;  // 网址搜索结果
HWND g_hAddBookmarkButton = NULL;  // 添加网址按钮

// 目录浏览模式相关变量
bool g_dirMode = false;  // 是否处于目录浏览模式

// 表达式解析辅助函数声明
void EnterCalculatorMode();
void ExitCalculatorMode();
void ShowCalculatorHelpInfo();
void ShowHelpInfo();
void EvaluateExpression(const WCHAR* expression);
void DisplayCalculationHistory();
void SaveCalculationHistory();
void LoadCalculationHistory();

// 网址收藏功能函数声明
void EnterBookmarkMode();
void ExitBookmarkMode();
void AddBookmark(const WCHAR* name, const WCHAR* url);
void DeleteBookmark(int index);
void SaveBookmarks();
void LoadBookmarks();
void SyncChromeBookmarks();
void SearchBookmarks(const WCHAR* query);
void DisplayBookmarkResults();
bool IsURL(const WCHAR* text);

// 目录浏览功能函数声明


// 网址管理对话框函数声明
INT_PTR CALLBACK BookmarkDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowBookmarkDialog();
void RefreshBookmarkList(HWND hList);
void AddBookmarkFromDialog(HWND hDlg);
void UpdateBookmarkFromDialog(HWND hDlg);
void DeleteBookmarkFromDialog(HWND hDlg);

// 表达式解析辅助函数声明
double parseNumber(const std::wstring& expr, size_t& pos);
double parseTerm(const std::wstring& expr, size_t& pos);
double parseExpression(const std::wstring& expr, size_t& pos);

// 字体相关函数声明
void CreateUIFont();
void ApplyFontToControl(HWND hWnd);
void LayoutControls(int windowWidth, int windowHeight);

// 新增功能函数声明
void ShowSettingsMenu();
void CopySelectedListItem();
void HandleSettingsMenuItemClick(INT_PTR itemIndex); // 处理设置菜单项双击
void ShowShortcutManagementDialog(); // 显示快捷方式管理对话框
void ShowSystemSettingsDialog(); // 显示系统设置对话框
void ShowAboutDialog(); // 显示关于对话框

// 窗口大小记忆功能函数声明
void SaveWindowSettings();
void LoadWindowSettings(int& x, int& y, int& width, int& height);

// HTML模板读取辅助函数声明
std::wstring ReadHtmlTemplate(const std::wstring& filePath);

// 日志功能已移至 logger.cpp

// 表达式解析辅助函数实现
double parseNumber(const std::wstring& expr, size_t& pos) {
    std::wstring numStr;
    while (pos < expr.length() && (iswdigit(expr[pos]) || expr[pos] == L'.')) {
        numStr += expr[pos];
        pos++;
    }
    return numStr.empty() ? 0.0 : _wtof(numStr.c_str());
}

double parseTerm(const std::wstring& expr, size_t& pos) {
    double value = parseNumber(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'*' || expr[pos] == L'/')) {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseNumber(expr, pos);
        
        if (op == L'*') {
            value *= nextValue;
        } else if (op == L'/' && nextValue != 0) {
            value /= nextValue;
        } else {
            LogToFile("parseTerm: 除零错误");
            throw std::exception("除零错误");
        }
    }
    
    return value;
}

double parseExpression(const std::wstring& expr, size_t& pos) {
    double value = parseTerm(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'+' || expr[pos] == L'-')) {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseTerm(expr, pos);
        
        if (op == L'+') {
            value += nextValue;
        } else {
            value -= nextValue;
        }
    }
    
    return value;
}

// Forward declarations
void ExecuteSelectedItem(INT_PTR index);
void ProcessCommand(const WCHAR* command);
void InitializeCommonShortcuts();
void SearchAndDisplayResults(const WCHAR* query);
void ShowLauncherWindow();
void HideLauncherWindow();
void AddDesktopShortcuts();
void SetEnglishInputMethod();
void UpdateListViewColumns();  // 根据当前模式更新ListView列标题
void AddHintRowToListView(const WCHAR* hintText);  // 在ListView第一行添加提示信息
void AddMultiLineHintsToListView(const WCHAR* hints[], int hintCount);  // 在ListView前面添加多行提示信息
int GetHintRowCount();  // 获取ListView前面提示行的数量
INT_PTR GetFirstActualItemIndex();  // 获取第一个实际项目（跳过提示行）的索引
void LogListViewContents();  // 打印ListView所有内容到日志

// WebView2 相关函数声明
void InitializeWebView2(HWND hwnd);  // 初始化 WebView2
void UpdateWebView2Content(const WCHAR* htmlContent);  // 更新 WebView2 内容
void CreateWebView2HTML(const std::vector<ShortcutItem>& items, const std::vector<std::wstring>& hints, std::wstring& html);  // 创建 HTML 内容
void UpdateCalculatorModeWebView();  // 刷新计算模式的 WebView2 显示
void UpdateSettingsMenuWebView();  // 刷新设置菜单的 WebView2 显示
void UpdateHelpInfoWebView();  // 刷新帮助信息的 WebView2 显示
void UpdateBookmarkModeWebView();  // 刷新网址收藏模式的 WebView2 显示
void ShowBasicUsage();  // 显示基本用法界面

// 系统托盘相关函数声明
void AddTrayIcon();
void RemoveTrayIcon();
void CreateTrayMenu();
void HandleTrayMessage(LPARAM lParam);

// 创建UI字体函数
void CreateUIFont()
{
    // 如果字体已存在，先释放
    if (g_hFont != NULL)
    {
        DeleteObject(g_hFont);
        g_hFont = NULL;
    }
    
    // 创建更光滑的字体 - 使用微软雅黑，启用抗锯齿
    LOGFONTW lf = {0};
    lf.lfHeight = -16;  // 字体大小，负值表示字符高度
    lf.lfWeight = FW_NORMAL;  // 正常字重
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;  // 使用TrueType字体
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;  // 启用ClearType抗锯齿
    lf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;  // 无衬线字体
    
    // 尝试使用微软雅黑字体，这是Windows系统中显示效果最好的字体之一
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei UI");
    
    g_hFont = CreateFontIndirectW(&lf);
    
    // 如果创建失败，尝试使用默认的微软雅黑
    if (g_hFont == NULL)
    {
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei");
        g_hFont = CreateFontIndirectW(&lf);
    }
    
    // 如果还是失败，尝试使用Segoe UI
    if (g_hFont == NULL)
    {
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Segoe UI");
        g_hFont = CreateFontIndirectW(&lf);
    }
    
    // 最后的备选方案：使用系统默认字体
    if (g_hFont == NULL)
    {
        g_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        LogToFile("CreateUIFont: 使用系统默认字体");
    }
    else
    {
        LogToFile("CreateUIFont: 成功创建高质量字体");
    }
}

// 应用字体到控件函数
void ApplyFontToControl(HWND hWnd)
{
    if (g_hFont != NULL && hWnd != NULL)
    {
        SendMessageW(hWnd, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }
}

// 根据当前模式更新ListView列标题
void UpdateListViewColumns()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return;
    }
    
    LVCOLUMNW lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    
    if (g_calculatorMode)
    {
        // 计算模式：表达式 | 结果
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"表达式";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 0, &lvc);
        
        lvc.iSubItem = 1;
        lvc.pszText = (WCHAR*)L"结果";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 1, &lvc);
        
        LogToFile("UpdateListViewColumns: 已更新为计算模式列标题（表达式 | 结果）");
    }
    else if (g_bookmarkMode)
    {
        // 网址收藏模式：名称 | URL
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"名称";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 0, &lvc);
        
        lvc.iSubItem = 1;
        lvc.pszText = (WCHAR*)L"URL";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 1, &lvc);
        
        LogToFile("UpdateListViewColumns: 已更新为网址收藏模式列标题（名称 | URL）");
    }
    else
    {
        // 普通模式：名称 | 路径
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"名称";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 0, &lvc);
        
        // 确保第二列存在
        int columnCount = Header_GetItemCount(ListView_GetHeader(g_hListView));
        if (columnCount < 2)
        {
            // 如果第二列不存在，创建它
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 180;
            ListView_InsertColumn(g_hListView, 1, &lvc);
        }
        else
        {
            // 如果第二列已存在，更新它
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 180;
            ListView_SetColumn(g_hListView, 1, &lvc);
        }
        
        LogToFile("UpdateListViewColumns: 已更新为普通模式列标题（名称 | 路径）");
    }
}

// 在ListView第一行添加提示信息（单行）
void AddHintRowToListView(const WCHAR* hintText)
{
    if (!g_hListView || !IsWindow(g_hListView) || !hintText)
    {
        return;
    }
    
    // 检查是否已经有提示行（第一行）
    int itemCount = ListView_GetItemCount(g_hListView);
    if (itemCount > 0)
    {
        // 检查第一行是否是提示行（通过检查文本是否包含提示标识）
        WCHAR firstItemText[1024] = {0};
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = firstItemText;
        lvi.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvi))
        {
            // 如果第一行已经是提示行，更新它
            if (wcsstr(firstItemText, L"提示:") == firstItemText || wcsstr(firstItemText, L"💡") == firstItemText)
            {
                lvi.pszText = const_cast<LPWSTR>(hintText);
                ListView_SetItem(g_hListView, &lvi);
                return;
            }
        }
    }
    
    // 插入新的提示行到第一行
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;  // 插入到第一行
    lvi.iSubItem = 0;
    lvi.pszText = const_cast<LPWSTR>(hintText);
    ListView_InsertItem(g_hListView, &lvi);
    
    // 如果是多列模式，设置第二列为空
    int columnCount = Header_GetItemCount(ListView_GetHeader(g_hListView));
    if (columnCount > 1)
    {
        lvi.iSubItem = 1;
        lvi.pszText = (WCHAR*)L"";
        ListView_SetItem(g_hListView, &lvi);
    }
}

// 在ListView前面添加多行提示信息
void AddMultiLineHintsToListView(const WCHAR* hints[], int hintCount)
{
    if (!g_hListView || !IsWindow(g_hListView) || !hints || hintCount <= 0)
    {
        return;
    }
    
    // 检查是否已经有提示行
    int itemCount = ListView_GetItemCount(g_hListView);
    bool hasHints = false;
    if (itemCount > 0)
    {
        WCHAR firstItemText[1024] = {0};
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = firstItemText;
        lvi.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvi))
        {
            if (wcsstr(firstItemText, L"提示:") == firstItemText || wcsstr(firstItemText, L"💡") == firstItemText)
            {
                hasHints = true;
            }
        }
    }
    
    // 如果已有提示行，删除所有提示行
    if (hasHints)
    {
        // 删除所有提示行（从后往前删除，避免索引变化）
        for (int i = itemCount - 1; i >= 0; i--)
        {
            WCHAR itemText[1024] = {0};
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = itemText;
            lvi.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
            if (ListView_GetItem(g_hListView, &lvi))
            {
                if (wcsstr(itemText, L"提示:") == itemText || wcsstr(itemText, L"💡") == itemText)
                {
                    ListView_DeleteItem(g_hListView, i);
                }
                else
                {
                    break;  // 遇到非提示行，停止删除
                }
            }
        }
    }
    
    // 插入新的提示行到前面
    for (int i = 0; i < hintCount; i++)
    {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;  // 插入到第i行
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(hints[i]);
        ListView_InsertItem(g_hListView, &lvi);
    }
}

// 获取ListView前面提示行的数量
int GetHintRowCount()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return 0;
    }
    
    int hintRowCount = 0;
    int itemCount = ListView_GetItemCount(g_hListView);
    for (int i = 0; i < itemCount; i++)
    {
        WCHAR itemText[1024] = {0};
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = itemText;
        lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvItem))
        {
            // 检查是否是提示行
            if (wcsstr(itemText, L"提示:") == itemText || wcsstr(itemText, L"💡") == itemText)
            {
                hintRowCount++;
            }
            else
            {
                break;  // 遇到非提示行，停止计数
            }
        }
    }
    
    return hintRowCount;
}

// 获取第一个实际项目（跳过提示行）的索引
INT_PTR GetFirstActualItemIndex()
{
    int hintRowCount = GetHintRowCount();
    int itemCount = ListView_GetItemCount(g_hListView);
    
    if (hintRowCount >= itemCount)
    {
        return -1;  // 只有提示行，没有实际项目
    }
    
    return hintRowCount;  // 第一个实际项目的索引
}

// 窗口大小调整时重新布局控件
void LayoutControls(int windowWidth, int windowHeight)
{
    if (windowWidth <= 0 || windowHeight <= 0)
    {
        return;
    }
    
    char layoutLog[200] = {0};
    sprintf(layoutLog, "LayoutControls: 开始重新布局控件，窗口大小 %dx%d", windowWidth, windowHeight);
    LogToFile(layoutLog);
    
    // 边距
    int margin = 10;
    int spacing = 10;
    
    // 控件高度
    int editHeight = 25;
    int buttonHeight = 25;
    
    // 计算各控件的位置和大小
    // 编辑框：上方位置，宽度占满窗口
    int editX = margin;
    int editY = margin;
    int editWidth = windowWidth - (margin * 2);
    
    // ListView：文本框下方，占据中间大部分空间
    int listViewX = margin;
    int listViewY = editY + editHeight + spacing;
    int listViewWidth = windowWidth - (margin * 2);
    int listViewHeight = windowHeight - listViewY - margin - buttonHeight - margin; // 为底部按钮和边距留空间
    
    // 按钮区域：底部位置
    int buttonY = windowHeight - margin - buttonHeight;
    int buttonWidth = 80;
    int buttonSpacing = 10;
    
    // 按钮位置：设置按钮在左侧，退出按钮在右侧
    int settingsButtonX = margin;
    int exitButtonX = windowWidth - margin - buttonWidth;
    
    // 应用新的位置和大小到各个控件
    SetWindowPos(g_hEdit, NULL, editX, editY, editWidth, editHeight, SWP_NOZORDER);
    SetWindowPos(g_hListView, NULL, listViewX, listViewY, listViewWidth, listViewHeight, SWP_NOZORDER);
    
    // 更新 WebView2 占位窗口的位置和大小（与 ListView 相同）
    if (g_hWebView2 != NULL)
    {
        SetWindowPos(g_hWebView2, NULL, listViewX, listViewY, listViewWidth, listViewHeight, SWP_NOZORDER);
        
        // 更新 WebView2 控制器的位置和大小
        if (g_webViewController != NULL)
        {
            RECT bounds = {0, 0, listViewWidth, listViewHeight};
            g_webViewController->put_Bounds(bounds);
        }
    }
    
    SetWindowPos(g_hSettingsButton, NULL, settingsButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(g_hExitCalcButton, NULL, exitButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    SetWindowPos(g_hExitBookmarkButton, NULL, exitButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    
    // 计算模式菜单按钮位置：在退出按钮左侧
    int calcMenuButtonX = exitButtonX - buttonWidth - buttonSpacing;
    SetWindowPos(g_hCalcMenuButton, NULL, calcMenuButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    
    // 刷新ListView显示
    if (g_hListView != NULL)
    {
        InvalidateRect(g_hListView, NULL, TRUE);
    }
    
    sprintf(layoutLog, "LayoutControls: 控件布局完成");
    LogToFile(layoutLog);
}

// Show launcher window
void ShowLauncherWindow()
{
    LogToFile("ShowLauncherWindow called - START");
    
    // 设置初始化标志，防止自动执行
    g_windowInitializing = true;
    
    // Clear edit control first before showing to avoid unexpected change notifications
    SetWindowTextW(g_hEdit, L"");
    LogToFile("Cleared search box before showing window");
    
    ShowWindow(g_hMainWindow, SW_SHOW);
    UpdateWindow(g_hMainWindow);
    SetForegroundWindow(g_hMainWindow);
    LogToFile("Window shown and brought to foreground");
    
    // 设置英文输入法
    SetEnglishInputMethod();
    
    // Set focus to edit control
    LogToFile("Setting focus to edit control");
    SetFocus(g_hEdit);
    
    // Display default search results
    LogToFile("Displaying default search results");
    SearchAndDisplayResults(L"");
    
    // 处理所有待处理的消息，确保初始化消息都已处理
    MSG msg;
    int messageCount = 0;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) && messageCount < 50)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        messageCount++;
    }
    
    // 延迟清除初始化标志，确保所有初始化消息都已处理
    Sleep(200);
    g_windowInitializing = false;
    
    LogToFile("ShowLauncherWindow finished - END");
}

// Hide launcher window
void HideLauncherWindow()
{
    ShowWindow(g_hMainWindow, SW_HIDE);
}

// 设置英文输入法
void SetEnglishInputMethod()
{
    // 获取当前线程的输入法上下文
    HIMC hIMC = ImmGetContext(g_hMainWindow);
    if (hIMC)
    {
        // 尝试切换到英文输入法
        // 使用0x0409表示英语(美国)的键盘布局
        HKL hKL = GetKeyboardLayout(0);
        if (LOWORD(hKL) != 0x0409)  // 如果不是英文输入法
        {
            // 尝试加载英文键盘布局
            HKL hUSLayout = LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE);
            if (hUSLayout)
            {
                ActivateKeyboardLayout(hUSLayout, KLF_SETFORPROCESS);
                LogToFile("SetEnglishInputMethod: 成功切换到英文输入法");
            }
            else
            {
                LogToFile("SetEnglishInputMethod: 加载英文键盘布局失败");
            }
        }
        else
        {
            LogToFile("SetEnglishInputMethod: 已经是英文输入法");
        }
        
        // 释放输入法上下文
        ImmReleaseContext(g_hMainWindow, hIMC);
    }
    else
    {
        LogToFile("SetEnglishInputMethod: 获取输入法上下文失败");
    }
}

// 添加系统托盘图标
void AddTrayIcon()
{
    // 如果已经添加了托盘图标，先移除
    if (g_trayIconAdded)
    {
        RemoveTrayIcon();
    }
    
    // 设置托盘图标数据
    g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    g_notifyIconData.hWnd = g_hMainWindow;
    g_notifyIconData.uID = 1;
    g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;
    // 从文件加载自定义图标
    g_notifyIconData.hIcon = (HICON)LoadImageW(
        NULL, 
        L"app_icon.ico", 
        IMAGE_ICON, 
        0, 
        0, 
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    );
    
    // 如果加载自定义图标失败，使用系统默认图标作为备选
    if (!g_notifyIconData.hIcon) {
        g_notifyIconData.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        LogToFile("AddTrayIcon: 加载自定义图标失败，使用系统默认图标");
    }
    
    // 使用memcpy来复制字符串，避免类型问题
    const wchar_t* tipText = L"Quick Launcher";
    memcpy(g_notifyIconData.szTip, tipText, (wcslen(tipText) + 1) * sizeof(wchar_t));
    
    // 添加托盘图标
    if (Shell_NotifyIcon(NIM_ADD, &g_notifyIconData))
    {
        g_trayIconAdded = true;
        LogToFile("AddTrayIcon: 成功添加系统托盘图标");
    }
    else
    {
        LogToFile("AddTrayIcon: 添加系统托盘图标失败");
    }
}

// 移除系统托盘图标
void RemoveTrayIcon()
{
    if (g_trayIconAdded)
    {
        if (Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData))
        {
            g_trayIconAdded = false;
            LogToFile("RemoveTrayIcon: 成功移除系统托盘图标");
        }
        else
        {
            LogToFile("RemoveTrayIcon: 移除系统托盘图标失败");
        }
    }
}

// 创建托盘右键菜单
void CreateTrayMenu()
{
    // 如果菜单已存在，先销毁
    if (g_trayMenu)
    {
        DestroyMenu(g_trayMenu);
    }
    
    // 创建弹出菜单
    g_trayMenu = CreatePopupMenu();
    if (g_trayMenu)
    {
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_SHOW, L"显示窗口");
        AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_EXIT, L"退出");
        LogToFile("CreateTrayMenu: 成功创建托盘右键菜单");
    }
    else
    {
        LogToFile("CreateTrayMenu: 创建托盘右键菜单失败");
    }
}

// 处理托盘消息
void HandleTrayMessage(LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
        // 双击左键显示窗口
        ShowLauncherWindow();
        LogToFile("HandleTrayMessage: 双击托盘图标，显示窗口");
        break;
        
    case WM_RBUTTONDOWN:
        // 右键点击显示菜单
        if (g_trayMenu)
        {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(g_hMainWindow);
            
            // 显示菜单并获取用户选择
            UINT cmd = TrackPopupMenu(g_trayMenu, 
                                     TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                     pt.x, pt.y, 0, g_hMainWindow, NULL);
            
            // 处理菜单选择
            switch (cmd)
            {
            case ID_TRAY_SHOW:
                ShowLauncherWindow();
                LogToFile("HandleTrayMessage: 用户选择显示窗口");
                break;
                
            case ID_TRAY_EXIT:
                PostMessage(g_hMainWindow, WM_CLOSE, 0, 0);
                LogToFile("HandleTrayMessage: 用户选择退出");
                break;
            }
        }
        break;
    }
}

// Process command
void ProcessCommand(const WCHAR* command)
{
    // 记录要处理的命令
    char commandLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, command, -1, commandLog, sizeof(commandLog), NULL, NULL);
    char logMsg[1100] = {0};
    sprintf(logMsg, "ProcessCommand: 处理命令 '%s'", commandLog);
    LogToFile(logMsg);
    
    // 检查是否在计算模式下输入"q"退出
    if (g_calculatorMode && wcscmp(command, L"q") == 0)
    {
        LogToFile("ProcessCommand: 在计算模式下输入'q'，退出计算模式");
        ExitCalculatorMode();
        return;
    }
    
    // 检查是否在网址收藏模式下输入"q"退出
    if (g_bookmarkMode && wcscmp(command, L"q") == 0)
    {
        LogToFile("ProcessCommand: 在网址收藏模式下输入'q'，退出网址收藏模式");
        ExitBookmarkMode();
        return;
    }
    
    // 检查是否在目录浏览模式下输入"q"退出
    if (g_dirMode && wcscmp(command, L"q") == 0)
    {
        LogToFile("ProcessCommand: 在目录浏览模式下输入'q'，退出目录浏览模式");
        ExitDirMode();
        return;
    }
    
    // 检查是否是"js"命令，用于进入计算模式
    if (wcscmp(command, L"js") == 0)
    {
        LogToFile("ProcessCommand: 识别为'js'命令，进入计算模式");
        EnterCalculatorMode();
        return;
    }
    
    // 检查是否是"wz"命令，用于进入网址收藏模式
    if (wcscmp(command, L"wz") == 0)
    {
        LogToFile("ProcessCommand: 识别为'wz'命令，进入网址收藏模式");
        EnterBookmarkMode();
        return;
    }
    
    // 检查是否是"dir"命令，用于进入目录浏览模式
    if (wcscmp(command, L"dir") == 0)
    {
        LogToFile("ProcessCommand: 识别为'dir'命令，进入目录浏览模式");
        EnterDirMode();
        return;
    }
    
    // 检查是否是"set"命令，显示设置菜单
    if (wcscmp(command, L"set") == 0)
    {
        LogToFile("ProcessCommand: 识别为'set'命令，显示设置菜单");
        ShowSettingsMenu();
        return;
    }
    
    // 检查是否是"help"命令，显示帮助信息
    if (wcscmp(command, L"help") == 0)
    {
        LogToFile("ProcessCommand: 识别为'help'命令，显示帮助信息");
        ShowHelpInfo();
        return;
    }
    
    // Clear previous results
    ListView_DeleteAllItems(g_hListView);
    
    // Check if command is a URL
    if (wcsstr(command, L"://") != NULL || wcsstr(command, L"www.") != NULL)
    {
        LogToFile("ProcessCommand: 识别为URL");
        WCHAR fullUrl[1024] = {0};
        WCHAR feedback[1024] = {0};
        
        // Add http:// prefix if missing
        if (wcsstr(command, L"://") == NULL)
        {
            wsprintfW(fullUrl, L"http://%s", command);
            wsprintfW(feedback, L"Add http:// prefix...");
            
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = feedback;
            ListView_InsertItem(g_hListView, &lvi);
            
            LogToFile("ProcessCommand: 添加http://前缀");
        }
        else
        {
            wcscpy(fullUrl, command);
            LogToFile("ProcessCommand: 使用原始URL");
        }
        
        // Show URL being opened
        wsprintfW(feedback, L"Opening URL: %s", fullUrl);
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = feedback;
        ListView_InsertItem(g_hListView, &lvi);
        
        // Open URL with default browser
        HINSTANCE result = ShellExecuteW(NULL, L"open", fullUrl, NULL, NULL, SW_SHOWNORMAL);
        
        // Log the URL opening attempt
        char urlLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, fullUrl, -1, urlLog, sizeof(urlLog), NULL, NULL);
        char finalLog[1024] = {0};
        sprintf(finalLog, "ProcessCommand: 打开URL '%s', ShellExecuteW返回值: %Id", urlLog, (INT_PTR)result);
        LogToFile(finalLog);
        
        if ((INT_PTR)result > 32)
        {
            wsprintfW(feedback, L"Success! URL opened in default browser");
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = feedback;
            ListView_InsertItem(g_hListView, &lvi);
            LogToFile("ProcessCommand: URL打开成功");
        }
        else
        {
            wsprintfW(feedback, L"Failed to open URL: error code %Id", (INT_PTR)result);
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = feedback;
            ListView_InsertItem(g_hListView, &lvi);
            sprintf(finalLog, "ProcessCommand: URL打开失败，错误代码 %Id", (INT_PTR)result);
            LogToFile(finalLog);
        }
    }
    // Handle file paths
    else if (GetFileAttributesW(command) != INVALID_FILE_ATTRIBUTES)
    {
        LogToFile("ProcessCommand: 识别为文件路径");
        WCHAR feedback[1024] = {0};
        wsprintfW(feedback, L"Opening file: %s", command);
        
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = feedback;
        ListView_InsertItem(g_hListView, &lvi);
        
        HINSTANCE result = ShellExecuteW(NULL, L"open", command, NULL, NULL, SW_SHOWNORMAL);
        
        sprintf(logMsg, "ProcessCommand: 打开文件 '%s', ShellExecuteW返回值: %Id", commandLog, (INT_PTR)result);
        LogToFile(logMsg);
        
        if ((INT_PTR)result > 32)
        {
            wsprintfW(feedback, L"File opened successfully");
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = feedback;
            ListView_InsertItem(g_hListView, &lvi);
            LogToFile("ProcessCommand: 文件打开成功");
        }
        else
        {
            wsprintfW(feedback, L"Failed to open file: error code %Id", (INT_PTR)result);
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = feedback;
            ListView_InsertItem(g_hListView, &lvi);
            sprintf(logMsg, "ProcessCommand: 文件打开失败，错误代码 %Id", (INT_PTR)result);
            LogToFile(logMsg);
        }
    }
    else
    {
        LogToFile("ProcessCommand: 不是URL也不是有效文件路径，尝试在快捷方式中查找");
        // Try to find in common directories
        bool found = false;
        for (size_t i = 0; i < g_shortcuts.size(); i++)
        {
            if (_wcsicmp(g_shortcuts[i].name, command) == 0)
            {
                sprintf(logMsg, "ProcessCommand: 在快捷方式中找到匹配项 '%s'，索引 %zu", commandLog, i);
                LogToFile(logMsg);
                ExecuteSelectedItem(i);
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            sprintf(logMsg, "ProcessCommand: 未找到匹配的命令或文件 '%s'", commandLog);
            LogToFile(logMsg);
            
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<LPWSTR>(L"No matching command or file found");
            ListView_InsertItem(g_hListView, &lvi);
        }
    }
}

// Add all desktop shortcuts to the list
void AddDesktopShortcuts()
{
    WCHAR desktopPath[MAX_PATH] = {0};
    
    // Get desktop path
    if (SHGetSpecialFolderPathW(NULL, desktopPath, CSIDL_DESKTOP, FALSE))
    {
        WCHAR searchPath[MAX_PATH] = {0};
        wsprintfW(searchPath, L"%s\\*.lnk", desktopPath);
        
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath, &findData);
        
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                // Skip . and .. directories
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    ShortcutItem shortcut = {0};
                    
                    // Extract name without extension
                    WCHAR* dotPos = wcsrchr(findData.cFileName, L'.');
                    if (dotPos)
                    {
                        *dotPos = L'\0';  // Remove .lnk extension
                    }
                    
                    // Set shortcut name and path
                    wcscpy(shortcut.name, findData.cFileName);
                    wsprintfW(shortcut.path, L"%s\\%s.lnk", desktopPath, findData.cFileName);
                    shortcut.type = 2;  // Mark as application
                    shortcut.usageCount = 0;
                    
                    g_shortcuts.push_back(shortcut);
                }
            } while (FindNextFileW(hFind, &findData));
            
            FindClose(hFind);
        }
    }
}

// Initialize common shortcuts
void InitializeCommonShortcuts()
{
    g_shortcuts.clear();
    
    // Add desktop shortcuts first
    AddDesktopShortcuts();
    
    // Desktop folder
    ShortcutItem desktop = {0};
    wcscpy(desktop.name, L"Desktop");
    desktop.type = 0;
    desktop.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, desktop.path, CSIDL_DESKTOP, FALSE);
    g_shortcuts.push_back(desktop);
    
    // Show Desktop
    ShortcutItem showDesktop = {0};
    wcscpy(showDesktop.name, L"Show Desktop");
    wcscpy(showDesktop.path, L"explorer.exe shell:::{3080F90D-D7AD-11D9-BD98-0000947B0257}");
    showDesktop.type = 2;
    showDesktop.usageCount = 0;
    g_shortcuts.push_back(showDesktop);
    
    // Start Menu Programs
    ShortcutItem startMenu = {0};
    wcscpy(startMenu.name, L"Start Menu");
    startMenu.type = 0;
    startMenu.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, startMenu.path, CSIDL_PROGRAMS, FALSE);
    g_shortcuts.push_back(startMenu);
    
    // Downloads folder
    ShortcutItem downloads = {0};
    wcscpy(downloads.name, L"Downloads");
    downloads.type = 0;
    downloads.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, downloads.path, CSIDL_MYDOCUMENTS, FALSE);
    wcscat(downloads.path, L"\\Downloads");
    g_shortcuts.push_back(downloads);
    
    // Documents folder
    ShortcutItem documents = {0};
    wcscpy(documents.name, L"Documents");
    documents.type = 0;
    documents.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, documents.path, CSIDL_MYDOCUMENTS, FALSE);
    g_shortcuts.push_back(documents);
    
    // Google URL
    ShortcutItem google = {0};
    wcscpy(google.name, L"Google");
    wcscpy(google.path, L"https://www.google.com");
    google.type = 1;
    google.usageCount = 0;
    g_shortcuts.push_back(google);
    
    // Baidu URL
    ShortcutItem baidu = {0};
    wcscpy(baidu.name, L"Baidu");
    wcscpy(baidu.path, L"https://www.baidu.com");
    baidu.type = 1;
    baidu.usageCount = 0;
    g_shortcuts.push_back(baidu);
    
    // File Explorer
    ShortcutItem explorer = {0};
    wcscpy(explorer.name, L"Explorer");
    wcscpy(explorer.path, L"explorer.exe");
    explorer.type = 2;
    explorer.usageCount = 0;
    g_shortcuts.push_back(explorer);
    
    // Notepad
    ShortcutItem notepad = {0};
    wcscpy(notepad.name, L"Notepad");
    wcscpy(notepad.path, L"notepad.exe");
    notepad.type = 2;
    notepad.usageCount = 0;
    
    // Check if Notepad is already in the list (might be from desktop shortcuts)
    bool alreadyExists = false;
    for (size_t i = 0; i < g_shortcuts.size(); i++)
    {
        if (_wcsicmp(g_shortcuts[i].name, L"Notepad") == 0)
        {
            alreadyExists = true;
            break;
        }
    }
    
    if (!alreadyExists)
    {
        g_shortcuts.push_back(notepad);
    }
}

// Search and display matching results
void SearchAndDisplayResults(const WCHAR* query)
{
    // 检查ListView是否有效
    if (!g_hListView || !IsWindow(g_hListView))
    {
        LogToFile("SearchAndDisplayResults: ListView句柄无效或窗口不存在");
        return;
    }
    
    // 进入搜索结果模式，退出设置菜单状态
    g_settingsMenuMode = false;
    std::vector<std::wstring> webViewHints;
    
    // 检查ListView是否有列（如果没有列，需要先初始化列）
    // 使用Header_GetItemCount来检查列数
    HWND hHeader = ListView_GetHeader(g_hListView);
    int columnCount = 0;
    if (hHeader)
    {
        columnCount = Header_GetItemCount(hHeader);
    }
    if (columnCount == 0)
    {
        LogToFile("SearchAndDisplayResults: ListView没有列，初始化列");
        // 初始化ListView的列
        LVCOLUMNW lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        
        // 第一列：名称
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"名称";
        lvc.cx = 180;
        ListView_InsertColumn(g_hListView, 0, &lvc);
        
        // 第二列：路径
        lvc.iSubItem = 1;
        lvc.pszText = (WCHAR*)L"路径";
        lvc.cx = 180;
        ListView_InsertColumn(g_hListView, 1, &lvc);
        
        LogToFile("SearchAndDisplayResults: ListView列初始化完成（名称 | 路径）");
    }
    
    // 记录搜索查询
    char queryLog[1024] = {0};
    if (query)
    {
        WideCharToMultiByte(CP_UTF8, 0, query, -1, queryLog, sizeof(queryLog), NULL, NULL);
    }
    else
    {
        strcpy(queryLog, "(null)");
    }
    
    char logMsg[1100] = {0};
    sprintf(logMsg, "SearchAndDisplayResults: 搜索查询 '%s'", queryLog);
    LogToFile(logMsg);
    
    // 注意：不再在SearchAndDisplayResults中处理"js"命令
    // "js"命令现在只在用户按回车键时在EN_RETURN消息中处理
    
    // 注意：不再在SearchAndDisplayResults中处理"wz"命令
    // "wz"命令现在只在用户按回车键时在EN_RETURN消息中处理
    
    // 清空旧内容并添加提示行
    ListView_DeleteAllItems(g_hListView);
    g_searchResults.clear();
    
    if (g_calculatorMode)
    {
        // 计算模式：添加多行提示
        const WCHAR* hints[] = {
            L"💡 输入数学表达式",
            L"💡 按回车计算",
            L"💡 输入 q 退出计算模式"
        };
        AddMultiLineHintsToListView(hints, 3);
        for (int i = 0; i < 3; ++i)
        {
            webViewHints.emplace_back(hints[i]);
        }
    }
    else if (g_bookmarkMode)
    {
        // 网址收藏模式：添加多行提示
        const WCHAR* hints[] = {
            L"💡 搜索或浏览收藏的网址",
            L"💡 按回车或双击打开",
            L"💡 输入 q 退出网址收藏模式"
        };
        AddMultiLineHintsToListView(hints, 3);
        for (int i = 0; i < 3; ++i)
        {
            webViewHints.emplace_back(hints[i]);
        }
    }
    else
    {
        // 普通模式：添加多行提示
        const WCHAR* hints[] = {
            L"💡 输入命令或网址搜索",
            L"💡 按回车或双击执行",
            L"💡 输入 js 进入计算模式，输入 wz 进入网址收藏模式",
            L"💡 输入 set 进入设置模式，输入 dir 进入目录管理模式"
        };
        AddMultiLineHintsToListView(hints, 4);
        for (int i = 0; i < 4; ++i)
        {
            webViewHints.emplace_back(hints[i]);
        }
    }
    
    if (!query || wcslen(query) == 0)
    {
        LogToFile("SearchAndDisplayResults: 查询为空，显示最常用的项目");
        // If query is empty, show most frequently used items
        std::vector<ShortcutItem> sorted = g_shortcuts;
        std::sort(sorted.begin(), sorted.end(), 
            [](const ShortcutItem& a, const ShortcutItem& b) { 
                return a.usageCount > b.usageCount; 
            });
        
        g_searchResults = sorted;
        
        sprintf(logMsg, "SearchAndDisplayResults: 显示 %zu 个最常用项目", sorted.size());
        LogToFile(logMsg);
        
        for (size_t i = 0; i < sorted.size(); i++)
        {
            WCHAR display[1024] = {0};
            if (sorted[i].type == 0) // Directory
                wsprintfW(display, L"DIR: %s", sorted[i].name);
            else if (sorted[i].type == 1) // URL
                wsprintfW(display, L"URL: %s", sorted[i].name);
            else // Application
                wsprintfW(display, L"APP: %s", sorted[i].name);
            
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = ListView_GetItemCount(g_hListView);
            lvi.iSubItem = 0;
            lvi.pszText = display;
            ListView_InsertItem(g_hListView, &lvi);
            
            // 记录添加到列表的项目
            char itemNameLog[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, sorted[i].name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
            sprintf(logMsg, "SearchAndDisplayResults: 添加项目 '%s' (类型: %d, 使用次数: %d)", 
                    itemNameLog, sorted[i].type, sorted[i].usageCount);
            LogToFile(logMsg);
        }
        
        // 更新 WebView2 显示搜索结果
        std::wstring html;
        CreateWebView2HTML(g_searchResults, webViewHints, html);
        UpdateWebView2Content(html.c_str());
        
        return;
    }
    
    sprintf(logMsg, "SearchAndDisplayResults: 在 %zu 个快捷方式中搜索匹配项", g_shortcuts.size());
    LogToFile(logMsg);
    
    // 首先搜索收藏的网址
    sprintf(logMsg, "SearchAndDisplayResults: 在 %zu 个收藏网址中搜索匹配项", g_bookmarks.size());
    LogToFile(logMsg);
    
    // 搜索收藏的网址
    for (size_t i = 0; i < g_bookmarks.size(); i++)
    {
        // 转换名称为小写以进行不区分大小写的比较
        std::wstring lowerName = g_bookmarks[i].first;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        
        std::wstring lowerUrl = g_bookmarks[i].second;
        std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::towlower);
        
        // 转换查询为小写
        std::wstring lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
        
        // 检查名称或URL是否包含查询字符串
        bool nameMatch = lowerName.find(lowerQuery) != std::wstring::npos;
        bool urlMatch = lowerUrl.find(lowerQuery) != std::wstring::npos;
        
        if (nameMatch || urlMatch)
        {
            sprintf(logMsg, "SearchAndDisplayResults: 找到收藏网址匹配 '%ls'", g_bookmarks[i].first.c_str());
            LogToFile(logMsg);
            
            // 创建一个临时的ShortcutItem来表示收藏的网址
            ShortcutItem bookmarkItem = {0};
            wcscpy(bookmarkItem.name, g_bookmarks[i].first.c_str());
            wcscpy(bookmarkItem.path, g_bookmarks[i].second.c_str());
            bookmarkItem.type = 1; // URL类型
            bookmarkItem.usageCount = 0;
            
            // 先添加到搜索结果，然后追加到ListView末尾（使用-1自动追加）
            g_searchResults.push_back(bookmarkItem);
            
            // 显示在列表视图中，使用特殊格式标识收藏网址
            WCHAR display[1024] = {0};
            wsprintfW(display, L"收藏: %s", g_bookmarks[i].first.c_str());
            
            // 先获取当前ListView的项目数量，作为插入位置
            int currentItemCount = ListView_GetItemCount(g_hListView);
            
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = currentItemCount;  // 使用当前项目数量作为插入位置
            lvi.iSubItem = 0;
            lvi.pszText = display;
            int actualIndex = ListView_InsertItem(g_hListView, &lvi);
            
            // 检查插入是否成功
            if (actualIndex == -1)
            {
                DWORD error = GetLastError();
                char errorLog[300] = {0};
                sprintf(errorLog, "SearchAndDisplayResults: 收藏网址ListView_InsertItem失败，错误代码: %lu, ListView项目数: %d", 
                        error, currentItemCount);
                LogToFile(errorLog);
            }
            else
            {
                // 添加URL到第二列
                lvi.iSubItem = 1;
                lvi.pszText = const_cast<LPWSTR>(g_bookmarks[i].second.c_str());
                ListView_SetItem(g_hListView, &lvi);
            }
        }
    }
    
    // Search for matching items using case-insensitive comparison
    for (size_t i = 0; i < g_shortcuts.size(); i++)
    {
        // 记录当前检查的项目
        char itemNameLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, g_shortcuts[i].name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
        
        // Check for exact match (already case-insensitive)
        if (_wcsicmp(g_shortcuts[i].name, query) == 0)
        {
            sprintf(logMsg, "SearchAndDisplayResults: 找到精确匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
            
            WCHAR display[1024] = {0};
            if (g_shortcuts[i].type == 0) // Directory
                wsprintfW(display, L"DIR: %s", g_shortcuts[i].name);
            else if (g_shortcuts[i].type == 1) // URL
                wsprintfW(display, L"URL: %s", g_shortcuts[i].name);
            else // Application
                wsprintfW(display, L"APP: %s", g_shortcuts[i].name);
            
            // 先添加到搜索结果，然后追加到ListView末尾
            g_searchResults.push_back(g_shortcuts[i]);
            size_t searchResultIndex = g_searchResults.size() - 1;
            
            // 先获取当前ListView的项目数量，作为插入位置
            int currentItemCount = ListView_GetItemCount(g_hListView);
            
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = currentItemCount;  // 使用当前项目数量作为插入位置
            lvi.iSubItem = 0;
            lvi.pszText = display;
            int actualIndex = ListView_InsertItem(g_hListView, &lvi);
            
            // 检查插入是否成功
            if (actualIndex == -1)
            {
                DWORD error = GetLastError();
                char errorLog[300] = {0};
                sprintf(errorLog, "SearchAndDisplayResults: 精确匹配ListView_InsertItem失败，错误代码: %lu, ListView项目数: %d, 句柄: %p", 
                        error, currentItemCount, g_hListView);
                LogToFile(errorLog);
            }
            else
            {
                // 记录插入位置用于调试
                char insertLog[300] = {0};
                sprintf(insertLog, "SearchAndDisplayResults: 精确匹配插入到ListView位置 %d, g_searchResults索引 %zu", 
                        actualIndex, searchResultIndex);
                LogToFile(insertLog);
                
                // 添加路径到第二列
                lvi.iSubItem = 1;
                lvi.pszText = g_shortcuts[i].path;
                ListView_SetItem(g_hListView, &lvi);
            }
        }
        else
        {
            // Check for case-insensitive substring match
            size_t queryLen = wcslen(query);
            size_t nameLen = wcslen(g_shortcuts[i].name);
            
            // First check exact match (already done above)
            // Then check for case-insensitive substring match
            if (_wcsnicmp(g_shortcuts[i].name, query, queryLen) == 0)
            {
                sprintf(logMsg, "SearchAndDisplayResults: 找到前缀匹配 '%s'", itemNameLog);
                LogToFile(logMsg);
                
                WCHAR display[1024] = {0};
                if (g_shortcuts[i].type == 0) // Directory
                    wsprintfW(display, L"DIR: %s", g_shortcuts[i].name);
                else if (g_shortcuts[i].type == 1) // URL
                    wsprintfW(display, L"URL: %s", g_shortcuts[i].name);
                else // Application
                    wsprintfW(display, L"APP: %s", g_shortcuts[i].name);
                
                // 先添加到搜索结果，然后追加到ListView末尾
                g_searchResults.push_back(g_shortcuts[i]);
                size_t searchResultIndex = g_searchResults.size() - 1;
                
                // 先获取当前ListView的项目数量，作为插入位置
                int currentItemCount = ListView_GetItemCount(g_hListView);
                
                LVITEMW lvi = {0};
                lvi.mask = LVIF_TEXT;
                lvi.iItem = currentItemCount;  // 使用当前项目数量作为插入位置
                lvi.iSubItem = 0;
                lvi.pszText = display;
                int actualIndex = ListView_InsertItem(g_hListView, &lvi);
                
                // 检查插入是否成功
                if (actualIndex == -1)
                {
                    DWORD error = GetLastError();
                    char errorLog[300] = {0};
                    sprintf(errorLog, "SearchAndDisplayResults: 前缀匹配ListView_InsertItem失败，错误代码: %lu, ListView项目数: %d", 
                            error, currentItemCount);
                    LogToFile(errorLog);
                }
                else
                {
                    // 记录插入位置用于调试
                    char insertLog[300] = {0};
                    sprintf(insertLog, "SearchAndDisplayResults: 前缀匹配插入到ListView位置 %d, g_searchResults索引 %zu, 名称: '%s'", 
                            actualIndex, searchResultIndex, itemNameLog);
                    LogToFile(insertLog);
                    
                    // 添加路径到第二列
                    lvi.iSubItem = 1;
                    lvi.pszText = g_shortcuts[i].path;
                    ListView_SetItem(g_hListView, &lvi);
                }
            }
            else if (queryLen <= nameLen)
            {
                // Also check for substring match anywhere in the name
                for (size_t j = 0; j <= nameLen - queryLen; j++)
                {
                    if (_wcsnicmp(&g_shortcuts[i].name[j], query, queryLen) == 0)
                    {
                        sprintf(logMsg, "SearchAndDisplayResults: 找到子字符串匹配 '%s'", itemNameLog);
                        LogToFile(logMsg);
                        
                        WCHAR display[1024] = {0};
                        if (g_shortcuts[i].type == 0) // Directory
                            wsprintfW(display, L"DIR: %s", g_shortcuts[i].name);
                        else if (g_shortcuts[i].type == 1) // URL
                            wsprintfW(display, L"URL: %s", g_shortcuts[i].name);
                        else // Application
                            wsprintfW(display, L"APP: %s", g_shortcuts[i].name);
                        
                        // 先添加到搜索结果，然后追加到ListView末尾
                        g_searchResults.push_back(g_shortcuts[i]);
                        size_t searchResultIndex = g_searchResults.size() - 1;
                        
                        // 先获取当前ListView的项目数量，作为插入位置
                        int currentItemCount = ListView_GetItemCount(g_hListView);
                        
                        LVITEMW lvi = {0};
                        lvi.mask = LVIF_TEXT;
                        lvi.iItem = currentItemCount;  // 使用当前项目数量作为插入位置
                        lvi.iSubItem = 0;
                        lvi.pszText = display;
                        int actualIndex = ListView_InsertItem(g_hListView, &lvi);
                        
                        // 检查插入是否成功
                        if (actualIndex == -1)
                        {
                            DWORD error = GetLastError();
                            char errorLog[300] = {0};
                            sprintf(errorLog, "SearchAndDisplayResults: 子字符串匹配ListView_InsertItem失败，错误代码: %lu, ListView项目数: %d", 
                                    error, currentItemCount);
                            LogToFile(errorLog);
                        }
                        else
                        {
                            // 记录插入位置用于调试
                            char insertLog[300] = {0};
                            sprintf(insertLog, "SearchAndDisplayResults: 子字符串匹配插入到ListView位置 %d, g_searchResults索引 %zu, 名称: '%s'", 
                                    actualIndex, searchResultIndex, itemNameLog);
                            LogToFile(insertLog);
                            
                            // 添加路径到第二列
                            lvi.iSubItem = 1;
                            lvi.pszText = g_shortcuts[i].path;
                            ListView_SetItem(g_hListView, &lvi);
                        }
                        break;
                    }
                }
            }
        }
    }
    
    // If no results found
    if (g_searchResults.empty())
    {
        LogToFile("SearchAndDisplayResults: 未找到匹配项，显示'未找到匹配项'消息");
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)L"No matching items found";
        ListView_InsertItem(g_hListView, &lvi);
    }
    else
    {
        sprintf(logMsg, "SearchAndDisplayResults: 找到 %zu 个匹配项", g_searchResults.size());
        LogToFile(logMsg);
        
        // 搜索完成后立即打印ListView内容用于调试
        LogToFile("SearchAndDisplayResults: 搜索完成，打印ListView和g_searchResults内容:");
        LogListViewContents();
    }

    // 无论是否有结果，WebView2 都显示提示信息和最新列表
    std::wstring html;
    CreateWebView2HTML(g_searchResults, webViewHints, html);
    UpdateWebView2Content(html.c_str());
}

// 打印ListView所有内容到日志
void LogListViewContents()
{
    if (!g_hListView)
    {
        LogToFile("LogListViewContents: ListView句柄为空");
        return;
    }
    
    int itemCount = ListView_GetItemCount(g_hListView);
    char logMsg[200] = {0};
    sprintf(logMsg, "LogListViewContents: ListView共有 %d 个项目", itemCount);
    LogToFile(logMsg);
    
    // 打印g_searchResults的内容
    sprintf(logMsg, "LogListViewContents: g_searchResults共有 %zu 个项目", g_searchResults.size());
    LogToFile(logMsg);
    for (size_t i = 0; i < g_searchResults.size() && i < 20; i++)
    {
        char itemNameLog[256] = {0};
        char itemPathLog[512] = {0};
        WideCharToMultiByte(CP_UTF8, 0, g_searchResults[i].name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, g_searchResults[i].path, -1, itemPathLog, sizeof(itemPathLog), NULL, NULL);
        char itemLog[800] = {0};
        sprintf(itemLog, "  g_searchResults[%zu]: name='%s', path='%s', type=%d", 
                i, itemNameLog, itemPathLog, g_searchResults[i].type);
        LogToFile(itemLog);
    }
    
    // 打印ListView显示的内容
    for (int i = 0; i < itemCount && i < 20; i++)
    {
        WCHAR itemText[1024] = {0};
        WCHAR itemPath[1024] = {0};
        
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = itemText;
        lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
        int result = ListView_GetItem(g_hListView, &lvItem);
        
        char resultLog[200] = {0};
        sprintf(resultLog, "  ListView_GetItem[%d] 返回值: %d", i, result);
        LogToFile(resultLog);
        
        // 获取第二列（路径）
        lvItem.iSubItem = 1;
        lvItem.pszText = itemPath;
        lvItem.cchTextMax = sizeof(itemPath) / sizeof(WCHAR);
        ListView_GetItem(g_hListView, &lvItem);
        
        char itemTextLog[1024] = {0};
        char itemPathLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, itemText, -1, itemTextLog, sizeof(itemTextLog), NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, itemPath, -1, itemPathLog, sizeof(itemPathLog), NULL, NULL);
        
        char listViewLog[2100] = {0};
        sprintf(listViewLog, "  ListView[%d]: text='%s' (长度=%zu), path='%s'", i, itemTextLog, wcslen(itemText), itemPathLog);
        LogToFile(listViewLog);
        
        // 检查是否为空
        if (wcslen(itemText) == 0)
        {
            char emptyLog[200] = {0};
            sprintf(emptyLog, "  WARNING: ListView[%d] 文本为空！", i);
            LogToFile(emptyLog);
        }
    }
    
    // 对比显示顺序是否一致
    if (itemCount > 0 && g_searchResults.size() > 0)
    {
        WCHAR firstListViewText[1024] = {0};
        LVITEMW lvItem = {0};
        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.pszText = firstListViewText;
        lvItem.cchTextMax = sizeof(firstListViewText) / sizeof(WCHAR);
        ListView_GetItem(g_hListView, &lvItem);
        
        char firstListViewLog[1024] = {0};
        char firstSearchResultLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, firstListViewText, -1, firstListViewLog, sizeof(firstListViewLog), NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, g_searchResults[0].name, -1, firstSearchResultLog, sizeof(firstSearchResultLog), NULL, NULL);
        
        char compareLog[1300] = {0};
        sprintf(compareLog, "LogListViewContents: 对比 - ListView[0]='%s', g_searchResults[0].name='%s'", 
                firstListViewLog, firstSearchResultLog);
        LogToFile(compareLog);
        
        // 检查是否匹配（ListView可能显示"DIR: "前缀）
        bool matches = false;
        if (wcsstr(firstListViewText, g_searchResults[0].name) != NULL)
        {
            matches = true;
        }
        else if (wcsstr(firstListViewText, L"收藏:") == firstListViewText)
        {
            // 收藏网址格式
            WCHAR bookmarkName[256] = {0};
            wcscpy(bookmarkName, firstListViewText + 3); // 跳过"收藏:"
            if (wcscmp(bookmarkName, g_searchResults[0].name) == 0)
            {
                matches = true;
            }
        }
        
        char matchLog[300] = {0};
        sprintf(matchLog, "LogListViewContents: 第一个项目匹配: %s", matches ? "是" : "否");
        LogToFile(matchLog);
    }
}

// Execute selected item from list
void ExecuteSelectedItem(INT_PTR index)
{
    // 检查ListView前面有多少行提示行，需要调整索引
    int hintRowCount = 0;
    int itemCount = ListView_GetItemCount(g_hListView);
    for (int i = 0; i < itemCount; i++)
    {
        WCHAR itemText[1024] = {0};
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = itemText;
        lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvItem))
        {
            // 检查是否是提示行
            if (wcsstr(itemText, L"提示:") == itemText || wcsstr(itemText, L"💡") == itemText)
            {
                hintRowCount++;
            }
            else
            {
                break;  // 遇到非提示行，停止计数
            }
        }
    }
    
    INT_PTR adjustedIndex = index - hintRowCount;
    
    if (adjustedIndex < 0 || (size_t)adjustedIndex >= g_searchResults.size())
    {
        char logMsg[200] = {0};
        sprintf(logMsg, "ExecuteSelectedItem: 无效索引 %Id（调整后 %Id），搜索结果大小为 %zu", index, adjustedIndex, g_searchResults.size());
        LogToFile(logMsg);
        return;
    }
    
    if (hintRowCount > 0)
    {
        char adjustLog[200] = {0};
        sprintf(adjustLog, "ExecuteSelectedItem: 检测到 %d 行提示行，实际执行索引调整为 %Id", hintRowCount, adjustedIndex);
        LogToFile(adjustLog);
    }
    
    // 记录所有搜索结果用于调试
    char debugMsg[2000] = {0};
    sprintf(debugMsg, "ExecuteSelectedItem: 当前搜索结果列表（共%zu项）:", g_searchResults.size());
    LogToFile(debugMsg);
    for (size_t i = 0; i < g_searchResults.size() && i < 10; i++)
    {
        char itemNameLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, g_searchResults[i].name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
        char itemLog[300] = {0};
        sprintf(itemLog, "  [%zu] %s", i, itemNameLog);
        LogToFile(itemLog);
    }
    
    // 记录ListView显示的第一个实际项目用于对比（跳过提示行）
    if (hintRowCount < itemCount)
    {
        WCHAR firstItemText[1024] = {0};
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = hintRowCount;  // 跳过提示行
        lvItem.iSubItem = 0;
        lvItem.pszText = firstItemText;
        lvItem.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvItem))
        {
            char firstItemLog[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
            char listViewLog[1100] = {0};
            sprintf(listViewLog, "ExecuteSelectedItem: ListView显示的第一个实际项目: '%s'", firstItemLog);
            LogToFile(listViewLog);
        }
    }
        
    ShortcutItem& item = g_searchResults[(size_t)adjustedIndex]; // Use reference to update usage count
    
    // 记录要执行的项目信息
    char itemNameLog[1024] = {0};
    char itemPathLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, item.name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, item.path, -1, itemPathLog, sizeof(itemPathLog), NULL, NULL);
    
    char logMsg[1100] = {0};
    sprintf(logMsg, "ExecuteSelectedItem: 执行项目[%Id] '%s' (路径: '%s', 类型: %d, 使用次数: %d)", 
            adjustedIndex, itemNameLog, itemPathLog, item.type, item.usageCount);
    LogToFile(logMsg);
    
    // 检查是否是计算模式
    if (item.type == 3 && wcscmp(item.path, L"calculator_mode") == 0)
    {
        LogToFile("ExecuteSelectedItem: 进入计算模式");
        EnterCalculatorMode();
        return;
    }
    
    // Execute the item without clearing the list
    HINSTANCE result;
    if (item.type == 0) // Directory
    {
        LogToFile("ExecuteSelectedItem: 执行目录");
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    else if (item.type == 1) // URL
    {
        LogToFile("ExecuteSelectedItem: 执行URL");
        // 检查是否是收藏的网址（路径以http://或https://开头）
        if (wcsstr(item.path, L"http://") == item.path || wcsstr(item.path, L"https://") == item.path)
        {
            // 收藏的网址，直接打开
            result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
        }
        else
        {
            // 其他URL，添加http://前缀
            WCHAR fullUrl[1024] = {0};
            if (wcsstr(item.path, L"://") == NULL)
            {
                wsprintfW(fullUrl, L"http://%s", item.path);
                result = ShellExecuteW(NULL, L"open", fullUrl, NULL, NULL, SW_SHOWNORMAL);
            }
            else
            {
                result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
            }
        }
    }
    else // Application
    {
        LogToFile("ExecuteSelectedItem: 执行应用程序");
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    
    // 记录执行结果
    sprintf(logMsg, "ExecuteSelectedItem: ShellExecuteW 返回值: %Id", (INT_PTR)result);
    LogToFile(logMsg);
    
    // Update usage count in both search results and original shortcuts list
    item.usageCount++;
    
    // Find and update the same item in the original shortcuts list
    for (size_t i = 0; i < g_shortcuts.size(); i++)
    {
        if (_wcsicmp(g_shortcuts[i].name, item.name) == 0 && 
            _wcsicmp(g_shortcuts[i].path, item.path) == 0)
        {
            g_shortcuts[i].usageCount = item.usageCount;
            sprintf(logMsg, "ExecuteSelectedItem: 更新使用次数为 %d", item.usageCount);
            LogToFile(logMsg);
            break;
        }
    }
    
    // Only show error message if execution failed
    if ((INT_PTR)result <= 32)
    {
        sprintf(logMsg, "ExecuteSelectedItem: 执行失败，错误代码 %Id", (INT_PTR)result);
        LogToFile(logMsg);
        WCHAR feedback[1024] = {0};
        wsprintfW(feedback, L"Failed to execute: error code %Id", (INT_PTR)result);
        ListView_DeleteAllItems(g_hListView);
        
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = feedback;
        ListView_InsertItem(g_hListView, &lvi);
    }
    else
    {
        LogToFile("ExecuteSelectedItem: 执行成功");
    }
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            // Create edit control without ES_WANTRETURN to receive WM_KEYDOWN messages
            // 文本框位置调整：移除左边标签，文本框靠左显示
            g_hEdit = CreateWindowExW(
                  0,
                  WC_EDITW,
                  L"",
                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                  10, 10, 280, 25,  // 调整位置和宽度：x=10, y=10, 宽度=280
                  hwnd, (HMENU)IDC_EDIT,
                  g_hInstance, NULL);
            
            // Register hotkey for Enter key detection instead of relying on EN_RETURN
            LogToFile("Edit control created without ES_WANTRETURN style to receive WM_KEYDOWN messages");

            // Set up subclassing for the edit control to intercept WM_KEYDOWN messages
            if (SetWindowSubclass(g_hEdit, EditSubclassProc, 0, 0))
            {
                LogToFile("Edit control subclassing successful");
            }
            else
            {
                LogToFile("Edit control subclassing failed");
            }

            // 创建 WebView2 占位窗口（用于承载 WebView2 控件）
            g_hWebView2 = CreateWindowExW(
                  0,
                  L"STATIC",
                  L"",
                  WS_CHILD | WS_VISIBLE | WS_BORDER,
                  10, 45, 360, 200,  // 调整位置，利用提示信息空间
                  hwnd, NULL,
                  g_hInstance, NULL);
            
            LogToFile("WebView2 占位窗口已创建");
            
            // 初始化 WebView2（异步创建，需要时间）
            InitializeWebView2(hwnd);
            
            // 保留 ListView 用于兼容（暂时隐藏）
            g_hListView = CreateWindowExW(
                  0,
                  WC_LISTVIEWW,
                  L"",
                  WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
                  10, 45, 360, 200,
                  hwnd, (HMENU)IDC_LISTVIEW,
                  g_hInstance, NULL);
            ShowWindow(g_hListView, SW_HIDE);  // 隐藏 ListView，使用 WebView2
            
            // 初始化ListView的列（保留用于兼容）
            LVCOLUMNW lvc;
            lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            lvc.iSubItem = 0;
            lvc.pszText = (WCHAR*)L"名称";
            lvc.cx = 180;
            ListView_InsertColumn(g_hListView, 0, &lvc);
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 180;
            ListView_InsertColumn(g_hListView, 1, &lvc);
            
            // Create exit calculator mode button (initially hidden)
            g_hExitCalcButton = CreateWindowExW(
                  0,
                  L"BUTTON",
                  L"退出计算",
                  WS_CHILD | BS_PUSHBUTTON,
                  300, 10, 80, 25,
                  hwnd, (HMENU)IDC_EXIT_CALC_BUTTON,
                  g_hInstance, NULL);
            
            // 设置按钮已移除，不再创建
            
            // Create exit bookmark mode button (initially hidden)
            g_hExitBookmarkButton = CreateWindowExW(
                  0,
                  L"BUTTON",
                  L"退出",
                  WS_CHILD | BS_PUSHBUTTON,
                  300, 10, 80, 25,
                  hwnd, (HMENU)IDC_EXIT_BOOKMARK_BUTTON,
                  g_hInstance, NULL);
            
            // Create calculator mode menu button (initially hidden)
            g_hCalcMenuButton = CreateWindowExW(
                  0,
                  L"BUTTON",
                  L"操作 ▼",
                  WS_CHILD | BS_PUSHBUTTON,
                  200, 10, 80, 25,
                  hwnd, (HMENU)IDC_CALC_MENU_BUTTON,
                  g_hInstance, NULL);
            
            // Initially hide the exit calculator button and exit bookmark button
            ShowWindow(g_hExitCalcButton, SW_HIDE);
            ShowWindow(g_hExitBookmarkButton, SW_HIDE);
            ShowWindow(g_hCalcMenuButton, SW_HIDE);
            
            // 应用字体到所有控件
            if (g_hFont != NULL)
            {
                ApplyFontToControl(g_hEdit);
                ApplyFontToControl(g_hListView);
                ApplyFontToControl(g_hExitCalcButton);
                ApplyFontToControl(g_hExitBookmarkButton);
                ApplyFontToControl(g_hCalcMenuButton);
                LogToFile("字体已应用到所有控件");
            }
            else
            {
                LogToFile("警告：字体句柄为空，无法应用字体");
            }
            
            return 0;
        }
            
        case WM_HOTKEY:
            {  
                if (wParam == HOTKEY_ID) // Ctrl+Alt+Q
                {
                    // Toggle window visibility when hotkey (Ctrl+Alt+Q) is pressed
                    if (IsWindowVisible(hwnd))
                    {
                        ShowWindow(hwnd, SW_HIDE);
                    }
                    else
                    {
                        ShowLauncherWindow();
                    }
                }
                else if (wParam == HOTKEY_ID_CTRL_F1) // Ctrl+F1
                {
                    // 总是显示窗口并设置到桌面位置
                    ShowWindow(hwnd, SW_HIDE); // 先隐藏确保重新定位
                    ShowLauncherWindow();
                    
                    // 设置窗口位置到桌面中央偏下位置，确保用户可见
                    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    int windowWidth = 400;
                    int windowHeight = 300;
                    int x = (screenWidth - windowWidth) / 2;
                    int y = screenHeight - windowHeight - 100; // 距离底部100像素
                    
                    SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
                    LogToFile("Ctrl+F1 pressed: Window positioned to desktop for visibility");
                }
                else if (wParam == HOTKEY_ID_CTRL_F2) // Ctrl+F2
                {
                    // 显示窗口
                    ShowLauncherWindow();
                    LogToFile("Ctrl+F2 pressed: Window shown");
                }
                
                return 0;
            }
            
        case WM_DESTROY:
            LogToFile("WM_DESTROY: Exiting program");
            
            // 保存窗口大小和位置设置
            SaveWindowSettings();
            
            UnregisterHotKey(hwnd, HOTKEY_ID);
            UnregisterHotKey(hwnd, HOTKEY_ID_CTRL_F1);
            UnregisterHotKey(hwnd, HOTKEY_ID_CTRL_F2);
            
            // 移除托盘图标和销毁托盘菜单
            RemoveTrayIcon();
            if (g_trayMenu)
            {
                DestroyMenu(g_trayMenu);
                g_trayMenu = NULL;
            }
            
            g_shortcuts.clear();
            
            // 记录退出信息并关闭日志文件
            LogToFile("WM_DESTROY: Program exiting, closing log file");
            CloseLogFile();
            
            PostQuitMessage(0);
            return 0;
            
        case WM_TIMER:
            // Timer is no longer needed since we're handling Enter key directly
            if (wParam == 1) {
                KillTimer(hwnd, 1);
                LogToFile("Timer killed - no longer needed for EN_RETURN handling");
            }
            return 0;
            
        case WM_SIZE:
            {
                // 获取新的窗口大小
                int newWidth = LOWORD(lParam);
                int newHeight = HIWORD(lParam);
                char sizeLog[200] = {0};
                sprintf(sizeLog, "WM_SIZE: 窗口大小改变为 %dx%d", newWidth, newHeight);
                LogToFile(sizeLog);
                
                // 当窗口最小化时自动隐藏窗口到托盘
                if (wParam == SIZE_MINIMIZED)
                {
                    LogToFile("WM_SIZE: 窗口最小化，隐藏到托盘");
                    ShowWindow(hwnd, SW_HIDE);
                    
                    // 如果托盘图标尚未添加，则添加
                    if (!g_trayIconAdded)
                    {
                        AddTrayIcon();
                        CreateTrayMenu();
                    }
                }
                else
                {
                    // 窗口大小改变，重新布局控件
                    LogToFile("WM_SIZE: 窗口大小改变，重新布局控件");
                    LayoutControls(newWidth, newHeight);
                }
            }
            return 0;
            
        case WM_EXITSIZEMOVE:
            {
                // 用户完成调整窗口大小或移动窗口后，保存窗口设置
                LogToFile("WM_EXITSIZEMOVE: 窗口大小调整完成，保存窗口设置");
                SaveWindowSettings();
            }
            return 0;
            
        case WM_TRAYICON:
            // 处理系统托盘图标消息
            HandleTrayMessage(lParam);
            return 0;
            
        case WM_SETFOCUS:
            // Log focus event with detailed information
            LogToFile("WM_SETFOCUS received for main window - setting ignore flag for next EN_RETURN");
            // Always set the ignore flag when the main window gets focus
            // This prevents EN_RETURN events from being triggered when clicking on the edit control
            g_ignoreNextReturn = true;
            LogToFile("  Setting ignore flag for next EN_RETURN due to focus change");
            // Allow normal focus behavior but ensure no auto-execution happens
            // Call default handler to ensure normal focus functionality
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            
        case WM_NOTIFY:
            {
                NMHDR* nmhdr = (NMHDR*)lParam;
                // 处理ListView的双击事件
                if (nmhdr->idFrom == IDC_LISTVIEW)
                {
                    // 处理双击事件：NM_DBLCLK是标准的双击通知，LVN_ITEMACTIVATE是ListView的激活通知
                    if (nmhdr->code == NM_DBLCLK)
                    {
                        LogToFile("WM_NOTIFY: ListView双击事件");
                        // 获取当前选中的项
                        INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED | LVNI_SELECTED);
                        if (selIndex == -1)
                        {
                            // 如果没有选中项，尝试获取第一个项
                            selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_ALL);
                        }
                        
                        if (selIndex >= 0)
                        {
                            // 检查是否是设置菜单模式
                            if (g_settingsMenuMode)
                            {
                                // 设置菜单模式下，检查是否是提示行
                                if (selIndex == 0)
                                {
                                    LogToFile("WM_NOTIFY: 设置菜单模式下双击提示行，忽略");
                                }
                                else
                                {
                                    LogToFile("WM_NOTIFY: 设置菜单模式下双击菜单项");
                                    HandleSettingsMenuItemClick(selIndex);
                                }
                            }
                            // 检查是否是计算模式或网址收藏模式
                            else if (g_calculatorMode)
                            {
                                // 计算模式下，双击不执行，只允许回车执行
                                LogToFile("WM_NOTIFY: 计算模式下，双击不执行");
                            }
                            else if (g_bookmarkMode)
                            {
                                // 网址收藏模式下，双击打开选中的网址
                                if (selIndex < (INT_PTR)g_bookmarkSearchResults.size())
                                {
                                    LogToFile("WM_NOTIFY: 网址收藏模式下，双击打开选中的网址");
                                    std::wstring url = g_bookmarkSearchResults[selIndex].second;
                                    ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                }
                            }
                            else
                            {
                                // 普通模式下，双击执行选中的项
                                // 检查是否是有效的搜索结果（不是"No matching items found"）
                                if (selIndex < (INT_PTR)g_searchResults.size())
                                {
                                    LogToFile("WM_NOTIFY: 双击执行选中的搜索结果");
                                    ExecuteSelectedItem(selIndex);
                                }
                                else
                                {
                                    // 检查是否是"No matching items found"消息
                                    WCHAR itemText[1024] = {0};
                                    LVITEMW lvItem = {0};
                                    lvItem.iItem = (int)selIndex;
                                    lvItem.iSubItem = 0;
                                    lvItem.pszText = itemText;
                                    lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
                                    ListView_GetItem(g_hListView, &lvItem);
                                    
                                    if (wcscmp(itemText, L"No matching items found") != 0)
                                    {
                                        LogToFile("WM_NOTIFY: 双击的项目不在搜索结果中，尝试执行");
                                        ExecuteSelectedItem(selIndex);
                                    }
                                    else
                                    {
                                        LogToFile("WM_NOTIFY: 双击的是'No matching items found'消息，不执行");
                                    }
                                }
                            }
                        }
                        return 0;
                    }
                }
            }
            // 对于其他WM_NOTIFY消息，调用默认处理
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            
        case WM_COMMAND:
            {
                // Log WM_COMMAND message details for debugging
                char logMsg[200] = {0};
                sprintf(logMsg, "WM_COMMAND received: Control ID=%d, Notification=%d", LOWORD(wParam), HIWORD(wParam));
                LogToFile(logMsg);
                
                // 处理退出计算模式按钮点击
                if (LOWORD(wParam) == IDC_EXIT_CALC_BUTTON)
                {
                    // 处理退出计算模式按钮点击
                    if (g_calculatorMode)
                    {
                        ExitCalculatorMode();
                        LogToFile("WM_COMMAND: 用户点击退出计算模式按钮");
                    }
                    return 0;
                }
                // 设置按钮已移除，不再处理设置按钮点击事件
                // 处理退出网址收藏模式按钮点击（已禁用，只允许"q"退出）
                else if (LOWORD(wParam) == IDC_EXIT_BOOKMARK_BUTTON)
                {
                    // 处理退出网址收藏模式按钮点击
                    if (g_bookmarkMode)
                    {
                        LogToFile("WM_COMMAND: 退出网址收藏模式按钮已被禁用，只允许使用'q'键退出");
                        // 不再调用ExitBookmarkMode();
                        // 可选择显示提示信息
                        MessageBoxW(hwnd, L"请使用'q'键退出网址收藏模式", L"提示", MB_OK | MB_ICONINFORMATION);
                    }
                    return 0;
                }
                // 处理计算模式操作菜单按钮点击
                else if (LOWORD(wParam) == IDC_CALC_MENU_BUTTON)
                {
                    if (g_calculatorMode)
                    {
                        LogToFile("WM_COMMAND: 用户点击计算模式操作菜单按钮");
                        
                        // 获取按钮位置
                        RECT buttonRect;
                        GetWindowRect(g_hCalcMenuButton, &buttonRect);
                        
                        // 创建下拉菜单
                        HMENU hMenu = CreatePopupMenu();
                        
                        // 添加菜单项
                        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_COPY, L"复制选中项");
                        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_DELETE_ITEM, L"删除选中项");
                        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_CLEAR_ALL, L"清空历史记录");
                        
                        // 显示菜单（在按钮下方）
                        int command = TrackPopupMenu(hMenu, 
                            TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY,
                            buttonRect.left, buttonRect.bottom, 0, hwnd, NULL);
                        
                        // 销毁菜单
                        DestroyMenu(hMenu);
                        
                        // 处理用户选择
                        if (command == ID_CONTEXT_COPY)
                        {
                            CopySelectedListItem();
                            LogToFile("操作菜单: 复制了选中的项目");
                        }
                        else if (command == ID_CONTEXT_DELETE_ITEM)
                        {
                            // 删除选中的计算结果
                            if (g_calculationHistory.empty())
                            {
                                LogToFile("操作菜单: 历史记录为空，无法删除");
                                MessageBoxW(hwnd, L"历史记录为空，没有可删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                                return 0;
                            }
                            
                            // 获取ListView中选中的项目
                            INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                            if (selIndex == -1)
                            {
                                // 如果没有选中项，尝试获取焦点项
                                selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                            }
                            
                            if (selIndex < 0 || selIndex >= (INT_PTR)g_calculationHistory.size())
                            {
                                MessageBoxW(hwnd, L"请先选择要删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                                return 0;
                            }
                            
                            // 转换ListView索引到实际历史记录索引
                            size_t actualIndex = g_calculationHistory.size() - 1 - selIndex;
                            
                            if (actualIndex >= g_calculationHistory.size())
                            {
                                MessageBoxW(hwnd, L"索引转换错误，无法删除", L"错误", MB_OK | MB_ICONERROR);
                                return 0;
                            }
                            
                            // 从历史记录中删除
                            g_calculationHistory.erase(g_calculationHistory.begin() + actualIndex);
                            
                            // 保存到文件
                            SaveCalculationHistory();
                            
                            // 重新显示历史记录
                            DisplayCalculationHistory();
                            
                            // 更新WebView2显示
                            UpdateCalculatorModeWebView();
                            
                            LogToFile("操作菜单: 删除了选中的计算结果");
                        }
                        else if (command == ID_CONTEXT_CLEAR_ALL)
                        {
                            // 清空所有历史记录
                            if (MessageBoxW(hwnd, L"确定要清空所有计算历史吗？", 
                                L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES)
                            {
                                g_calculationHistory.clear();
                                SaveCalculationHistory();
                                DisplayCalculationHistory();
                                
                                // 更新WebView2显示
                                UpdateCalculatorModeWebView();
                                
                                LogToFile("操作菜单: 清空了所有计算历史");
                            }
                        }
                    }
                    return 0;
                }
                // 处理设置菜单命令
                else if (LOWORD(wParam) == ID_SETTINGS_BOOKMARK)
                {
                    // 显示网址设置对话框
                    LogToFile("WM_COMMAND: 用户选择设置菜单-网址设置");
                    ShowBookmarkDialog();
                    return 0;
                }
                else if (LOWORD(wParam) == ID_SETTINGS_EXIT)
                {
                    // 退出程序
                    LogToFile("WM_COMMAND: 用户选择设置菜单-退出程序");
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                }
                // 处理托盘菜单命令
                else if (LOWORD(wParam) == ID_TRAY_SHOW)
                {
                    ShowLauncherWindow();
                    LogToFile("WM_COMMAND: 用户选择显示窗口");
                    return 0;
                }
                else if (LOWORD(wParam) == ID_TRAY_EXIT)
                {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    LogToFile("WM_COMMAND: 用户选择退出");
                    return 0;
                }
                // 处理网址收藏模式右键菜单命令
                else if (LOWORD(wParam) == ID_CONTEXT_DELETE_BOOKMARK)
                {
                    // 删除选中的网址
                    INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                    if (selIndex != -1 && selIndex < (INT_PTR)g_bookmarkSearchResults.size())
                    {
                        // 获取选中的网址名称
                        std::wstring selectedName = g_bookmarkSearchResults[selIndex].first;
                        
                        // 在原始网址列表中查找并删除
                        for (size_t i = 0; i < g_bookmarks.size(); i++)
                        {
                            if (g_bookmarks[i].first == selectedName)
                            {
                                g_bookmarks.erase(g_bookmarks.begin() + i);
                                break;
                            }
                        }
                        
                        // 保存到文件
                        SaveBookmarks();
                        
                        // 重新搜索并显示结果
                        SearchBookmarks(g_currentSearch);
                        
                        LogToFile("WM_COMMAND: 删除了选中的网址");
                    }
                    return 0;
                }
                else if (LOWORD(wParam) == ID_CONTEXT_SYNC_CHROME)
                {
                    // 同步Chrome书签
                    SyncChromeBookmarks();
                    LogToFile("WM_COMMAND: 同步Chrome书签");
                    return 0;
                }
                
                if (LOWORD(wParam) == IDC_EDIT)
                {
                    // Declare searchText before switch statement to avoid initialization issues
                    WCHAR searchText[1024] = {0};
                    
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            // Log edit control EN_CHANGE notification
                            LogToFile("  Edit control EN_CHANGE notification - processing search");
                            
                            // Real-time search
                            GetWindowTextW(g_hEdit, searchText, sizeof(searchText)/sizeof(WCHAR));
                            
                            // 记录输入的字符到日志
                            {
                                char inputCharLog[1024] = {0};
                                WideCharToMultiByte(CP_UTF8, 0, searchText, -1, inputCharLog, sizeof(inputCharLog), NULL, NULL);
                                char changeLog[1100] = {0};
                                sprintf(changeLog, "  EN_CHANGE: 输入字符: '%s'", inputCharLog);
                                LogToFile(changeLog);
                            }
                            
                            wcscpy(g_currentSearch, searchText);
                            
                            // 检查是否在计算模式
                            if (g_calculatorMode)
                            {
                                // 在计算模式下，不进行搜索，也不实时计算，只记录输入变化
                                LogToFile("  EN_CHANGE: 计算模式下，输入内容已变化，但不计算");
                            }
                            else
                            {
                                // 在搜索模式下，但需要检查是否是完整的特殊命令
                                // 只有当输入不是特殊命令时才进行搜索，避免误解发
                                if (wcscmp(searchText, L"js") != 0 && 
                                    wcscmp(searchText, L"help") != 0 && 
                                    wcscmp(searchText, L"set") != 0)
                                {
                                    // 在搜索模式下，进行正常搜索
                                    SearchAndDisplayResults(searchText);
                                }
                                else
                                {
                                    // 输入为特殊命令，但不进行搜索，只清空搜索结果
                                    LogToFile("  EN_CHANGE: 检测到特殊命令输入，但不执行搜索，等待回车键");
                                    SearchAndDisplayResults(L""); // 清空搜索结果
                                }
                            }
                            break;
                            
                        case EN_RETURN:
                            // Log edit control EN_RETURN notification
                            LogToFile("  Edit control EN_RETURN notification - processing");
                            
                            // 打印ListView所有内容用于调试
                            LogToFile("  EN_RETURN: 回车键按下，打印ListView内容:");
                            LogListViewContents();
                            
                            // Handle Enter key press in edit control
                            // Check if this EN_RETURN is caused by focus change and should be ignored
                            if (g_ignoreNextReturn)
                            {
                                LogToFile("  EN_RETURN: 回车键被忽略 (焦点变化导致)");
                                g_ignoreNextReturn = false; // Reset the flag
                            }
                            else
                            {
                                LogToFile("  EN_RETURN: 处理用户按下的回车键");
                                
                                // 获取当前输入框的内容
                                WCHAR currentText[1024] = {0};
                                GetWindowTextW(g_hEdit, currentText, sizeof(currentText)/sizeof(WCHAR));
                                char currentTextLog[1024] = {0};
                                WideCharToMultiByte(CP_UTF8, 0, currentText, -1, currentTextLog, sizeof(currentTextLog), NULL, NULL);
                                char enterLog[1100] = {0};
                                sprintf(enterLog, "  EN_RETURN: 当前输入框内容: '%s'", currentTextLog);
                                LogToFile(enterLog);
                                
                                // 检查是否在计算模式
                                if (g_calculatorMode)
                                {
                                    // 在计算模式下，计算表达式
                                    LogToFile("  EN_RETURN: 计算模式下，计算表达式");
                                    EvaluateExpression(currentText);
                                }
                                else
                                {
                                    // 在搜索模式下，执行正常的搜索结果
                                    // Handle return key - Ensure it executes the first item
                                    {
                                        int itemCount = ListView_GetItemCount(g_hListView);
                                        char logMsg[200] = {0};
                                        sprintf(logMsg, "  EN_RETURN: 列表框项目数量: %d", itemCount);
                                        LogToFile(logMsg);
                                    
                                        // 检查输入内容是否是特殊命令 - 优先处理命令
                                        if (wcscmp(currentText, L"js") == 0)
                                        {
                                            LogToFile("  EN_RETURN: 识别为'js'命令，调用ProcessCommand");
                                            ProcessCommand(currentText);
                                            return 0;
                                        }
                                        else if (wcscmp(currentText, L"help") == 0)
                                        {
                                            LogToFile("  EN_RETURN: 识别为'help'命令，显示使用帮助");
                                            ShowHelpInfo();
                                            return 0;
                                        }
                                        else if (wcscmp(currentText, L"set") == 0)
                                        {
                                            LogToFile("  EN_RETURN: 识别为'set'命令，显示设置菜单");
                                            ShowSettingsMenu();
                                            return 0;
                                        }
                                        else if (itemCount > 0)
                                        {
                                            // 获取第一个实际项目（跳过提示行）
                                            INT_PTR firstSelIndex = GetFirstActualItemIndex();
                                            if (firstSelIndex == -1)
                                            {
                                                LogToFile("  EN_RETURN: 只有提示行，没有实际项目");
                                                return 0;
                                            }
                                            
                                            // Force select the first actual item to ensure it's highlighted
                                            ListView_SetItemState(g_hListView, firstSelIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                                            LogToFile("  EN_RETURN: 强制选择第一个实际项目（跳过提示行）");
                                            
                                            // 获取第一个实际项目的文本
                                            WCHAR firstItemText[1024] = {0};
                                            LVITEMW lvItem = {0};
                                            lvItem.iItem = (int)firstSelIndex;
                                            lvItem.iSubItem = 0;
                                            lvItem.pszText = firstItemText;
                                            lvItem.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
                                            ListView_GetItem(g_hListView, &lvItem);
                                            char firstItemLog[1024] = {0};
                                            WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                                            sprintf(logMsg, "  EN_RETURN: 第一个实际项目文本: '%s'", firstItemLog);
                                            LogToFile(logMsg);
                                            
                                            // 检查是否是收藏的网址
                                            bool isBookmark = (wcsstr(firstItemText, L"收藏:") == firstItemText);
                                            if (isBookmark)
                                            {
                                                LogToFile("  EN_RETURN: 识别为收藏的网址，直接打开");
                                                // 获取收藏的网址名称
                                                std::wstring bookmarkName = firstItemText + 4; // 跳过"收藏:"前缀
                                                
                                                // 在收藏中查找对应的网址
                                                for (size_t i = 0; i < g_bookmarks.size(); i++)
                                                {
                                                    if (g_bookmarks[i].first == bookmarkName)
                                                    {
                                                        // 直接打开收藏的网址
                                                        ShellExecuteW(NULL, L"open", g_bookmarks[i].second.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                        LogToFile("  EN_RETURN: 成功打开收藏的网址");
                                                        break;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                // Verify g_searchResults has items before executing
                                                // Also check if the first item is not the "No matching items found" message
                                                if (!g_searchResults.empty() && g_searchResults.size() > 0)
                                                {
                                                    LogToFile("  EN_RETURN: 搜索结果不为空，执行第一个项目");
                                                    ExecuteSelectedItem(firstSelIndex);
                                                }
                                                else
                                                {
                                                    // Check if the first item is "No matching items found"
                                                    if (wcscmp(firstItemText, L"No matching items found") == 0)
                                                    {
                                                        LogToFile("  EN_RETURN: 第一个项目是'未找到匹配项'消息，不执行");
                                                    }
                                                    else
                                                    {
                                                        LogToFile("  EN_RETURN: 错误：搜索结果为空但列表框有实际项目");
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // If no items, process as command
                                            if (wcslen(currentText) > 0)
                                            {
                                                sprintf(logMsg, "  EN_RETURN: 列表为空，将输入内容作为命令处理: '%s'", currentTextLog);
                                                LogToFile(logMsg);
                                                ProcessCommand(currentText);
                                            }
                                            else
                                            {
                                                LogToFile("  EN_RETURN: 列表为空且输入内容为空，不执行任何操作");
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        
                        // Explicitly ignore all other edit control notifications
                        default:
                            // Log other edit control notifications
                            {
                                char logMsg[200] = {0};
                                sprintf(logMsg, "  Edit control unknown notification: %d", HIWORD(wParam));
                                LogToFile(logMsg);
                            }
                            break;
                    }
                }
                else if (LOWORD(wParam) == IDC_LISTVIEW)
                {
                    // Only handle double click - explicitly ignore all other listbox notifications
                    // This prevents auto-opening on selection change or focus change
                    if (HIWORD(wParam) == LBN_DBLCLK)
                    {
                        INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                        if (selIndex != -1)
                        {
                            if (g_settingsMenuMode)
                            {
                                if (selIndex == 0)
                                {
                                    LogToFile("WM_COMMAND: 设置菜单模式下双击提示行，忽略");
                                    return 0;
                                }
                                
                                LogToFile("WM_COMMAND: 检测到设置菜单模式，调用菜单项处理函数");
                                HandleSettingsMenuItemClick(selIndex);
                            }
                            else
                            {
                                // 正常模式，执行选中的项目
                                LogToFile("WM_COMMAND: 正常模式，执行选中的项目");
                                ExecuteSelectedItem(selIndex);
                            }
                        }
                    }
                    // Explicitly ignore LBN_SELCHANGE to prevent auto-open on selection
                    // Also ignore LBN_SETFOCUS and other notifications
                }
                return 0;
            }
            
        case WM_KEYDOWN:
            // Handle Delete key press in bookmark mode
            if (wParam == VK_DELETE && g_bookmarkMode && GetFocus() == g_hListView)
            {
                LogToFile("WM_KEYDOWN: Delete key pressed in bookmark mode");
                
                // 获取选中的网址索引
                INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                if (selIndex != LB_ERR && selIndex < (INT_PTR)g_bookmarkSearchResults.size())
                {
                    // 获取选中的网址名称
                    std::wstring selectedName = g_bookmarkSearchResults[selIndex].first;
                    
                    // 确认删除
                    WCHAR confirmMsg[512];
                    swprintf(confirmMsg, sizeof(confirmMsg)/sizeof(WCHAR), L"确定要删除网址 '%s' 吗？", selectedName.c_str());
                    if (MessageBoxW(g_hMainWindow, confirmMsg, L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES)
                    {
                        // 在原始网址列表中查找并删除
                        for (size_t i = 0; i < g_bookmarks.size(); i++)
                        {
                            if (g_bookmarks[i].first == selectedName)
                            {
                                g_bookmarks.erase(g_bookmarks.begin() + i);
                                break;
                            }
                        }
                        
                        // 保存到文件
                        SaveBookmarks();
                        
                        // 重新搜索并显示结果
                        SearchBookmarks(g_currentSearch);
                        
                        LogToFile("WM_KEYDOWN: 删除了选中的网址");
                    }
                    else
                    {
                        LogToFile("WM_KEYDOWN: 用户取消了删除操作");
                    }
                }
                else
                {
                    LogToFile("WM_KEYDOWN: 没有选中任何网址");
                }
                return 0; // 消息已处理
            }
            // Handle Enter key press directly in edit control
            else if (wParam == VK_RETURN && GetFocus() == g_hEdit)
            {
                LogToFile("WM_KEYDOWN: Enter key pressed - processing directly");
                
                // 获取当前输入框的内容
                WCHAR currentText[1024] = {0};
                GetWindowTextW(g_hEdit, currentText, sizeof(currentText)/sizeof(WCHAR));
                char currentTextLog[1024] = {0};
                WideCharToMultiByte(CP_UTF8, 0, currentText, -1, currentTextLog, sizeof(currentTextLog), NULL, NULL);
                char enterLog[1100] = {0};
                sprintf(enterLog, "  WM_KEYDOWN: 当前输入框内容: '%s'", currentTextLog);
                LogToFile(enterLog);
                
                // 打印ListView所有内容用于调试
                LogToFile("  WM_KEYDOWN: 回车键按下，打印ListView内容:");
                LogListViewContents();
                
                // 检查是否在计算模式
                if (g_calculatorMode)
                {
                    // 在计算模式下，计算表达式
                    LogToFile("  WM_KEYDOWN: 计算模式下，计算表达式");
                    EvaluateExpression(currentText);
                }
                else
                {
                    // 在搜索模式下，执行正常的搜索结果
                    // Handle return key - Ensure it executes the first item
                    {
                        int itemCount = ListView_GetItemCount(g_hListView);
                        char logMsg[200] = {0};
                        sprintf(logMsg, "  WM_KEYDOWN: 列表框项目数量: %d", itemCount);
                        LogToFile(logMsg);
                    
                        // 检查输入内容是否是"js"命令或"q"命令 - 优先处理命令
                        if (wcscmp(currentText, L"js") == 0)
                        {
                            LogToFile("  WM_KEYDOWN: 识别为'js'命令，调用ProcessCommand");
                            ProcessCommand(currentText);
                            return 0;
                        }
                        else if (itemCount > 0)
                        {
                            // 获取第一个实际项目（跳过提示行）
                            INT_PTR firstSelIndex = GetFirstActualItemIndex();
                            if (firstSelIndex == -1)
                            {
                                LogToFile("  WM_KEYDOWN: 只有提示行，没有实际项目");
                                return 0;
                            }
                            
                            // Force select the first actual item to ensure it's highlighted
                            ListView_SetItemState(g_hListView, firstSelIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            LogToFile("  WM_KEYDOWN: 强制选择第一个实际项目（跳过提示行）");
                            
                            // 获取第一个实际项目的文本
                            WCHAR firstItemText[1024] = {0};
                            LVITEMW lvItem = {0};
                            lvItem.iItem = (int)firstSelIndex;
                            lvItem.iSubItem = 0;
                            lvItem.pszText = firstItemText;
                            lvItem.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
                            ListView_GetItem(g_hListView, &lvItem);
                            char firstItemLog[1024] = {0};
                            WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                            sprintf(logMsg, "  WM_KEYDOWN: 第一个实际项目文本: '%s'", firstItemLog);
                            LogToFile(logMsg);
                            
                            // Verify g_searchResults has items before executing
                            // Also check if the first item is not the "No matching items found" message
                            if (!g_searchResults.empty() && g_searchResults.size() > 0)
                            {
                                LogToFile("  WM_KEYDOWN: 搜索结果不为空，执行第一个项目");
                                ExecuteSelectedItem(firstSelIndex);
                            }
                            else
                            {
                                // Check if the first item is "No matching items found"
                                if (wcscmp(firstItemText, L"No matching items found") == 0)
                                {
                                    LogToFile("  WM_KEYDOWN: 第一个项目是'未找到匹配项'消息，不执行");
                                }
                                else
                                {
                                    LogToFile("  WM_KEYDOWN: 错误：搜索结果为空但列表框有实际项目");
                                }
                            }
                        }
                        else
                        {
                            // If no items, process as command
                            if (wcslen(currentText) > 0)
                            {
                                sprintf(logMsg, "  WM_KEYDOWN: 列表为空，将输入内容作为命令处理: '%s'", currentTextLog);
                                LogToFile(logMsg);
                                ProcessCommand(currentText);
                            }
                            else
                            {
                                LogToFile("  WM_KEYDOWN: 列表为空且输入内容为空，不执行任何操作");
                            }
                        }
                    }
                }
                return 0; // 消息已处理，不需要进一步处理
            }
            
        case WM_CONTEXTMENU:
            // 处理右键菜单
            {
                // 获取鼠标位置
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                
                // 如果坐标是(-1, -1)，表示由键盘触发，使用当前鼠标位置
                if (pt.x == -1 && pt.y == -1)
                {
                    GetCursorPos(&pt);
                }
                
                // 检查是否在计算模式下，并且右键点击的是列表框
                if (g_calculatorMode)
                {
                    // 获取列表框的屏幕坐标
                    RECT listBoxRect;
                    GetWindowRect(g_hListView, &listBoxRect);
                    
                    // 检查鼠标是否在列表框内
                    if (PtInRect(&listBoxRect, pt))
                    {
                        // 创建右键菜单
                        HMENU hContextMenu = CreatePopupMenu();
                        
                        // 添加菜单项
                        AppendMenuW(hContextMenu, MF_STRING, ID_CONTEXT_COPY, L"复制");
                        AppendMenuW(hContextMenu, MF_STRING, ID_CONTEXT_DELETE_ITEM, L"删除此项");
                        AppendMenuW(hContextMenu, MF_STRING, ID_CONTEXT_CLEAR_ALL, L"清空历史");
                        
                        // 显示菜单并获取用户选择
                        int command = TrackPopupMenu(hContextMenu, 
                            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                            pt.x, pt.y, 0, g_hMainWindow, NULL);
                        
                        // 销毁菜单
                        DestroyMenu(hContextMenu);
                        
                        // 处理用户选择
                        if (command == ID_CONTEXT_COPY)
                        {
                            // 复制选中的项目
                            CopySelectedListItem();
                            LogToFile("右键菜单: 复制了选中的项目");
                        }
                        else if (command == ID_CONTEXT_DELETE_ITEM)
                        {
                            // 删除选中的计算结果
                            
                            // 检查历史记录是否为空
                            if (g_calculationHistory.empty())
                            {
                                LogToFile("ListView删除: 历史记录为空，无法删除");
                                MessageBoxW(g_hMainWindow, L"历史记录为空，没有可删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                                return 0;
                            }
                            
                            // 记录删除前状态
                            char beforeStateLog[300] = {0};
                            sprintf(beforeStateLog, "删除开始: g_calculationHistory大小=%zu, ListView项目数=%d", 
                                g_calculationHistory.size(), ListView_GetItemCount(g_hListView));
                            LogToFile(beforeStateLog);
                            
                            // 获取ListView中当前鼠标位置处的项目（右键点击的项目）
                            LVHITTESTINFO lvhti = {0};
                            lvhti.pt = pt;
                            // 转换屏幕坐标到ListView坐标
                            ScreenToClient(g_hListView, &lvhti.pt);
                            
                            // 获取点击处的项目索引
                            INT_PTR clickedIndex = ListView_HitTest(g_hListView, &lvhti);
                            
                            // 记录鼠标点击测试结果
                            char logMsg0[300] = {0};
                            sprintf(logMsg0, "ListView删除: 鼠标点击索引=%d, 标志=0x%x", (int)clickedIndex, lvhti.flags);
                            LogToFile(logMsg0);
                            
                            // 如果无法通过鼠标点击获取索引，则尝试获取选中项
                            INT_PTR selIndex = -1;
                            if (clickedIndex != -1 && clickedIndex >= 0) {
                                selIndex = clickedIndex;
                                LogToFile("ListView删除: 使用鼠标点击索引");
                            } else {
                                // 尝试获取选中项
                                selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
                                if (selIndex == -1) {
                                    // 如果没有选中项，尝试获取焦点项
                                    selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                                }
                                LogToFile("ListView删除: 使用选择/焦点索引");
                            }
                            
                            // 记录选中索引的获取方式
                            char logMsg1[300] = {0};
                            sprintf(logMsg1, "ListView删除: 最终使用索引=%d, 历史记录总数=%zu, ListView项目数=%d", 
                                (int)selIndex, g_calculationHistory.size(), ListView_GetItemCount(g_hListView));
                            LogToFile(logMsg1);
                            
                            // 验证索引是否有效（ListView索引应该在[0, g_calculationHistory.size()-1]范围内）
                            if (selIndex < 0 || selIndex >= (INT_PTR)g_calculationHistory.size()) {
                                char errorLog[300] = {0};
                                if (g_calculationHistory.size() > 0) {
                                    sprintf(errorLog, "ListView删除: 选中索引=%d 超出有效范围[0,%zu]", (int)selIndex, g_calculationHistory.size()-1);
                                } else {
                                    sprintf(errorLog, "ListView删除: 选中索引=%d 但历史记录为空", (int)selIndex);
                                }
                                LogToFile(errorLog);
                                MessageBoxW(g_hMainWindow, L"请先选择要删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                                return 0;
                            }
                            
                            // 修复索引转换逻辑：
                            // DisplayCalculationHistory使用反向迭代器显示，所以：
                            // ListView第0行 = g_calculationHistory的最后一条记录（最新）
                            // ListView第1行 = g_calculationHistory的倒数第二条记录
                            // ListView第N行 = g_calculationHistory的第(N+1)条记录
                            // 因此ListView选中索引selIndex对应的实际索引是：
                            // actualIndex = g_calculationHistory.size() - 1 - selIndex
                            size_t actualIndex = g_calculationHistory.size() - 1 - selIndex;
                            
                            // 再次验证转换后的索引
                            if (actualIndex >= g_calculationHistory.size()) {
                                char errorLog[300] = {0};
                                sprintf(errorLog, "ListView删除: 索引转换后超出范围: selIndex=%d -> actualIndex=%zu (历史记录总数=%zu)", 
                                    (int)selIndex, actualIndex, g_calculationHistory.size());
                                LogToFile(errorLog);
                                MessageBoxW(g_hMainWindow, L"索引转换错误，无法删除", L"错误", MB_OK | MB_ICONERROR);
                                return 0;
                            }
                            
                            // 记录索引转换详情
                            char logMsg2[300] = {0};
                            sprintf(logMsg2, "ListView删除: ListView索引=%d -> 实际索引=%zu (计算: %zu - 1 - %d)", 
                                (int)selIndex, actualIndex, g_calculationHistory.size(), (int)selIndex);
                            LogToFile(logMsg2);
                            
                            // 记录要删除的记录内容
                            char expression[200] = {0};
                            WideCharToMultiByte(CP_ACP, 0, g_calculationHistory[actualIndex].expression.c_str(), -1, expression, sizeof(expression), NULL, NULL);
                            char result[200] = {0};
                            WideCharToMultiByte(CP_ACP, 0, g_calculationHistory[actualIndex].result.c_str(), -1, result, sizeof(result), NULL, NULL);
                            char logMsg3[400] = {0};
                            sprintf(logMsg3, "ListView删除: 将删除记录: %s = %s", expression, result);
                            LogToFile(logMsg3);
                            
                            // 从历史记录中删除
                            LogToFile("ListView删除: 开始执行删除操作");
                            g_calculationHistory.erase(g_calculationHistory.begin() + actualIndex);
                            
                            // 保存到文件
                            SaveCalculationHistory();
                            
                            // 重新显示历史记录
                            LogToFile("ListView删除: 删除完成，重新显示历史记录");
                            DisplayCalculationHistory();
                            
                            // 记录删除后状态
                            char afterStateLog[300] = {0};
                            sprintf(afterStateLog, "删除完成: g_calculationHistory大小=%zu, ListView项目数=%d", 
                                g_calculationHistory.size(), ListView_GetItemCount(g_hListView));
                            LogToFile(afterStateLog);
                            
                            LogToFile("右键菜单: 删除了选中的计算结果");
                        }
                        else if (command == ID_CONTEXT_CLEAR_ALL)
                        {
                            // 清空所有历史记录
                            if (MessageBoxW(g_hMainWindow, L"确定要清空所有计算历史吗？", 
                                L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES)
                            {
                                g_calculationHistory.clear();
                                SaveCalculationHistory();
                                DisplayCalculationHistory();
                                LogToFile("右键菜单: 清空了所有计算历史");
                            }
                        }
                        
                        return 0; // 消息已处理
                    }
                }
                // 检查是否在网址收藏模式下，并且右键点击的是列表框
                else if (g_bookmarkMode)
                {
                    // 获取列表框的屏幕坐标
                    RECT listBoxRect;
                    GetWindowRect(g_hListView, &listBoxRect);
                    
                    // 检查鼠标是否在列表框内
                    if (PtInRect(&listBoxRect, pt))
                    {
                        // 创建右键菜单
                        HMENU hContextMenu = CreatePopupMenu();
                        
                        // 添加菜单项
                        AppendMenuW(hContextMenu, MF_STRING, ID_CONTEXT_DELETE_BOOKMARK, L"删除此项");
                        AppendMenuW(hContextMenu, MF_STRING, ID_CONTEXT_SYNC_CHROME, L"同步Chrome书签");
                        
                        // 显示菜单并获取用户选择
                        int command = TrackPopupMenu(hContextMenu, 
                            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                            pt.x, pt.y, 0, g_hMainWindow, NULL);
                        
                        // 销毁菜单
                        DestroyMenu(hContextMenu);
                        
                        // 处理用户选择
                        if (command == ID_CONTEXT_DELETE_BOOKMARK)
                        {
                            // 删除选中的网址
                            INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                            if (selIndex != -1 && selIndex < (INT_PTR)g_bookmarkSearchResults.size())
                            {
                                // 获取选中的网址名称
                                std::wstring selectedName = g_bookmarkSearchResults[selIndex].first;
                                
                                // 在原始网址列表中查找并删除
                                for (size_t i = 0; i < g_bookmarks.size(); i++)
                                {
                                    if (g_bookmarks[i].first == selectedName)
                                    {
                                        g_bookmarks.erase(g_bookmarks.begin() + i);
                                        break;
                                    }
                                }
                                
                                // 保存到文件
                                SaveBookmarks();
                                
                                // 重新搜索并显示结果
                                SearchBookmarks(g_currentSearch);
                                
                                LogToFile("右键菜单: 删除了选中的网址");
                            }
                        }
                        else if (command == ID_CONTEXT_SYNC_CHROME)
                        {
                            // 同步Chrome书签
                            SyncChromeBookmarks();
                            LogToFile("右键菜单: 同步Chrome书签");
                        }
                        
                        return 0; // 消息已处理
                    }
                }
            }
            // For other contexts, fall through to default handler
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    LogToFile("Program started");
    g_hInstance = hInstance;

    bool comInitialized = false;
    HRESULT hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrCoInit))
    {
        comInitialized = true;
        LogToFile("CoInitializeEx succeeded with COINIT_APARTMENTTHREADED");
    }
    else
    {
        char logMsg[200] = {0};
        sprintf(logMsg, "CoInitializeEx failed: 0x%08X", hrCoInit);
        LogToFile(logMsg);
        MessageBoxW(NULL, L"初始化 WebView2 所需的 COM 环境失败，请重启或检查系统设置。", L"Funny Quick", MB_ICONERROR | MB_OK);
        return 0;
    }
    
    // 初始化Common Controls，ListView需要用到
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    
    // 注意：快捷方式初始化移到窗口创建后，先显示帮助信息
    // InitializeCommonShortcuts();  // 移到窗口创建后
    
    // 创建UI字体
    CreateUIFont();
    LogToFile("UI字体已创建");
    
    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"QuickLauncherClass";
    
    // 从文件加载自定义图标作为窗口类图标
    wc.hIcon = (HICON)LoadImageW(
        NULL, 
        L"app_icon.ico", 
        IMAGE_ICON, 
        GetSystemMetrics(SM_CXICON), 
        GetSystemMetrics(SM_CYICON), 
        LR_LOADFROMFILE
    );
    
    // 从文件加载自定义图标作为小图标
    wc.hIconSm = (HICON)LoadImageW(
        NULL, 
        L"app_icon.ico", 
        IMAGE_ICON, 
        GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), 
        LR_LOADFROMFILE
    );
    
    // 如果加载自定义图标失败，使用系统默认图标作为备选
    if (!wc.hIcon) {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    if (!wc.hIconSm) {
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    // Register window class
    if (!RegisterClassExW(&wc))
    {
        LogToFile("Window class registration failed");
        return 0;
    }
    LogToFile("Window class registered successfully");

    // 加载保存的窗口设置，允许用户调整大小
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 800;
    int windowHeight = 600;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    // 从注册表加载窗口设置
    LoadWindowSettings(x, y, windowWidth, windowHeight);
    
    // Create window with resizable style (removed the resizing restrictions)
    g_hMainWindow = CreateWindowExW(
        0,
        L"QuickLauncherClass",
        L"快速启动",
        WS_OVERLAPPEDWINDOW,  // 允许调整大小
        x, y, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL);
    
    if (g_hMainWindow == NULL)
    {
        LogToFile("Window creation failed");
        return 0;
    }
    LogToFile("Window created successfully");
    
    // Show window
    ShowWindow(g_hMainWindow, SW_SHOW);
    UpdateWindow(g_hMainWindow);
    
    // Wait a bit for the window to be fully created and controls to be initialized
    // Process any pending messages to ensure WM_CREATE is handled
    MSG initMsg;
    while (PeekMessageW(&initMsg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&initMsg);
        DispatchMessageW(&initMsg);
    }
    
    // 设置初始化标志，防止自动执行
    g_windowInitializing = true;
    
    // Now set focus to edit box
    if (g_hEdit)
    {
        SetFocus(g_hEdit);
        LogToFile("Set focus to edit control after window creation");
    }
    
    // 先显示帮助信息
    if (g_hListView)
    {
        LogToFile("显示启动帮助信息");
        ShowHelpInfo();
    }
    
    // 然后初始化快捷方式（延迟初始化，确保WebView2已准备好）
    LogToFile("初始化快捷方式");
    InitializeCommonShortcuts();
    
    // 初始化完成后，显示默认搜索结果
    if (g_hListView)
    {
        SearchAndDisplayResults(L"");
        LogToFile("Displayed default search results after initialization");
    }
    
    // 处理所有待处理的消息，确保初始化消息都已处理
    MSG initMsg2;
    int messageCount2 = 0;
    while (PeekMessageW(&initMsg2, NULL, 0, 0, PM_REMOVE) && messageCount2 < 50)
    {
        TranslateMessage(&initMsg2);
        DispatchMessageW(&initMsg2);
        messageCount2++;
    }
    
    // 延迟清除初始化标志，确保所有初始化消息都已处理
    Sleep(200);
    g_windowInitializing = false;
    LogToFile("Window initialization complete, auto-execution enabled");
    
    // Register hotkey combination Ctrl+Alt+Q
    if (!RegisterHotKey(g_hMainWindow, HOTKEY_ID, MOD_CONTROL | MOD_ALT, 'Q'))
    {
        DWORD errorCode = GetLastError();
        char logMsg[100] = {0};
        sprintf(logMsg, "Hotkey registration failed with error: %lu", errorCode);
        LogToFile(logMsg);
    }
    else
    {
        LogToFile("Hotkey Ctrl+Alt+Q registered successfully");
    }
    
    // Register hotkey combination Ctrl+F1
    if (!RegisterHotKey(g_hMainWindow, HOTKEY_ID_CTRL_F1, MOD_CONTROL, VK_F1))
    {
        DWORD errorCode = GetLastError();
        char logMsg[100] = {0};
        sprintf(logMsg, "Ctrl+F1 hotkey registration failed with error: %lu", errorCode);
        LogToFile(logMsg);
    }
    else
    {
        LogToFile("Hotkey Ctrl+F1 registered successfully");
    }
    
    // Register hotkey combination Ctrl+F2
    if (!RegisterHotKey(g_hMainWindow, HOTKEY_ID_CTRL_F2, MOD_CONTROL, VK_F2))
    {
        DWORD errorCode = GetLastError();
        char logMsg[100] = {0};
        sprintf(logMsg, "Ctrl+F2 hotkey registration failed with error: %lu", errorCode);
        LogToFile(logMsg);
    }
    else
    {
        LogToFile("Hotkey Ctrl+F2 registered successfully");
    }
    
    // 初始化系统托盘图标和菜单
    AddTrayIcon();
    CreateTrayMenu();
    LogToFile("System tray icon and menu initialized");
    
    // 加载计算历史记录
    LoadCalculationHistory();
    
    // Start message loop
    LogToFile("Starting message loop");
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    // Clean up
    LogToFile("Exiting, unregistering hotkey");
    if (comInitialized)
    {
        CoUninitialize();
        LogToFile("COM uninitialized");
    }
    return (int)msg.wParam;
}

// Edit control subclassing procedure implementation
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                LogToFile("EditSubclassProc: WM_KEYDOWN with VK_RETURN received");
                
                // Get current text from edit control
                WCHAR currentText[1024] = {0};
                GetWindowTextW(hwnd, currentText, sizeof(currentText)/sizeof(WCHAR));
                char currentTextLog[1024] = {0};
                WideCharToMultiByte(CP_UTF8, 0, currentText, -1, currentTextLog, sizeof(currentTextLog), NULL, NULL);
                char enterLog[1100] = {0};
                sprintf(enterLog, "  EditSubclassProc: Current edit text: '%s'", currentTextLog);
                LogToFile(enterLog);
                
                // 打印ListView所有内容用于调试
                LogToFile("  EditSubclassProc: 回车键按下，打印ListView内容:");
                LogListViewContents();
                
                // Handle return key - Ensure it executes the first item
                {
                    int itemCount = ListView_GetItemCount(g_hListView);
                    char logMsg[200] = {0};
                    sprintf(logMsg, "  EditSubclassProc: Listbox item count: %d", itemCount);
                    LogToFile(logMsg);
                
                    // If list has items, explicitly select and open the first one
                    if (itemCount > 0)
                    {
                        // 首先检查特殊命令（"js"、"wz"、"dir"、"set"、"help"等），优先处理这些命令
                        if (!g_calculatorMode && (wcscmp(currentText, L"js") == 0 || wcscmp(currentText, L"wz") == 0 || wcscmp(currentText, L"dir") == 0 || wcscmp(currentText, L"set") == 0 || wcscmp(currentText, L"help") == 0))
                        {
                            LogToFile("  EditSubclassProc: 检测到特殊命令，调用ProcessCommand处理");
                            ProcessCommand(currentText);
                            return 0; // 特殊命令处理完成，不执行搜索结果
                        }
                        
                        // 检查是否在计算模式、网址收藏模式或目录浏览模式，优先处理"q"退出命令
                        if (g_calculatorMode || g_bookmarkMode || g_dirMode)
                        {
                            // 在特殊模式下，首先检查"q"退出命令
                            if (wcscmp(currentText, L"q") == 0)
                            {
                                LogToFile("  EditSubclassProc: 检测到特殊模式下输入'q'，退出当前模式");
                                if (g_calculatorMode)
                                {
                                    ExitCalculatorMode();
                                }
                                if (g_bookmarkMode)
                                {
                                    ExitBookmarkMode();
                                }
                                if (g_dirMode)
                                {
                                    ExitDirMode();
                                }
                            }
                            else
                            {
                                // 不是"q"命令，按照模式特定方式处理
                                if (g_calculatorMode)
                                {
                                    LogToFile("  EditSubclassProc: 计算模式下，忽略列表项，调用EvaluateExpression");
                                    EvaluateExpression(currentText);
                                }
                                else if (g_bookmarkMode)
                                {
                                    LogToFile("  EditSubclassProc: 网址收藏模式下，搜索网址收藏");
                                    SearchBookmarks(currentText);
                                }
                            }
                            return 0; // 特殊模式处理完成
                        }
                        else
                        {
                            // 获取第一个实际项目（跳过提示行）
                            INT_PTR firstSelIndex = GetFirstActualItemIndex();
                            if (firstSelIndex == -1)
                            {
                                LogToFile("  EditSubclassProc: 只有提示行，没有实际项目");
                                return 0;
                            }
                            
                            // Force select the first actual item to ensure it's highlighted
                            ListView_SetItemState(g_hListView, firstSelIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            LogToFile("  EditSubclassProc: Force selecting first actual item (跳过提示行)");
                            
                            // Get first actual item text
                            WCHAR firstItemText[1024] = {0};
                            LVITEMW lvItem = {0};
                            lvItem.mask = LVIF_TEXT;  // 添加mask标志
                            lvItem.iItem = (int)firstSelIndex;
                            lvItem.iSubItem = 0;
                            lvItem.pszText = firstItemText;
                            lvItem.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
                            int getItemResult = ListView_GetItem(g_hListView, &lvItem);
                            char firstItemLog[1024] = {0};
                            WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                            sprintf(logMsg, "  EditSubclassProc: First actual item text: '%s' (GetItem返回值: %d, 文本长度: %zu)", 
                                    firstItemLog, getItemResult, wcslen(firstItemText));
                            LogToFile(logMsg);
                            
                            // 如果获取失败，尝试直接使用g_searchResults
                            if (wcslen(firstItemText) == 0 && !g_searchResults.empty())
                            {
                                char fallbackLog[300] = {0};
                                char fallbackName[256] = {0};
                                WideCharToMultiByte(CP_UTF8, 0, g_searchResults[0].name, -1, fallbackName, sizeof(fallbackName), NULL, NULL);
                                sprintf(fallbackLog, "  EditSubclassProc: ListView获取失败，使用g_searchResults[0]: '%s'", fallbackName);
                                LogToFile(fallbackLog);
                            }
                            
                            // Verify g_searchResults has items before executing
                            // Also check if the first item is not the "No matching items found" message
                            // 只有在不是窗口初始化时才自动执行
                            // 并且只有在用户明确按回车键时才执行，不允许自动执行
                            if (!g_windowInitializing && !g_searchResults.empty() && g_searchResults.size() > 0)
                            {
                                // 只有在用户按回车键时才执行，不允许自动执行
                                // 这里已经是WM_KEYDOWN with VK_RETURN，所以是用户按了回车键
                                LogToFile("  EditSubclassProc: 用户按回车键，执行第一个搜索结果");
                                ExecuteSelectedItem(firstSelIndex);
                            }
                            else if (g_windowInitializing)
                            {
                                LogToFile("  EditSubclassProc: 窗口初始化中，跳过自动执行");
                            }
                            else
                            {
                                // Check if the first item is "No matching items found"
                                if (wcscmp(firstItemText, L"No matching items found") == 0)
                                {
                                    LogToFile("  EditSubclassProc: First item is 'No matching items found' message, not executing");
                                }
                                else
                                {
                                    LogToFile("  EditSubclassProc: 搜索结果为空，不执行");
                                }
                            }
                        }
                    }
                    else
                    {
                        // If no items, process as command
                        if (wcslen(currentText) > 0)
                        {
                            sprintf(logMsg, "  EditSubclassProc: List empty, processing input as command: '%s'", currentTextLog);
                            LogToFile(logMsg);
                            
                            // 检查是否在计算模式、网址收藏模式或目录浏览模式，优先处理"q"退出命令
                            if ((g_calculatorMode || g_bookmarkMode || g_dirMode) && wcscmp(currentText, L"q") == 0)
                            {
                                LogToFile("  EditSubclassProc: 检测到特殊模式下输入'q'，退出当前模式");
                                if (g_calculatorMode)
                                {
                                    ExitCalculatorMode();
                                }
                                if (g_bookmarkMode)
                                {
                                    ExitBookmarkMode();
                                }
                                if (g_dirMode)
                                {
                                    ExitDirMode();
                                }
                                return 0; // 特殊模式退出处理完成
                            }
                            else
                            {
                                // 调用ProcessCommand处理特殊命令（包括set、help、js、wz等）
                                LogToFile("  EditSubclassProc: 调用ProcessCommand处理命令");
                                ProcessCommand(currentText);
                                
                                // 如果在计算模式下且不是"js"/"wz"命令，则调用EvaluateExpression
                                if (g_calculatorMode && 
                                    wcscmp(currentText, L"js") != 0 && 
                                    wcscmp(currentText, L"wz") != 0 &&
                                    wcscmp(currentText, L"q") != 0)
                                {
                                    LogToFile("  EditSubclassProc: 计算模式下，调用EvaluateExpression");
                                    EvaluateExpression(currentText);
                                }
                                // 如果在网址收藏模式下且不是"js"/"wz"/"q"命令，则搜索网址收藏
                                else if (g_bookmarkMode && 
                                         wcscmp(currentText, L"js") != 0 && 
                                         wcscmp(currentText, L"wz") != 0 &&
                                         wcscmp(currentText, L"q") != 0)
                                {
                                    LogToFile("  EditSubclassProc: 网址收藏模式下，搜索网址收藏");
                                    SearchBookmarks(currentText);
                                }
                            }
                        }
                        else
                        {
                            // 在计算模式下，如果输入为空，显示提示信息
                            if (g_calculatorMode)
                            {
                                LogToFile("  EditSubclassProc: 计算模式下空输入，显示帮助信息");
                                ShowCalculatorHelpInfo();
                            }
                            else
                            {
                                LogToFile("  EditSubclassProc: List empty and input text empty, no action taken");
                            }
                        }
                    }
                }
                return 0; // Message handled, no further processing needed
            }
            break;
            
        case WM_SETFOCUS:
            // 在计算模式下，允许文本框正常获得焦点，但记录状态
            if (g_calculatorMode)
            {
                LogToFile("EditSubclassProc: 计算模式下文本框获得焦点，允许正常处理");
                // 不阻止焦点处理，允许用户正常输入
            }
            // 不再自动执行程序，只有回车或双击listview时才执行
            break;

        case WM_KILLFOCUS:
            // 在计算模式下，允许焦点正常失去
            if (g_calculatorMode)
            {
                LogToFile("EditSubclassProc: 计算模式下文本框失去焦点，正常处理");
                // 不阻止焦点处理
            }
            break;
    }
    
    // For other messages, call the original edit control procedure
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// Enter calculator mode
void EnterCalculatorMode()
{
    LogToFile("EnterCalculatorMode: 进入计算模式");
    
    g_settingsMenuMode = false;
    // 设置计算模式标志
    g_calculatorMode = true;
    g_currentSearch[0] = L'\0';
    
    // 更新模式标签文本 - 已移除模式标签控件
    // SetWindowTextW(g_hModeLabel, L"计算:");
    
    // 不显示退出计算模式按钮，通过输入"q"退出
    // ShowWindow(g_hExitCalcButton, SW_SHOW);
    
    // 在计算模式下不显示Windows控件按钮，使用WebView2内的按钮
    // ShowWindow(g_hCalcMenuButton, SW_SHOW);
    
    // 在计算模式下保持设置按钮可见
    // ShowWindow(g_hSettingsButton, SW_HIDE);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");

    // 更新ListView列标题
    UpdateListViewColumns();

    // 清空之前的列表框内容
    ListView_DeleteAllItems(g_hListView);

    // 显示模式提示信息（多行）
    const WCHAR* hints[] = {
        L"💡 输入数学表达式",
        L"💡 按回车计算",
        L"💡 输入 q 退出计算模式"
    };
    AddMultiLineHintsToListView(hints, 3);

    // 显示计算历史记录
    DisplayCalculationHistory();

    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Exit calculator mode
void ExitCalculatorMode()
{
    LogToFile("ExitCalculatorMode: 退出计算模式");
    
    g_settingsMenuMode = false;
    // 清除计算模式标志
    g_calculatorMode = false;
    
    // 更新模式标签文本 - 已移除模式标签控件
    // SetWindowTextW(g_hModeLabel, L"搜索:");
    
    // 隐藏退出计算模式按钮
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    
    // 隐藏计算模式操作菜单按钮
    ShowWindow(g_hCalcMenuButton, SW_HIDE);
    
    // 设置按钮已移除，不再需要显示
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");

    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    SearchAndDisplayResults(L"");

    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// 显示计算模式帮助信息
void ShowCalculatorHelpInfo()
{
    LogToFile("ShowCalculatorHelpInfo: 显示计算模式帮助信息");
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    
    // 显示计算模式帮助信息
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    
    // 基本操作说明
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = (WCHAR*)L"计算模式帮助";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 1;
    lvi.pszText = (WCHAR*)L"输入数学表达式并按回车进行计算";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 2;
    lvi.pszText = (WCHAR*)L"支持运算符：+ - * / % ^";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 3;
    lvi.pszText = (WCHAR*)L"支持括号：()";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 4;
    lvi.pszText = (WCHAR*)L"常用函数：sin cos tan sqrt abs";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 5;
    lvi.pszText = (WCHAR*)L"输入 'q' 退出计算模式";
    ListView_InsertItem(g_hListView, &lvi);
    
    // 示例
    lvi.iItem = 6;
    lvi.pszText = (WCHAR*)L"";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 7;
    lvi.pszText = (WCHAR*)L"示例：";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 8;
    lvi.pszText = (WCHAR*)L"2+3*4";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 9;
    lvi.pszText = (WCHAR*)L"sqrt(16)";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 10;
    lvi.pszText = (WCHAR*)L"sin(30*3.14159/180)";
    ListView_InsertItem(g_hListView, &lvi);
    
    LogToFile("ShowCalculatorHelpInfo: 计算模式帮助信息显示完成");
    
    if (g_calculatorMode)
    {
        UpdateCalculatorModeWebView();
    }
}

// 显示使用帮助信息
void ShowHelpInfo()
{
    LogToFile("ShowHelpInfo: 显示使用帮助信息");
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    
    // 显示使用帮助信息
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    
    // 标题
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = (WCHAR*)L"使用帮助";
    ListView_InsertItem(g_hListView, &lvi);
    
    // 基本操作
    lvi.iItem = 1;
    lvi.pszText = (WCHAR*)L"基本操作：";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 2;
    lvi.pszText = (WCHAR*)L"在输入框中输入内容，按回车键执行";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 3;
    lvi.pszText = (WCHAR*)L"支持实时搜索和快捷启动";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 4;
    lvi.pszText = (WCHAR*)L"";
    ListView_InsertItem(g_hListView, &lvi);
    
    // 快捷命令
    lvi.iItem = 5;
    lvi.pszText = (WCHAR*)L"快捷命令：";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 6;
    lvi.pszText = (WCHAR*)L"help - 显示此帮助信息";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 7;
    lvi.pszText = (WCHAR*)L"set - 显示设置菜单";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 8;
    lvi.pszText = (WCHAR*)L"js - 切换到计算模式";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 9;
    lvi.pszText = (WCHAR*)L"wz - 切换到网址收藏模式";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 10;
    lvi.pszText = (WCHAR*)L"q - 退出现有模式";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 11;
    lvi.pszText = (WCHAR*)L"";
    ListView_InsertItem(g_hListView, &lvi);
    
    // 使用技巧
    lvi.iItem = 12;
    lvi.pszText = (WCHAR*)L"使用技巧：";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 13;
    lvi.pszText = (WCHAR*)L"双击列表项可执行对应操作";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 14;
    lvi.pszText = (WCHAR*)L"使用 Ctrl+Alt+Q 快速显示/隐藏窗口";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 15;
    lvi.pszText = (WCHAR*)L"使用 Ctrl+F1 将窗口定位到桌面中央";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 16;
    lvi.pszText = (WCHAR*)L"最小化窗口时会自动隐藏到系统托盘";
    ListView_InsertItem(g_hListView, &lvi);
    
    LogToFile("ShowHelpInfo: 使用帮助信息显示完成");
    
    // 更新WebView2显示帮助信息
    UpdateHelpInfoWebView();
}

// Enter bookmark mode
void EnterBookmarkMode()
{
    LogToFile("EnterBookmarkMode: 进入网址收藏模式");
    
    g_settingsMenuMode = false;
    // 设置网址收藏模式标志
    g_bookmarkMode = true;
    
    // 更新模式标签文本 - 已移除模式标签控件
    // SetWindowTextW(g_hModeLabel, L"网址:");
    
    // 隐藏退出网址收藏模式按钮（移除按钮，只保留"q"退出）
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    
    // 设置按钮已移除，不再需要隐藏
    
    // 隐藏退出计算模式按钮（如果显示）
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    
    // 显示列表框
    ShowWindow(g_hListView, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");

    // 更新ListView列标题
    UpdateListViewColumns();

    // 清空之前的列表框内容
    ListView_DeleteAllItems(g_hListView);
    
    // 显示模式提示信息（多行）
    const WCHAR* hints[] = {
        L"💡 搜索或浏览收藏的网址",
        L"💡 按回车或双击打开",
        L"💡 输入 q 退出网址收藏模式"
    };
    AddMultiLineHintsToListView(hints, 3);
    
    // 加载并显示网址收藏
    LoadBookmarks();
    
    // 清空搜索查询，显示所有网址
    g_currentSearch[0] = L'\0';
    g_bookmarkSearchResults.clear();
    
    DisplayBookmarkResults();
    
    // 更新WebView2显示
    UpdateBookmarkModeWebView();
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Exit bookmark mode
void ExitBookmarkMode()
{
    LogToFile("ExitBookmarkMode: 退出网址收藏模式");
    
    g_settingsMenuMode = false;
    // 清除网址收藏模式标志
    g_bookmarkMode = false;
    
    // 更新模式标签文本 - 已移除模式标签控件
    // SetWindowTextW(g_hModeLabel, L"搜索:");
    
    // 隐藏退出网址收藏模式按钮（保持隐藏状态，只使用"q"退出）
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    
    // 显示设置按钮
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");

    // 更新ListView列标题
    UpdateListViewColumns();

    // 清空列表框
    ListView_DeleteAllItems(g_hListView);

    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// 添加网址收藏
void AddBookmark(const WCHAR* name, const WCHAR* url)
{
    LogToFile("AddBookmark: 添加网址收藏");
    
    // 验证URL格式
    if (!IsURL(url))
    {
        LogToFile("AddBookmark: URL格式无效");
        MessageBoxW(g_hMainWindow, L"请输入有效的网址", L"添加网址失败", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 检查是否已存在相同的网址
    for (const auto& bookmark : g_bookmarks)
    {
        if (bookmark.second == url)
        {
            LogToFile("AddBookmark: 网址已存在");
            MessageBoxW(g_hMainWindow, L"该网址已存在于收藏中", L"添加网址失败", MB_OK | MB_ICONWARNING);
            return;
        }
    }
    
    // 添加到收藏列表
    g_bookmarks.push_back(std::make_pair(std::wstring(name), std::wstring(url)));
    
    // 保存到文件
    SaveBookmarks();
    
    // 刷新显示
    DisplayBookmarkResults();
    
    LogToFile("AddBookmark: 网址收藏添加成功");
}

// 删除网址收藏
void DeleteBookmark(int index)
{
    LogToFile("DeleteBookmark: 删除网址收藏");
    
    if (index < 0 || index >= static_cast<int>(g_bookmarks.size()))
    {
        LogToFile("DeleteBookmark: 索引超出范围");
        return;
    }
    
    // 删除指定索引的网址收藏
    g_bookmarks.erase(g_bookmarks.begin() + index);
    
    // 保存到文件
    SaveBookmarks();
    
    // 刷新显示
    DisplayBookmarkResults();
    
    LogToFile("DeleteBookmark: 网址收藏删除成功");
}

// 保存网址收藏到文件
void SaveBookmarks()
{
    LogToFile("SaveBookmarks: 开始保存网址收藏");
    
    // 创建数据目录（如果不存在）
    CreateDirectoryW(L"data", NULL);
    
    // 打开网址收藏文件
    FILE* file = _wfopen(L"data\\bookmarks.txt", L"w, ccs=UTF-8");
    if (!file)
    {
        LogToFile("SaveBookmarks: 无法打开网址收藏文件进行写入");
        return;
    }
    
    // 写入网址收藏
    for (const auto& bookmark : g_bookmarks)
    {
        // 格式：名称|URL
        fwprintf(file, L"%s|%s\n", bookmark.first.c_str(), bookmark.second.c_str());
    }
    
    fclose(file);
    
    // 记录保存的网址收藏数量
    char logMsg[200] = {0};
    sprintf(logMsg, "SaveBookmarks: 保存了 %zu 条网址收藏", g_bookmarks.size());
    LogToFile(logMsg);
    LogToFile("SaveBookmarks: 函数结束");
}

// 从文件加载网址收藏
void LoadBookmarks()
{
    LogToFile("LoadBookmarks: 开始加载网址收藏");
    
    try
    {
        // 检查数据目录是否存在
        DWORD dwAttrib = GetFileAttributesW(L"data");
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            LogToFile("LoadBookmarks: 数据目录不存在，创建目录");
            CreateDirectoryW(L"data", NULL);
        }
        
        // 检查网址收藏文件是否存在
        dwAttrib = GetFileAttributesW(L"data\\bookmarks.txt");
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            LogToFile("LoadBookmarks: 网址收藏文件不存在，可能是首次运行");
            return;
        }
        
        // 打开网址收藏文件
        LogToFile("LoadBookmarks: 尝试打开网址收藏文件");
        FILE* file = _wfopen(L"data\\bookmarks.txt", L"r, ccs=UTF-8");
        if (!file)
        {
            LogToFile("LoadBookmarks: 无法打开网址收藏文件进行读取，可能是首次运行");
            return;
        }
        
        LogToFile("LoadBookmarks: 成功打开网址收藏文件");
        
        // 检查文件是否为空
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        if (fileSize == 0)
        {
            LogToFile("LoadBookmarks: 文件为空，无需加载");
            fclose(file);
            return;
        }
        
        char sizeLog[100] = {0};
        sprintf(sizeLog, "LoadBookmarks: 文件大小为 %ld 字节", fileSize);
        LogToFile(sizeLog);
        
        // 清空当前网址收藏
        g_bookmarks.clear();
        LogToFile("LoadBookmarks: 已清空当前网址收藏");
        
        // 读取网址收藏
        WCHAR buffer[2048];
        int lineCount = 0;
        
        while (fgetws(buffer, sizeof(buffer)/sizeof(WCHAR), file))
        {
            lineCount++;
            
            // 移除换行符
            size_t len = wcslen(buffer);
            if (len > 0 && buffer[len - 1] == L'\n')
            {
                buffer[len - 1] = L'\0';
                len--;
            }
            
            // 跳过空行
            if (len == 0)
            {
                LogToFile("LoadBookmarks: 跳过空行");
                continue;
            }
            
            // 解析格式：名称|URL
            WCHAR* separator = wcschr(buffer, L'|');
            if (!separator)
            {
                LogToFile("LoadBookmarks: 跳过格式不正确的行");
                continue;
            }
            
            // 分割名称和URL
            *separator = L'\0';
            WCHAR* name = buffer;
            WCHAR* url = separator + 1;
            
            // 添加到网址收藏列表
            g_bookmarks.push_back(std::make_pair(std::wstring(name), std::wstring(url)));
            
            // 记录每行读取的内容（仅前5行）
            if (lineCount <= 5)
            {
                char lineLog[2200] = {0};
                WideCharToMultiByte(CP_UTF8, 0, buffer, -1, lineLog, sizeof(lineLog), NULL, NULL);
                LogToFile(lineLog);
            }
        }
        
        fclose(file);
        LogToFile("LoadBookmarks: 已关闭网址收藏文件");
        
        // 记录加载的网址收藏数量
        char logMsg[200] = {0};
        sprintf(logMsg, "LoadBookmarks: 加载了 %zu 条网址收藏，共读取 %d 行", g_bookmarks.size(), lineCount);
        LogToFile(logMsg);
        LogToFile("LoadBookmarks: 函数结束");
    }
    catch (...)
    {
        LogToFile("LoadBookmarks: 加载网址收藏时发生异常");
    }
}

// 显示网址收藏
void DisplayBookmarkResults()
{
    LogToFile("DisplayBookmarkResults: 显示网址收藏");
    
    // 暂停列表视图重绘以提高性能
    SendMessageW(g_hListView, WM_SETREDRAW, FALSE, 0);
    
    // 清空列表视图
    ListView_DeleteAllItems(g_hListView);
    
    // 使用搜索结果（如果有）或全部网址收藏
    const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
    
    // 添加网址收藏到列表视图（双列显示：名称和URL）
    for (const auto& bookmark : displayBookmarks)
    {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;  // 插入到顶部
        
        // 第一列：名称
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(bookmark.first.c_str());
        ListView_InsertItem(g_hListView, &lvi);
        
        // 第二列：URL
        lvi.iSubItem = 1;
        lvi.pszText = const_cast<LPWSTR>(bookmark.second.c_str());
        ListView_SetItem(g_hListView, &lvi);
    }
    
    // 恢复列表视图重绘
    SendMessageW(g_hListView, WM_SETREDRAW, TRUE, 0);
    
    // 强制重绘列表视图
    InvalidateRect(g_hListView, NULL, TRUE);
    
    // 记录显示的网址收藏数量
    char logMsg[200] = {0};
    sprintf(logMsg, "DisplayBookmarkResults: 显示了 %zu 条网址收藏", displayBookmarks.size());
    LogToFile(logMsg);
    
    // 更新WebView2显示
    UpdateBookmarkModeWebView();
}

// 检查字符串是否为有效的URL
bool IsURL(const WCHAR* str)
{
    if (!str || wcslen(str) < 4)
    {
        return false;
    }
    
    // 检查是否以http://或https://开头
    if (wcsncmp(str, L"http://", 7) == 0 || wcsncmp(str, L"https://", 8) == 0)
    {
        return true;
    }
    
    // 检查是否以www.开头
    if (wcsncmp(str, L"www.", 4) == 0)
    {
        return true;
    }
    
    // 检查是否包含点号（简单的域名检查）
    if (wcschr(str, L'.') != NULL)
    {
        return true;
    }
    
    return false;
}

// 搜索网址收藏
void SearchBookmarks(const WCHAR* query)
{
    LogToFile("SearchBookmarks: 搜索网址收藏");
    
    // 清空搜索结果
    g_bookmarkSearchResults.clear();
    
    // 如果查询为空，显示所有网址收藏
    if (!query || wcslen(query) == 0)
    {
        // 清空搜索结果，显示所有网址
        g_bookmarkSearchResults.clear();
        DisplayBookmarkResults();
        return;
    }
    
    // 转换查询为小写以进行不区分大小写的搜索
    std::wstring lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
    
    // 搜索匹配的网址收藏
    for (const auto& bookmark : g_bookmarks)
    {
        // 转换名称和URL为小写
        std::wstring lowerName = bookmark.first;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
        
        std::wstring lowerUrl = bookmark.second;
        std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), ::towlower);
        
        // 检查名称或URL是否包含查询字符串
        if (lowerName.find(lowerQuery) != std::wstring::npos || 
            lowerUrl.find(lowerQuery) != std::wstring::npos)
        {
            g_bookmarkSearchResults.push_back(bookmark);
        }
    }
    
    // 显示搜索结果
    DisplayBookmarkResults();
    
    // 记录搜索结果数量
    char logMsg[200] = {0};
    sprintf(logMsg, "SearchBookmarks: 找到 %zu 条匹配的网址收藏", g_bookmarkSearchResults.size());
    LogToFile(logMsg);
}

// 同步Chrome书签
void SyncChromeBookmarks()
{
    LogToFile("SyncChromeBookmarks: 开始同步Chrome书签");
    
    // Chrome书签文件路径
    WCHAR bookmarksPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, bookmarksPath)))
    {
        wcscat_s(bookmarksPath, L"\\Google\\Chrome\\User Data\\Default\\Bookmarks");
        
        // 检查文件是否存在
        DWORD dwAttrib = GetFileAttributesW(bookmarksPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            LogToFile("SyncChromeBookmarks: Chrome书签文件不存在");
            MessageBoxW(g_hMainWindow, L"未找到Chrome书签文件，请确保Chrome已安装并至少添加过一个书签", L"同步失败", MB_OK | MB_ICONERROR);
            return;
        }
        
        // 打开Chrome书签文件（JSON格式）
        FILE* file = _wfopen(bookmarksPath, L"r, ccs=UTF-8");
        if (!file)
        {
            LogToFile("SyncChromeBookmarks: 无法打开Chrome书签文件");
            MessageBoxW(g_hMainWindow, L"无法读取Chrome书签文件，请确保Chrome已关闭", L"同步失败", MB_OK | MB_ICONERROR);
            return;
        }
        
        // 读取文件内容
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char* buffer = new char[fileSize + 1];
        fread(buffer, 1, fileSize, file);
        buffer[fileSize] = '\0';
        fclose(file);
        
        // 简单解析JSON（实际应用中应使用专门的JSON解析库）
        // 这里只做简单的字符串匹配，提取书签名称和URL
        std::string content(buffer);
        delete[] buffer;
        
        // 查找书签条目
        size_t pos = 0;
        int addedCount = 0;
        
        while ((pos = content.find("\"name\":", pos)) != std::string::npos)
        {
            // 提取名称
            pos += 8; // 跳过"name":
            while (pos < content.length() && isspace(content[pos])) pos++;
            if (pos >= content.length() || content[pos] != '"') continue;
            pos++; // 跳过开始的引号
            
            size_t nameEnd = content.find("\"", pos);
            if (nameEnd == std::string::npos) continue;
            std::string name = content.substr(pos, nameEnd - pos);
            pos = nameEnd + 1;
            
            // 查找URL
            size_t urlPos = content.find("\"url\":", pos);
            if (urlPos == std::string::npos) continue;
            urlPos += 7; // 跳过"url":
            while (urlPos < content.length() && isspace(content[urlPos])) urlPos++;
            if (urlPos >= content.length() || content[urlPos] != '"') continue;
            urlPos++; // 跳过开始的引号
            
            size_t urlEnd = content.find("\"", urlPos);
            if (urlEnd == std::string::npos) continue;
            std::string url = content.substr(urlPos, urlEnd - urlPos);
            pos = urlEnd + 1;
            
            // 跳过文件夹（没有URL的条目）
            if (url.empty()) continue;
            
            // 转换为宽字符
            int nameLen = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, NULL, 0);
            int urlLen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, NULL, 0);
            
            WCHAR* wName = new WCHAR[nameLen];
            WCHAR* wUrl = new WCHAR[urlLen];
            
            MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wName, nameLen);
            MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wUrl, urlLen);
            
            // 检查是否已存在
            bool exists = false;
            for (const auto& bookmark : g_bookmarks)
            {
                if (bookmark.second == wUrl)
                {
                    exists = true;
                    break;
                }
            }
            
            // 如果不存在，则添加
            if (!exists)
            {
                g_bookmarks.push_back(std::make_pair(std::wstring(wName), std::wstring(wUrl)));
                addedCount++;
            }
            
            delete[] wName;
            delete[] wUrl;
        }
        
        // 保存到文件
        SaveBookmarks();
        
        // 刷新显示
        DisplayBookmarkResults();
        
        // 显示结果
        WCHAR msg[200];
        swprintf(msg, sizeof(msg)/sizeof(WCHAR), L"成功从Chrome同步了 %d 个新书签", addedCount);
        MessageBoxW(g_hMainWindow, msg, L"同步完成", MB_OK | MB_ICONINFORMATION);
        
        // 记录同步结果
        char logMsg[200] = {0};
        sprintf(logMsg, "SyncChromeBookmarks: 同步完成，添加了 %d 个新书签", addedCount);
        LogToFile(logMsg);
    }
    else
    {
        LogToFile("SyncChromeBookmarks: 无法获取用户数据目录");
        MessageBoxW(g_hMainWindow, L"无法获取用户数据目录", L"同步失败", MB_OK | MB_ICONERROR);
    }
}

// Evaluate mathematical expression
void EvaluateExpression(const WCHAR* expression)
{
    LogToFile("EvaluateExpression: 函数开始");
    
    if (!expression || wcslen(expression) == 0)
    {
        LogToFile("EvaluateExpression: 表达式为空");
        return;
    }
    
    // 记录表达式
    char exprLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, expression, -1, exprLog, sizeof(exprLog), NULL, NULL);
    char logMsg[1100] = {0};
    sprintf(logMsg, "EvaluateExpression: 计算表达式 '%s'", exprLog);
    LogToFile(logMsg);
    
    // 这里应该实现表达式计算逻辑
    // 为了简单起见，我们只实现基本的加减乘除
    // 在实际应用中，可以使用更复杂的表达式解析器
    
    try
    {
        LogToFile("EvaluateExpression: 进入try块");
        
        // 将表达式转换为字符串以便处理
        std::wstring expr = expression;
        LogToFile("EvaluateExpression: 创建了wstring表达式");
        
        // 检查并提取注释内容（#后面的内容）
        std::wstring comment;
        size_t hashPos = expr.find(L'#');
        if (hashPos != std::wstring::npos) {
            comment = expr.substr(hashPos + 1);
            expr = expr.substr(0, hashPos);
            // 移除注释前后的空格
            size_t start = comment.find_first_not_of(L' ');
            size_t end = comment.find_last_not_of(L' ');
            if (start != std::wstring::npos && end != std::wstring::npos) {
                comment = comment.substr(start, end - start + 1);
            } else {
                comment = L"";
            }
            
            char commentLog[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, comment.c_str(), -1, commentLog, sizeof(commentLog), NULL, NULL);
            sprintf(logMsg, "EvaluateExpression: 提取到注释 '%s'", commentLog);
            LogToFile(logMsg);
        }
        
        // 检查表达式中是否包含等号，如果包含则只取等号前的部分
        size_t equalPos = expr.find(L'=');
        if (equalPos != std::wstring::npos) {
            expr = expr.substr(0, equalPos);
            char trimmedLog[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, expr.c_str(), -1, trimmedLog, sizeof(trimmedLog), NULL, NULL);
            sprintf(logMsg, "EvaluateExpression: 发现等号，截取表达式为 '%s'", trimmedLog);
            LogToFile(logMsg);
        }
        
        // 移除空格
        expr.erase(std::remove(expr.begin(), expr.end(), L' '), expr.end());
        LogToFile("EvaluateExpression: 移除了空格");
        
        // 简单的表达式计算（这里只是示例，实际应该使用更健壮的方法）
        double result = 0.0;
        bool success = false;
        
        // 尝试解析为数字
        try
        {
            LogToFile("EvaluateExpression: 尝试解析为数字");
            
            // 检查表达式是否只包含数字和小数点
            bool isPureNumber = true;
            for (wchar_t c : expr) {
                if (!isdigit(c) && c != L'.' && c != L'-') {
                    isPureNumber = false;
                    break;
                }
            }
            
            if (isPureNumber) {
                result = std::stod(expr);
                success = true;
                LogToFile("EvaluateExpression: 成功解析为单个数字");
            } else {
                LogToFile("EvaluateExpression: 表达式包含非数字字符，尝试解析表达式");
                throw std::exception(); // 强制进入表达式解析逻辑
            }
        }
        catch (...)
        {
            LogToFile("EvaluateExpression: 不是单个数字，尝试解析表达式");
            // 不是简单的数字，需要更复杂的解析
            // 使用递归下降法解析表达式，支持多个运算符
            
            try {
                size_t pos = 0;
                result = parseExpression(expr, pos);
                success = true;
                
                char resultLog[256] = {0};
                sprintf(resultLog, "EvaluateExpression: 表达式计算结果为 %f", result);
                LogToFile(resultLog);
            } catch (...) {
                LogToFile("EvaluateExpression: 表达式解析失败");
                success = false;
            }
        }
        
        LogToFile("EvaluateExpression: 表达式解析完成");
        
        if (success)
        {
            LogToFile("EvaluateExpression: 开始处理成功结果");
            
            // 创建结果字符串
            WCHAR resultStr[256] = {0};
            swprintf(resultStr, sizeof(resultStr)/sizeof(WCHAR), L"%.6g", result);
            LogToFile("EvaluateExpression: 创建了结果字符串");
            
            // 创建历史记录条目（只使用去除注释的表达式）
            std::wstring displayExpr = expr;  // 使用去除注释的表达式
            displayExpr += L" = ";
            displayExpr += resultStr;
            LogToFile("EvaluateExpression: 创建了历史记录条目");
            
            // 创建历史记录结构体，包含完整表达式（表达式+结果）和注释
            CalculationRecord record;
            record.expression = displayExpr;  // 使用包含结果的完整表达式（不包含注释）
            record.result = resultStr;
            record.comment = comment;
            LogToFile("EvaluateExpression: 创建了计算记录结构体");
            
            // 添加到计算历史
            g_calculationHistory.push_back(record);
            LogToFile("EvaluateExpression: 添加到历史记录");
            
            // 限制历史记录数量
            if (g_calculationHistory.size() > 50)
            {
                g_calculationHistory.erase(g_calculationHistory.begin());
            }
            LogToFile("EvaluateExpression: 检查了历史记录数量");
            
            // 保存计算历史到文件
            SaveCalculationHistory();
            LogToFile("EvaluateExpression: 保存了计算历史到文件");
            
            // 显示计算历史
            LogToFile("EvaluateExpression: 准备显示计算历史");
            DisplayCalculationHistory();
            LogToFile("EvaluateExpression: 显示了计算历史");
            
            // 记录结果
            char resultLog[256] = {0};
            WideCharToMultiByte(CP_UTF8, 0, resultStr, -1, resultLog, sizeof(resultLog), NULL, NULL);
            sprintf(logMsg, "EvaluateExpression: 计算结果为 %s", resultLog);
            LogToFile(logMsg);
            
            // 将结果复制到编辑框
            LogToFile("EvaluateExpression: 准备设置编辑框文本");
            g_updatingEditBox = true;  // 设置标志，防止触发EN_CHANGE
            SetWindowTextW(g_hEdit, resultStr);
            g_updatingEditBox = false; // 清除标志
            LogToFile("EvaluateExpression: 设置了编辑框文本");
            
            LogToFile("EvaluateExpression: 准备选择编辑框文本");
            SendMessageW(g_hEdit, EM_SETSEL, 0, -1); // 全选文本
            LogToFile("EvaluateExpression: 选择了编辑框文本");
            
            LogToFile("EvaluateExpression: 成功处理结果");
        }
        else
        {
            LogToFile("EvaluateExpression: 表达式计算失败");
            MessageBoxW(g_hMainWindow, L"无法计算表达式", L"计算错误", MB_OK | MB_ICONERROR);
        }
    }
    catch (...)
    {
        LogToFile("EvaluateExpression: 表达式计算异常");
        MessageBoxW(g_hMainWindow, L"表达式计算异常", L"计算错误", MB_OK | MB_ICONERROR);
    }
    
    LogToFile("EvaluateExpression: 函数结束");
}

// Display calculation history
void DisplayCalculationHistory()
{
    LogToFile("DisplayCalculationHistory: 开始显示计算历史");
    
    // 记录当前历史记录状态
    char histLog[200] = {0};
    sprintf(histLog, "DisplayCalculationHistory: 当前有 %zu 条历史记录", g_calculationHistory.size());
    LogToFile(histLog);
    
    // 暂停列表视图重绘以提高性能
    LogToFile("DisplayCalculationHistory: 暂停ListView重绘");
    SendMessageW(g_hListView, WM_SETREDRAW, FALSE, 0);
    
    // 清空列表视图
    LogToFile("DisplayCalculationHistory: 清空ListView");
    ListView_DeleteAllItems(g_hListView);
    
    // 添加历史记录到列表视图（最新的在顶部）
    // ListView第0行显示最新记录，第1行显示第二新记录，以此类推
    for (size_t i = 0; i < g_calculationHistory.size(); ++i)
    {
        // 从最新记录开始显示
        const auto& record = g_calculationHistory[g_calculationHistory.size() - 1 - i];
        
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;  // 插入到对应位置
        
        // 第一列：表达式
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(record.expression.c_str());
        ListView_InsertItem(g_hListView, &lvi);
        
        // 第二列：注释（标签）
        lvi.iSubItem = 1;
        lvi.pszText = const_cast<LPWSTR>(record.comment.c_str());
        ListView_SetItem(g_hListView, &lvi);
        
        // 第三列：结果
        lvi.iSubItem = 2;
        lvi.pszText = const_cast<LPWSTR>(record.result.c_str());
        ListView_SetItem(g_hListView, &lvi);
        
        // 添加详细日志
        char expr[200] = {0};
        char result[200] = {0};
        WideCharToMultiByte(CP_ACP, 0, record.expression.c_str(), -1, expr, sizeof(expr), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, record.result.c_str(), -1, result, sizeof(result), NULL, NULL);
        
        char displayLog[400] = {0};
        sprintf(displayLog, "DisplayCalculationHistory: g_calculationHistory[%zu] -> ListView第%zu行: %s = %s", 
            g_calculationHistory.size() - 1 - i, i, expr, result);
        LogToFile(displayLog);
    }
    
    // 恢复列表视图重绘
    LogToFile("DisplayCalculationHistory: 恢复ListView重绘");
    SendMessageW(g_hListView, WM_SETREDRAW, TRUE, 0);
    
    // 强制重绘列表视图
    LogToFile("DisplayCalculationHistory: 强制重绘ListView");
    InvalidateRect(g_hListView, NULL, TRUE);
    SendMessageW(g_hListView, WM_PAINT, 0, 0);
    UpdateWindow(g_hListView);
    
    // 记录显示的历史记录数量
    char logMsg[200] = {0};
    sprintf(logMsg, "DisplayCalculationHistory: 显示完成，共 %zu 条历史记录", g_calculationHistory.size());
    LogToFile(logMsg);
    
    // 验证ListView项目数量
    int actualCount = ListView_GetItemCount(g_hListView);
    char verifyLog[200] = {0};
    sprintf(verifyLog, "DisplayCalculationHistory: ListView实际项目数 = %d", actualCount);
    LogToFile(verifyLog);
    
    if (g_calculatorMode)
    {
        UpdateCalculatorModeWebView();
    }
}

// 保存计算历史到文件
void SaveCalculationHistory()
{
    LogToFile("SaveCalculationHistory: 开始保存计算历史");
    
    // 创建数据目录（如果不存在）
    CreateDirectoryW(L"data", NULL);
    
    // 打开历史文件
    FILE* file = _wfopen(L"data\\calculation_history.txt", L"w, ccs=UTF-8");
    if (!file)
    {
        LogToFile("SaveCalculationHistory: 无法打开历史文件进行写入");
        return;
    }
    
    // 写入历史记录（格式：表达式[TAB]结果[TAB]注释）
    for (const auto& record : g_calculationHistory)
    {
        // 使用制表符分隔三个字段
        fwprintf(file, L"%s\t%s\t%s\n", record.expression.c_str(), record.result.c_str(), record.comment.c_str());
    }
    
    fclose(file);
    
    // 记录保存的历史记录数量
    char logMsg[200] = {0};
    sprintf(logMsg, "SaveCalculationHistory: 保存了 %zu 条历史记录", g_calculationHistory.size());
    LogToFile(logMsg);
    LogToFile("SaveCalculationHistory: 函数结束");
}

// 从文件加载计算历史
void LoadCalculationHistory()
{
    LogToFile("LoadCalculationHistory: 开始加载计算历史");
    
    try
    {
        // 检查数据目录是否存在
        DWORD dwAttrib = GetFileAttributesW(L"data");
        if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            LogToFile("LoadCalculationHistory: 数据目录不存在，创建目录");
            CreateDirectoryW(L"data", NULL);
        }
        
        // 检查历史文件是否存在
        dwAttrib = GetFileAttributesW(L"data\\calculation_history.txt");
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            LogToFile("LoadCalculationHistory: 历史文件不存在，可能是首次运行");
            return;
        }
        
        // 打开历史文件
        LogToFile("LoadCalculationHistory: 尝试打开历史文件");
        FILE* file = _wfopen(L"data\\calculation_history.txt", L"r, ccs=UTF-8");
        if (!file)
        {
            LogToFile("LoadCalculationHistory: 无法打开历史文件进行读取，可能是首次运行");
            return;
        }
        
        LogToFile("LoadCalculationHistory: 成功打开历史文件");
        
        // 检查文件是否为空
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        if (fileSize == 0)
        {
            LogToFile("LoadCalculationHistory: 文件为空，无需加载");
            fclose(file);
            return;
        }
        
        char sizeLog[100] = {0};
        sprintf(sizeLog, "LoadCalculationHistory: 文件大小为 %ld 字节", fileSize);
        LogToFile(sizeLog);
        
        // 清空当前历史记录
        g_calculationHistory.clear();
        LogToFile("LoadCalculationHistory: 已清空当前历史记录");
        
        // 读取历史记录
        WCHAR buffer[1024];
        int lineCount = 0;
        
        while (fgetws(buffer, sizeof(buffer)/sizeof(WCHAR), file))
        {
            lineCount++;
            
            // 移除换行符
            size_t len = wcslen(buffer);
            if (len > 0 && buffer[len - 1] == L'\n')
            {
                buffer[len - 1] = L'\0';
                len--;
            }
            
            // 跳过空行
            if (len == 0)
            {
                LogToFile("LoadCalculationHistory: 跳过空行");
                continue;
            }
            
            // 解析记录格式：表达式[TAB]结果[TAB]注释
            CalculationRecord record;
            
            // 找到第一个制表符
            WCHAR* tab1 = wcschr(buffer, L'\t');
            if (tab1) {
                *tab1 = L'\0';
                
                // 找到第二个制表符
                WCHAR* tab2 = wcschr(tab1 + 1, L'\t');
                if (tab2) {
                    *tab2 = L'\0';
                    record.comment = tab2 + 1;
                } else {
                    record.comment = L"";
                }
                
                record.expression = buffer;
                record.result = tab1 + 1;
            } else {
                // 兼容旧格式：如果没有制表符，整个字符串作为表达式
                record.expression = buffer;
                record.result = L"";
                record.comment = L"";
            }
            
            // 添加到历史记录
            g_calculationHistory.push_back(record);
            
            // 记录每行读取的内容（仅前5行）
            if (lineCount <= 5)
            {
                char lineLog[1100] = {0};
                WideCharToMultiByte(CP_ACP, 0, buffer, -1, lineLog, sizeof(lineLog), NULL, NULL);
                LogToFile(lineLog);
            }
        }
        
        fclose(file);
        LogToFile("LoadCalculationHistory: 已关闭历史文件");
        
        // 记录加载的历史记录数量
        char logMsg[200] = {0};
        sprintf(logMsg, "LoadCalculationHistory: 加载了 %zu 条历史记录，共读取 %d 行", g_calculationHistory.size(), lineCount);
        LogToFile(logMsg);
        LogToFile("LoadCalculationHistory: 函数结束");
    }
    catch (...)
    {
        LogToFile("LoadCalculationHistory: 加载计算历史时发生异常");
    }
}

// 网址管理对话框实现

// 显示网址管理对话框
void ShowBookmarkDialog()
{
    LogToFile("ShowBookmarkDialog: 显示网址管理对话框");
    
    // 先加载网址收藏
    LoadBookmarks();
    
    // 使用更简单的方法创建对话框
    // 创建一个模态对话框
    INT_PTR result = DialogBox(
        g_hInstance,
        MAKEINTRESOURCE(IDD_BOOKMARK_DIALOG),
        g_hMainWindow,
        BookmarkDialogProc
    );
    
    if (result == -1 || result == -2)
    {
        LogToFile("ShowBookmarkDialog: 无法创建对话框");
        char errorMsg[256] = {0};
        sprintf(errorMsg, "ShowBookmarkDialog: 错误代码 %d", GetLastError());
        LogToFile(errorMsg);
    }
    else
    {
        LogToFile("ShowBookmarkDialog: 对话框已关闭");
    }
}

// 网址管理对话框过程
INT_PTR CALLBACK BookmarkDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LogToFile("BookmarkDialogProc: WM_INITDIALOG");
            
            // 加载网址收藏
            LoadBookmarks();
            
            // 设置窗口标题
            SetWindowTextW(hwnd, L"网址收藏管理");
            
            // 动态设置按钮文本，确保中文显示正确
            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_ADD), L"添加");
            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_UPDATE), L"更新");
            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_DELETE), L"删除");
            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_CLOSE), L"关闭");
            
            // 动态设置标签文本，确保中文显示正确
            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_NAME_LABEL), L"名称:");
            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_URL_LABEL), L"URL:");
            
            // 刷新网址列表
            RefreshBookmarkList(GetDlgItem(hwnd, IDC_BOOKMARK_LIST));
            
            // 设置焦点到列表框
            SetFocus(GetDlgItem(hwnd, IDC_BOOKMARK_LIST));
            
            return TRUE;
        }
        
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BOOKMARK_LIST:
                {
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        // 获取选中的网址
                        HWND hList = GetDlgItem(hwnd, IDC_BOOKMARK_LIST);
                        int selIndex = (int)ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
                        
                        if (selIndex != -1 && selIndex < (int)g_bookmarks.size())
                        {
                            // 在编辑框中显示选中的网址信息
                            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_NAME), g_bookmarks[selIndex].first.c_str());
                            SetWindowTextW(GetDlgItem(hwnd, IDC_BOOKMARK_URL), g_bookmarks[selIndex].second.c_str());
                        }
                    }
                    return TRUE;
                }
                
                case IDC_BOOKMARK_ADD:
                {
                    AddBookmarkFromDialog(hwnd);
                    return TRUE;
                }
                
                case IDC_BOOKMARK_UPDATE:
                {
                    UpdateBookmarkFromDialog(hwnd);
                    return TRUE;
                }
                
                case IDC_BOOKMARK_DELETE:
                {
                    DeleteBookmarkFromDialog(hwnd);
                    return TRUE;
                }
                
                case IDC_BOOKMARK_CLOSE:
                {
                    EndDialog(hwnd, IDOK);
                    // 关闭对话框后进入网址收藏模式
                    EnterBookmarkMode();
                    return TRUE;
                }
            }
            break;
        }
        
        case WM_CLOSE:
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        
        case WM_DESTROY:
        {
            LogToFile("BookmarkDialogProc: WM_DESTROY");
            return TRUE;
        }
    }
    
    return FALSE;
}

// 刷新网址列表
void RefreshBookmarkList(HWND hList)
{
    LogToFile("RefreshBookmarkList: 刷新网址列表");
    
    if (!hList)
    {
        LogToFile("RefreshBookmarkList: 列表框句柄为空");
        return;
    }
    
    // 暂停列表框重绘以提高性能
    SendMessageW(hList, WM_SETREDRAW, FALSE, 0);
    
    // 清空列表框
    ListView_DeleteAllItems(hList);
    
    // 添加网址到列表框
    for (const auto& bookmark : g_bookmarks)
    {
        // 创建显示字符串：名称 - URL
        std::wstring displayStr = bookmark.first + L" - " + bookmark.second;
        LVITEMW lvItem = {0};
        lvItem.iItem = (int)ListView_GetItemCount(hList);
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPWSTR)displayStr.c_str();
        ListView_InsertItem(hList, &lvItem);
    }
    
    // 恢复列表框重绘
    SendMessageW(hList, WM_SETREDRAW, TRUE, 0);
    
    // 强制重绘列表框
    InvalidateRect(hList, NULL, TRUE);
    
    // 记录刷新的网址数量
    char logMsg[200] = {0};
    sprintf(logMsg, "RefreshBookmarkList: 刷新了 %zu 个网址", g_bookmarks.size());
    LogToFile(logMsg);
}

// 从对话框添加网址
void AddBookmarkFromDialog(HWND hDlg)
{
    LogToFile("AddBookmarkFromDialog: 从对话框添加网址");
    
    // 获取名称和URL
    WCHAR name[256] = {0};
    WCHAR url[1024] = {0};
    
    GetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_NAME), name, sizeof(name)/sizeof(WCHAR));
    GetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_URL), url, sizeof(url)/sizeof(WCHAR));
    
    // 检查输入
    if (wcslen(name) == 0 || wcslen(url) == 0)
    {
        MessageBoxW(hDlg, L"请输入名称和URL", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("AddBookmarkFromDialog: 名称或URL为空");
        return;
    }
    
    // 检查URL格式
    if (!IsURL(url))
    {
        MessageBoxW(hDlg, L"请输入有效的URL", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("AddBookmarkFromDialog: URL格式无效");
        return;
    }
    
    // 添加网址
    AddBookmark(name, url);
    
    // 刷新列表
    RefreshBookmarkList(GetDlgItem(hDlg, IDC_BOOKMARK_LIST));
    
    // 清空编辑框
    SetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_NAME), L"");
    SetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_URL), L"");
    
    // 设置焦点到名称编辑框
    SetFocus(GetDlgItem(hDlg, IDC_BOOKMARK_NAME));
    
    LogToFile("AddBookmarkFromDialog: 网址添加成功");
}

// 从对话框更新网址
void UpdateBookmarkFromDialog(HWND hDlg)
{
    LogToFile("UpdateBookmarkFromDialog: 从对话框更新网址");
    
    // 获取选中的网址索引
    HWND hList = GetDlgItem(hDlg, IDC_BOOKMARK_LIST);
    int selIndex = (int)ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
    
    if (selIndex == -1 || selIndex >= (int)g_bookmarks.size())
    {
        MessageBoxW(hDlg, L"请选择要更新的网址", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("UpdateBookmarkFromDialog: 未选择网址");
        return;
    }
    
    // 获取名称和URL
    WCHAR name[256] = {0};
    WCHAR url[1024] = {0};
    
    GetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_NAME), name, sizeof(name)/sizeof(WCHAR));
    GetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_URL), url, sizeof(url)/sizeof(WCHAR));
    
    // 检查输入
    if (wcslen(name) == 0 || wcslen(url) == 0)
    {
        MessageBoxW(hDlg, L"请输入名称和URL", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("UpdateBookmarkFromDialog: 名称或URL为空");
        return;
    }
    
    // 检查URL格式
    if (!IsURL(url))
    {
        MessageBoxW(hDlg, L"请输入有效的URL", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("UpdateBookmarkFromDialog: URL格式无效");
        return;
    }
    
    // 更新网址
    g_bookmarks[selIndex].first = name;
    g_bookmarks[selIndex].second = url;
    
    // 保存到文件
    SaveBookmarks();
    
    // 刷新列表
    RefreshBookmarkList(hList);
    
    // 重新选择更新后的项
    ListView_SetItemState(hList, selIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    
    LogToFile("UpdateBookmarkFromDialog: 网址更新成功");
}

// 从对话框删除网址
void DeleteBookmarkFromDialog(HWND hDlg)
{
    LogToFile("DeleteBookmarkFromDialog: 从对话框删除网址");
    
    // 获取选中的网址索引
    HWND hList = GetDlgItem(hDlg, IDC_BOOKMARK_LIST);
    int selIndex = (int)ListView_GetNextItem(hList, -1, LVNI_FOCUSED);
    
    if (selIndex == -1 || selIndex >= (int)g_bookmarks.size())
    {
        MessageBoxW(hDlg, L"请选择要删除的网址", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("DeleteBookmarkFromDialog: 未选择网址");
        return;
    }
    
    // 确认删除
    if (MessageBoxW(hDlg, L"确定要删除选中的网址吗？", L"确认", MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        LogToFile("DeleteBookmarkFromDialog: 用户取消删除");
        return;
    }
    
    // 删除网址
    g_bookmarks.erase(g_bookmarks.begin() + selIndex);
    
    // 保存到文件
    SaveBookmarks();
    
    // 刷新列表
    RefreshBookmarkList(hList);
    
    // 清空编辑框
    SetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_NAME), L"");
    SetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_URL), L"");
    
    LogToFile("DeleteBookmarkFromDialog: 网址删除成功");
}

// 显示设置菜单（现在通过set命令调用）
void ShowSettingsMenu() {
    LogToFile("ShowSettingsMenu: 显示设置菜单");
    g_settingsMenuMode = true;
    g_currentSearch[0] = L'\0';
    
    // 清空列表框并显示菜单项
    ListView_DeleteAllItems(g_hListView);
    g_searchResults.clear();
    
    // 添加提示行
    AddHintRowToListView(L"💡 设置菜单 - 双击选择项目");
    
    // 添加菜单项到列表框
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    
    // 添加菜单项
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.iSubItem = 0;
    lvi.pszText = (WCHAR*)L"退出程序";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"网址管理";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"快捷方式管理";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"系统设置";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"关于软件";
    ListView_InsertItem(g_hListView, &lvi);
    
    // 记录菜单项数量
    int itemCount = ListView_GetItemCount(g_hListView);
    char logMsg[200] = {0};
    sprintf(logMsg, "ShowSettingsMenu: 显示了 %d 个菜单项", itemCount);
    LogToFile(logMsg);
    
    // 更新WebView2显示
    UpdateSettingsMenuWebView();
}

// 显示快捷方式管理对话框
void ShowShortcutManagementDialog() {
    LogToFile("ShowShortcutManagementDialog: 显示快捷方式管理对话框");
    MessageBox(g_hMainWindow, L"快捷方式管理功能开发中...", L"快捷方式管理", MB_OK | MB_ICONINFORMATION);
}

// 显示系统设置对话框
void ShowSystemSettingsDialog() {
    LogToFile("ShowSystemSettingsDialog: 显示系统设置对话框");
    MessageBox(g_hMainWindow, L"系统设置功能开发中...", L"系统设置", MB_OK | MB_ICONINFORMATION);
}

// 显示关于对话框
void ShowAboutDialog() {
    LogToFile("ShowAboutDialog: 显示关于对话框");
    MessageBox(g_hMainWindow, 
               L"BV快启工具箱\\n"
               L"版本: 2.0\\n"
               L"开发者: BV团队\\n"
               L"\\n"
               L"这是一个快速启动工具，支持网址收藏、计算器等功能的窗口。\\n"
               L"通过'输入'和'set'命令可以快速访问各种功能。",
               L"关于软件", 
               MB_OK | MB_ICONINFORMATION);
}

// 处理设置菜单项的双击事件
void HandleSettingsMenuItemClick(INT_PTR itemIndex) {
    char logMsg[512] = {0};
    sprintf(logMsg, "HandleSettingsMenuItemClick: 处理设置菜单项点击，索引: %Id", itemIndex);
    LogToFile(logMsg);
    
    // 检查索引是否有效
    int itemCount = ListView_GetItemCount(g_hListView);
    if (itemIndex < 0 || itemIndex >= itemCount)
    {
        sprintf(logMsg, "HandleSettingsMenuItemClick: 无效索引 %Id，ListView项目数: %d", itemIndex, itemCount);
        LogToFile(logMsg);
        return;
    }
    
    // 获取菜单项文本
    WCHAR itemText[256] = {0};
    LVITEMW lvItem = {0};
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = (int)itemIndex;
    lvItem.iSubItem = 0;
    lvItem.pszText = itemText;
    lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
    
    BOOL getItemResult = ListView_GetItem(g_hListView, &lvItem);
    if (!getItemResult)
    {
        DWORD error = GetLastError();
        sprintf(logMsg, "HandleSettingsMenuItemClick: ListView_GetItem失败，错误代码: %lu", error);
        LogToFile(logMsg);
        return;
    }
    
    char itemLog[512] = {0};
    WideCharToMultiByte(CP_UTF8, 0, itemText, -1, itemLog, sizeof(itemLog), NULL, NULL);
    sprintf(logMsg, "HandleSettingsMenuItemClick: 用户选择了 '%s' (索引: %Id)", itemLog, itemIndex);
    LogToFile(logMsg);
    
    // 根据选择的菜单项执行相应操作
    if (wcscmp(itemText, L"退出程序") == 0) {
        // 退出程序
        LogToFile("HandleSettingsMenuItemClick: 用户选择退出程序");
        PostMessage(g_hMainWindow, WM_CLOSE, 0, 0);
    }
    else if (wcscmp(itemText, L"网址管理") == 0) {
        // 网址管理
        LogToFile("HandleSettingsMenuItemClick: 用户选择网址管理");
        ShowBookmarkDialog();
    }
    else if (wcscmp(itemText, L"快捷方式管理") == 0) {
        // 快捷方式管理
        LogToFile("HandleSettingsMenuItemClick: 用户选择快捷方式管理");
        ShowShortcutManagementDialog();
    }
    else if (wcscmp(itemText, L"系统设置") == 0) {
        // 系统设置
        LogToFile("HandleSettingsMenuItemClick: 用户选择系统设置");
        ShowSystemSettingsDialog();
    }
    else if (wcscmp(itemText, L"关于软件") == 0) {
        // 关于软件
        LogToFile("HandleSettingsMenuItemClick: 用户选择关于软件");
        ShowAboutDialog();
    }
}

// 复制选中的ListView项目
void CopySelectedListItem() {
    // 获取选中的项目
    INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
    if (selIndex == -1) {
        LogToFile("CopySelectedListItem: 没有选中的项目");
        return;
    }
    
    // 根据当前模式获取项目文本
    WCHAR itemText[1024] = {0};
    
    if (g_calculatorMode) {
        // 计算模式下，从计算历史中获取
        if (selIndex < (INT_PTR)g_calculationHistory.size()) {
            size_t actualIndex = g_calculationHistory.size() - 1 - selIndex;
            wcscpy(itemText, g_calculationHistory[actualIndex].expression.c_str());
        }
    } else if (g_bookmarkMode) {
        // 收藏模式下，从收藏结果中获取
        if (selIndex < (INT_PTR)g_bookmarkSearchResults.size()) {
            wcscpy(itemText, g_bookmarkSearchResults[selIndex].first.c_str());
        }
    } else {
        // 普通搜索模式下，直接从ListView获取
        LVITEMW lvItem = {0};
        lvItem.iItem = (INT)selIndex;
        lvItem.iSubItem = 0;
        lvItem.pszText = itemText;
        lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
        ListView_GetItem(g_hListView, &lvItem);
    }
    
    // 复制到剪贴板
    if (itemText[0] != L'\0') {
        if (OpenClipboard(g_hMainWindow)) {
            EmptyClipboard();
            
            // 计算所需内存大小（包括null终止符）
            size_t byteCount = (wcslen(itemText) + 1) * sizeof(wchar_t);
            
            // 分配内存
            HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, byteCount);
            if (hClipboardData) {
                // 锁定内存并复制文本
                LPVOID lpMem = GlobalLock(hClipboardData);
                memcpy(lpMem, itemText, byteCount);
                GlobalUnlock(hClipboardData);
                
                // 设置剪贴板数据
                SetClipboardData(CF_UNICODETEXT, hClipboardData);
            }
            
            CloseClipboard();
            LogToFile("CopySelectedListItem: 已复制选中项目到剪贴板");
        }
    }
}

// 保存窗口大小和位置到注册表
void SaveWindowSettings() {
    RECT windowRect;
    if (GetWindowRect(g_hMainWindow, &windowRect)) {
        // 保存窗口位置和大小到注册表
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueEx(hKey, L"WindowLeft", 0, REG_DWORD, (BYTE*)&windowRect.left, sizeof(DWORD));
            RegSetValueEx(hKey, L"WindowTop", 0, REG_DWORD, (BYTE*)&windowRect.top, sizeof(DWORD));
            DWORD windowWidth = (DWORD)(windowRect.right - windowRect.left);
            DWORD windowHeight = (DWORD)(windowRect.bottom - windowRect.top);
            RegSetValueEx(hKey, L"WindowWidth", 0, REG_DWORD, (BYTE*)&windowWidth, sizeof(DWORD));
            RegSetValueEx(hKey, L"WindowHeight", 0, REG_DWORD, (BYTE*)&windowHeight, sizeof(DWORD));
            RegCloseKey(hKey);
            LogToFile("窗口大小和位置已保存");
        }
    }
}

// 从注册表加载窗口大小和位置
void LoadWindowSettings(int& x, int& y, int& width, int& height) {
    // 默认窗口大小和位置
    width = 800;
    height = 600;
    
    // 获取屏幕工作区域大小
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int screenWidth = workArea.right - workArea.left;
    int screenHeight = workArea.bottom - workArea.top;
    
    // 居中显示
    x = workArea.left + (screenWidth - width) / 2;
    y = workArea.top + (screenHeight - height) / 2;
    
    // 从注册表加载窗口设置
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue;
        DWORD dwSize = sizeof(DWORD);
        
        if (RegQueryValueEx(hKey, L"WindowWidth", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            width = dwValue;
        }
        if (RegQueryValueEx(hKey, L"WindowHeight", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            height = dwValue;
        }
        if (RegQueryValueEx(hKey, L"WindowLeft", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            x = dwValue;
        }
        if (RegQueryValueEx(hKey, L"WindowTop", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            y = dwValue;
        }
        
        RegCloseKey(hKey);
    }
    
    // 确保窗口在屏幕范围内
    if (x < workArea.left) x = workArea.left;
    if (y < workArea.top) y = workArea.top;
    if (x + width > workArea.right) x = workArea.right - width;
    if (y + height > workArea.bottom) y = workArea.bottom - height;
    
    // 确保最小窗口大小
    if (width < 600) width = 600;
    if (height < 400) height = 400;
}

// 初始化 WebView2


// 更新 WebView2 内容
void UpdateWebView2Content(const WCHAR* htmlContent)
{
    if (g_webView && htmlContent)
    {
        // 直接使用NavigateToString，这是最快的方法
        HRESULT hr = g_webView->NavigateToString(htmlContent);
        if (SUCCEEDED(hr))
        {
            // 只在调试时记录日志，减少日志开销
            #ifdef _DEBUG
            char logMsg[300] = {0};
            int contentLen = wcslen(htmlContent);
            sprintf(logMsg, "UpdateWebView2Content: 已更新 WebView2 内容，HTML 长度: %d 字符", contentLen);
            LogToFile(logMsg);
            #endif
        }
        else
        {
            char errorMsg[256] = {0};
            sprintf(errorMsg, "UpdateWebView2Content: 更新失败，错误代码: 0x%08X", hr);
            LogToFile(errorMsg);
        }
    }
    else
    {
        if (!g_webView)
        {
            LogToFile("UpdateWebView2Content: WebView2 未初始化 (g_webView 为空)");
        }
        if (!htmlContent)
        {
            LogToFile("UpdateWebView2Content: HTML 内容为空");
        }
    }
}

// 创建 WebView2 HTML 内容
void CreateWebView2HTML(const std::vector<ShortcutItem>& items, const std::vector<std::wstring>& hints, std::wstring& html)
{
    // 预分配内存，减少重新分配开销（估算：每个项目约100字符，基础HTML约2000字符）
    html.reserve(items.size() * 100 + 2000);
    html = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += L"<style>";
    html += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 10px; background: #f5f5f5; }";
    html += L".hint-banner { background: #fffbee; border-left: 4px solid #f7a531; padding: 12px 16px; margin-bottom: 12px; border-radius: 6px; box-shadow: 0 1px 2px rgba(0,0,0,0.05); }";
    html += L".hint-banner .banner-title { font-weight: bold; color: #b15c00; margin-bottom: 6px; display: flex; align-items: center; }";
    html += L".hint-banner ul { margin: 0; padding-left: 20px; color: #6b4f1d; }";
    html += L".hint-banner li { margin: 4px 0; }";
    html += L"table { width: 100%; border-collapse: collapse; background: white; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-radius: 8px; overflow: hidden; }";
    html += L"th { background: #4CAF50; color: white; padding: 12px; text-align: left; font-weight: bold; }";
    html += L"td { padding: 10px; border-bottom: 1px solid #f0f0f0; }";
    html += L"tr:hover { background: #f0f7ff; cursor: pointer; }";
    html += L"tr.selected { background: #e3f2fd; }";
    html += L"tr.empty-row td { text-align: center; color: #777; font-style: italic; }";
    html += L"</style>";
    html += L"<script>";
    html += L"let selectedIndex = -1;";
    html += L"function selectRow(index) {";
    html += L"  let rows = document.querySelectorAll('tr.item-row');";
    html += L"  rows.forEach((r, i) => r.classList.toggle('selected', i === index));";
    html += L"  selectedIndex = index;";
    html += L"}";
    html += L"function onRowClick(index) {";
    html += L"  selectRow(index);";
    html += L"  if (window.chrome && window.chrome.webview) {";
    html += L"    window.chrome.webview.postMessage(JSON.stringify({type:'itemClick', index:index}));";
    html += L"  }";
    html += L"}";
    html += L"function onRowDblClick(index) {";
    html += L"  if (window.chrome && window.chrome.webview) {";
    html += L"    window.chrome.webview.postMessage(JSON.stringify({type:'itemDblClick', index:index}));";
    html += L"  }";
    html += L"}";
    html += L"</script>";
    html += L"</head><body>";
    
    if (!hints.empty())
    {
        html += L"<div class='hint-banner'>";
        html += L"<div class='banner-title'>💡 操作提示</div><ul>";
        for (const auto& hint : hints)
        {
            html += L"<li>";
            html += hint;
            html += L"</li>";
        }
        html += L"</ul></div>";
    }
    
    html += L"<table><thead><tr><th>名称</th><th>路径</th></tr></thead><tbody>";
    
    if (items.empty())
    {
        html += L"<tr class='empty-row'><td colspan='2'>未找到匹配项，试试其他关键字，或输入 <strong>help</strong> 查看可用命令。</td></tr>";
    }
    else
    {
        for (size_t i = 0; i < items.size(); i++)
        {
            html += L"<tr class='item-row' onclick='onRowClick(";
            html += std::to_wstring(i);
            html += L")' ondblclick='onRowDblClick(";
            html += std::to_wstring(i);
            html += L")'>";
            html += L"<td>";
            html += items[i].name;
            html += L"</td><td>";
            html += items[i].path;
            html += L"</td></tr>";
        }
    }
    
    html += L"</tbody></table></body></html>";
}

void UpdateCalculatorModeWebView()
{
    if (!g_webView)
    {
        LogToFile("UpdateCalculatorModeWebView: WebView2 未初始化，无法显示计算模式内容");
        return;
    }
    
    // 预分配内存，减少重新分配开销
    std::wstring html;
    html.reserve(g_calculationHistory.size() * 150 + 3000);
    html = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += L"<style>";
    html += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 10px; background: #10131a; color: #f5f8ff; }";
    html += L".mode-banner { background: linear-gradient(90deg, #ff8a00, #e52e71); padding: 16px; border-radius: 10px; font-size: 18px; font-weight: bold; box-shadow: 0 4px 12px rgba(0,0,0,0.3); display: flex; justify-content: space-between; align-items: center; }";
    html += L".hint-list { margin: 14px 0; padding: 14px; background: rgba(255,255,255,0.08); border-radius: 8px; }";
    html += L".hint-list ul { margin: 0; padding-left: 24px; }";
    html += L".hint-list li { margin: 6px 0; }";
    html += L".history { margin-top: 14px; background: rgba(0,0,0,0.25); border-radius: 10px; padding: 10px; box-shadow: inset 0 0 0 1px rgba(255,255,255,0.05); }";
    html += L"table { width: 100%; border-collapse: collapse; }";
    html += L"th, td { padding: 10px; border-bottom: 1px solid rgba(255,255,255,0.08); }";
    html += L"th { text-align: left; color: #7dd3fc; font-size: 14px; }";
    html += L"td { font-size: 14px; color: #f5f8ff; }";
    html += L"tr:last-child td { border-bottom: none; }";
    html += L"tr.history-row { cursor: pointer; }";
    html += L"tr.history-row:hover { background: rgba(255,255,255,0.1); }";
    html += L"tr.history-row.selected { background: rgba(59, 130, 246, 0.3); }";
    html += L".empty { text-align: center; color: #9ca3af; font-style: italic; padding: 20px 0; }";
    html += L".action-button { position: relative; background: rgba(255,255,255,0.15); border: 1px solid rgba(255,255,255,0.3); color: #f5f8ff; padding: 8px 16px; border-radius: 6px; cursor: pointer; font-size: 14px; transition: all 0.2s; }";
    html += L".action-button:hover { background: rgba(255,255,255,0.25); }";
    html += L".dropdown-menu { position: absolute; top: 100%; right: 0; margin-top: 4px; background: rgba(30, 30, 40, 0.98); border: 1px solid rgba(255,255,255,0.2); border-radius: 6px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); min-width: 160px; z-index: 1000; display: none; }";
    html += L".dropdown-menu.show { display: block; }";
    html += L".dropdown-menu-item { padding: 10px 16px; cursor: pointer; color: #f5f8ff; font-size: 14px; border-bottom: 1px solid rgba(255,255,255,0.1); }";
    html += L".dropdown-menu-item:last-child { border-bottom: none; }";
    html += L".dropdown-menu-item:hover { background: rgba(255,255,255,0.1); }";
    html += L".dropdown-menu-separator { height: 1px; background: rgba(255,255,255,0.1); margin: 4px 0; }";
    html += L"</style>";
    html += L"<script>";
    html += L"let selectedHistoryIndex = -1;";
    html += L"function selectHistoryRow(index) {";
    html += L"  let rows = document.querySelectorAll('tr.history-row');";
    html += L"  rows.forEach((r, i) => r.classList.toggle('selected', i === index));";
    html += L"  selectedHistoryIndex = index;";
    html += L"}";
    html += L"function onHistoryRowClick(index) {";
    html += L"  selectHistoryRow(index);";
    html += L"}";
    html += L"function toggleDropdown() {";
    html += L"  let menu = document.getElementById('actionMenu');";
    html += L"  menu.classList.toggle('show');";
    html += L"}";
    html += L"function hideDropdown() {";
    html += L"  let menu = document.getElementById('actionMenu');";
    html += L"  menu.classList.remove('show');";
    html += L"}";
    html += L"function handleAction(action) {";
    html += L"  hideDropdown();";
    html += L"  if (window.chrome && window.chrome.webview) {";
    html += L"    window.chrome.webview.postMessage(JSON.stringify({type:'calcAction', action:action, index:selectedHistoryIndex}));";
    html += L"  }";
    html += L"}";
    html += L"document.addEventListener('click', function(e) {";
    html += L"  if (!e.target.closest('.action-button-container')) {";
    html += L"    hideDropdown();";
    html += L"  }";
    html += L"});";
    html += L"</script>";
    html += L"</head><body>";
    
    html += L"<div class='mode-banner'>";
    html += L"<span>🧮 计算模式 (js) · 输入表达式并按回车即可计算</span>";
    html += L"<div class='action-button-container' style='position: relative;'>";
    html += L"<button class='action-button' onclick='toggleDropdown()'>操作 ▼</button>";
    html += L"<div class='dropdown-menu' id='actionMenu'>";
    html += L"<div class='dropdown-menu-item' onclick='handleAction(\"copy\")'>复制选中项</div>";
    html += L"<div class='dropdown-menu-separator'></div>";
    html += L"<div class='dropdown-menu-item' onclick='handleAction(\"delete\")'>删除选中项</div>";
    html += L"<div class='dropdown-menu-item' onclick='handleAction(\"clearAll\")'>清空历史记录</div>";
    html += L"</div></div></div>";
    
    html += L"<div class='hint-list'><ul>";
    html += L"<li>输入数学表达式，例如 <code>2+3*4</code> 或 <code>sqrt(16)</code></li>";
    html += L"<li>支持函数：sin、cos、tan、sqrt、abs 等</li>";
    html += L"<li>输入 <strong>q</strong> 退出计算模式</li>";
    html += L"</ul></div>";
    
    html += L"<div class='history'>";
    if (g_calculationHistory.empty())
    {
        html += L"<div class='empty'>暂无计算记录，试着输入表达式开始计算吧。</div>";
    }
    else
    {
        html += L"<table><thead><tr><th>表达式</th><th>结果</th><th>备注</th></tr></thead><tbody>";
        for (size_t i = g_calculationHistory.size(); i > 0; --i)
        {
            size_t displayIndex = g_calculationHistory.size() - i;
            html += L"<tr class='history-row' onclick='onHistoryRowClick(";
            html += std::to_wstring(displayIndex);
            html += L")'><td>";
            html += g_calculationHistory[i - 1].expression;
            html += L"</td><td>";
            html += g_calculationHistory[i - 1].result;
            html += L"</td><td>";
            if (g_calculationHistory[i - 1].comment.empty())
            {
                html += L"-";
            }
            else
            {
                html += g_calculationHistory[i - 1].comment;
            }
            html += L"</td></tr>";
        }
        html += L"</tbody></table>";
    }
    html += L"</div></body></html>";
    
    UpdateWebView2Content(html.c_str());
}

void UpdateSettingsMenuWebView()
{
    if (!g_webView)
    {
        LogToFile("UpdateSettingsMenuWebView: WebView2 未初始化，无法显示设置菜单");
        return;
    }
    
    // 使用缓存，避免重复生成
    if (!g_settingsHtmlCached)
    {
        g_cachedSettingsHtml.reserve(1500);  // 预分配内存
        g_cachedSettingsHtml = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
        g_cachedSettingsHtml += L"<style>";
        g_cachedSettingsHtml += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 10px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #f5f8ff; }";
        g_cachedSettingsHtml += L".mode-banner { background: linear-gradient(90deg, #4a90e2, #357abd); padding: 16px; border-radius: 10px; font-size: 18px; font-weight: bold; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 20px; }";
        g_cachedSettingsHtml += L".hint-banner { background: rgba(255,255,255,0.15); border-left: 4px solid #FFD700; padding: 12px 16px; margin-bottom: 20px; border-radius: 6px; }";
        g_cachedSettingsHtml += L".menu-container { background: rgba(255,255,255,0.95); border-radius: 10px; padding: 20px; box-shadow: 0 4px 12px rgba(0,0,0,0.2); }";
        g_cachedSettingsHtml += L".menu-item { padding: 16px 20px; margin: 8px 0; background: linear-gradient(90deg, #f8f9fa, #e9ecef); border-left: 4px solid #4a90e2; border-radius: 6px; cursor: pointer; transition: all 0.2s; color: #333; font-size: 16px; }";
        g_cachedSettingsHtml += L".menu-item:hover { background: linear-gradient(90deg, #e9ecef, #dee2e6); transform: translateX(5px); box-shadow: 0 2px 8px rgba(0,0,0,0.15); }";
        g_cachedSettingsHtml += L".menu-item:active { transform: translateX(2px); }";
        g_cachedSettingsHtml += L".menu-icon { display: inline-block; width: 24px; margin-right: 12px; text-align: center; font-size: 20px; }";
        g_cachedSettingsHtml += L"</style>";
        g_cachedSettingsHtml += L"<script>";
        g_cachedSettingsHtml += L"function onMenuItemClick(index) {";
        g_cachedSettingsHtml += L"  if (window.chrome && window.chrome.webview) {";
        g_cachedSettingsHtml += L"    window.chrome.webview.postMessage(JSON.stringify({type:'settingsAction', index:index}));";
        g_cachedSettingsHtml += L"  }";
        g_cachedSettingsHtml += L"}";
        g_cachedSettingsHtml += L"</script>";
        g_cachedSettingsHtml += L"</head><body>";
        
        g_cachedSettingsHtml += L"<div class='mode-banner'>⚙️ 设置菜单 (set) · 选择功能进行配置</div>";
        
        g_cachedSettingsHtml += L"<div class='hint-banner'>💡 双击或点击菜单项执行操作</div>";
        
        g_cachedSettingsHtml += L"<div class='menu-container'>";
        
        // 菜单项列表
        const WCHAR* menuItems[] = {
            L"退出程序",
            L"网址管理",
            L"快捷方式管理",
            L"系统设置",
            L"关于软件"
        };
        
        const WCHAR* menuIcons[] = {
            L"🚪",
            L"🔖",
            L"📁",
            L"⚙️",
            L"ℹ️"
        };
        
        for (int i = 0; i < 5; i++)
        {
            g_cachedSettingsHtml += L"<div class='menu-item' onclick='onMenuItemClick(";
            g_cachedSettingsHtml += std::to_wstring(i);
            g_cachedSettingsHtml += L")' ondblclick='onMenuItemClick(";
            g_cachedSettingsHtml += std::to_wstring(i);
            g_cachedSettingsHtml += L")'>";
            g_cachedSettingsHtml += L"<span class='menu-icon'>";
            g_cachedSettingsHtml += menuIcons[i];
            g_cachedSettingsHtml += L"</span>";
            g_cachedSettingsHtml += menuItems[i];
            g_cachedSettingsHtml += L"</div>";
        }
        
        g_cachedSettingsHtml += L"</div></body></html>";
        g_settingsHtmlCached = true;
        LogToFile("UpdateSettingsMenuWebView: 设置菜单HTML已缓存");
    }
    
    UpdateWebView2Content(g_cachedSettingsHtml.c_str());
}

void UpdateHelpInfoWebView()
{
    if (!g_webView)
    {
        LogToFile("UpdateHelpInfoWebView: WebView2 未初始化，无法显示帮助信息");
        return;
    }
    
    // 使用缓存，避免重复生成
    if (!g_helpHtmlCached)
    {
        // 从外部模板文件读取HTML内容
        std::wstring templatePath = L"data/help_template.html";
        g_cachedHelpHtml = ReadHtmlTemplate(templatePath);
        
        // 如果读取失败，使用默认的HTML内容作为后备
        if (g_cachedHelpHtml.find(L"错误：无法加载模板文件") != std::wstring::npos)
        {
            LogToFile("UpdateHelpInfoWebView: 模板文件读取失败，使用默认HTML内容");
            
            // 使用原来的硬编码HTML内容作为后备
            g_cachedHelpHtml.reserve(2000);  // 预分配内存
            g_cachedHelpHtml = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
            g_cachedHelpHtml += L"<style>";
            g_cachedHelpHtml += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 10px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #f5f8ff; }";
            g_cachedHelpHtml += L".help-banner { background: linear-gradient(90deg, #4a90e2, #357abd); padding: 16px; border-radius: 10px; font-size: 18px; font-weight: bold; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 20px; }";
            g_cachedHelpHtml += L".help-section { background: rgba(255,255,255,0.95); border-radius: 10px; padding: 20px; margin-bottom: 15px; box-shadow: 0 4px 12px rgba(0,0,0,0.2); color: #333; }";
            g_cachedHelpHtml += L".help-section h2 { color: #4a90e2; margin-top: 0; margin-bottom: 15px; font-size: 18px; border-bottom: 2px solid #4a90e2; padding-bottom: 8px; }";
            g_cachedHelpHtml += L".help-section ul { margin: 0; padding-left: 20px; }";
            g_cachedHelpHtml += L".help-section li { margin: 8px 0; line-height: 1.6; }";
            g_cachedHelpHtml += L".help-section code { background: #f0f0f0; padding: 2px 6px; border-radius: 4px; font-family: 'Courier New', monospace; color: #d63384; }";
            g_cachedHelpHtml += L".help-tip { background: rgba(255, 193, 7, 0.2); border-left: 4px solid #ffc107; padding: 12px; margin: 10px 0; border-radius: 4px; }";
            g_cachedHelpHtml += L"</style></head><body>";
            
            g_cachedHelpHtml += L"<div class='help-banner'>📖 使用帮助 · 快速了解如何使用本工具</div>";
            
            g_cachedHelpHtml += L"<div class='help-section'>";
            g_cachedHelpHtml += L"<h2>基本操作</h2>";
            g_cachedHelpHtml += L"<ul>";
            g_cachedHelpHtml += L"<li>在输入框中输入内容，按回车键执行</li>";
            g_cachedHelpHtml += L"<li>支持实时搜索和快捷启动</li>";
            g_cachedHelpHtml += L"<li>双击列表项可执行对应操作</li>";
            g_cachedHelpHtml += L"</ul>";
            g_cachedHelpHtml += L"</div>";
            
            g_cachedHelpHtml += L"<div class='help-section'>";
            g_cachedHelpHtml += L"<h2>快捷命令</h2>";
            g_cachedHelpHtml += L"<ul>";
            g_cachedHelpHtml += L"<li><code>help</code> - 显示此帮助信息</li>";
            g_cachedHelpHtml += L"<li><code>set</code> - 显示设置菜单</li>";
            g_cachedHelpHtml += L"<li><code>js</code> - 切换到计算模式</li>";
            g_cachedHelpHtml += L"<li><code>wz</code> - 切换到网址收藏模式</li>";
            g_cachedHelpHtml += L"<li><code>dir</code> - 切换到目录浏览模式</li>";
            g_cachedHelpHtml += L"<li><code>q</code> - 退出现有模式</li>";
            g_cachedHelpHtml += L"</ul>";
            g_cachedHelpHtml += L"</div>";
            
            g_cachedHelpHtml += L"<div class='help-section'>";
            g_cachedHelpHtml += L"<h2>模式说明</h2>";
            g_cachedHelpHtml += L"<ul>";
            g_cachedHelpHtml += L"<li><strong>设置模式 (set)</strong>：显示设置菜单，可以管理网址收藏、退出程序等</li>";
            g_cachedHelpHtml += L"<li><strong>计算模式 (js)</strong>：输入数学表达式进行计算，支持常用数学函数</li>";
            g_cachedHelpHtml += L"<li><strong>网址收藏模式 (wz)</strong>：浏览和管理收藏的网址，支持搜索和快速打开</li>";
            g_cachedHelpHtml += L"<li><strong>目录浏览模式 (dir)</strong>：浏览文件和文件夹，支持展开目录和打开文件</li>";
            g_cachedHelpHtml += L"</ul>";
            g_cachedHelpHtml += L"</div>";
            
            g_cachedHelpHtml += L"<div class='help-section'>";
            g_cachedHelpHtml += L"<h2>使用技巧</h2>";
            g_cachedHelpHtml += L"<ul>";
            g_cachedHelpHtml += L"<li>使用 <code>Ctrl+Alt+Q</code> 快速显示/隐藏窗口</li>";
            g_cachedHelpHtml += L"<li>使用 <code>Ctrl+F1</code> 将窗口定位到桌面中央</li>";
            g_cachedHelpHtml += L"<li>最小化窗口时会自动隐藏到系统托盘</li>";
            g_cachedHelpHtml += L"<li>支持模糊搜索，输入部分名称即可匹配</li>";
            g_cachedHelpHtml += L"<li>在目录浏览模式下，点击目录可展开，双击文件可打开</li>";
            g_cachedHelpHtml += L"</ul>";
            g_cachedHelpHtml += L"</div>";
            
            g_cachedHelpHtml += L"<div class='help-tip'>💡 提示：输入任意内容开始搜索，或使用上述命令进入特定模式</div>";
            
            g_cachedHelpHtml += L"</body></html>";
        }
        
        g_helpHtmlCached = true;
        LogToFile("UpdateHelpInfoWebView: 帮助信息HTML已缓存");
    }
    
    UpdateWebView2Content(g_cachedHelpHtml.c_str());
}

void UpdateBookmarkModeWebView()
{
    if (!g_webView)
    {
        LogToFile("UpdateBookmarkModeWebView: WebView2 未初始化，无法显示网址收藏");
        return;
    }
    
    // 使用搜索结果（如果有）或全部网址收藏
    const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
    
    // 预分配内存
    std::wstring html;
    html.reserve(displayBookmarks.size() * 200 + 3000);
    html = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += L"<style>";
    html += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 10px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #f5f8ff; }";
    html += L".mode-banner { background: linear-gradient(90deg, #4a90e2, #357abd); padding: 16px; border-radius: 10px; font-size: 18px; font-weight: bold; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 20px; }";
    html += L".hint-banner { background: rgba(255,255,255,0.15); border-left: 4px solid #FFD700; padding: 12px 16px; margin-bottom: 20px; border-radius: 6px; }";
    html += L".add-button { background: linear-gradient(90deg, #28a745, #20c997); color: white; border: none; padding: 10px 20px; border-radius: 6px; font-size: 14px; font-weight: bold; cursor: pointer; margin-bottom: 20px; box-shadow: 0 2px 8px rgba(0,0,0,0.2); transition: all 0.3s ease; }";
    html += L".add-button:hover { background: linear-gradient(90deg, #20c997, #17a2b8); transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.3); }";
    html += L".add-button:active { transform: translateY(0); box-shadow: 0 2px 4px rgba(0,0,0,0.2); }";
    html += L"table { width: 100%; border-collapse: collapse; background: rgba(255,255,255,0.95); border-radius: 10px; overflow: hidden; box-shadow: 0 4px 12px rgba(0,0,0,0.2); }";
    html += L"th { background: linear-gradient(90deg, #4a90e2, #357abd); color: white; padding: 12px; text-align: left; font-weight: bold; }";
    html += L"td { padding: 10px; border-bottom: 1px solid #f0f0f0; color: #333; }";
    html += L"tr.bookmark-row { cursor: pointer; }";
    html += L"tr.bookmark-row:hover { background: #f0f7ff; }";
    html += L"tr.bookmark-row.selected { background: #e3f2fd; }";
    html += L"tr:last-child td { border-bottom: none; }";
    html += L".empty { text-align: center; color: #9ca3af; font-style: italic; padding: 20px 0; background: rgba(255,255,255,0.95); border-radius: 10px; }";
    html += L".url-cell { color: #666; font-size: 13px; max-width: 400px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }";
    html += L"</style>";
    html += L"<script>";
    html += L"let selectedBookmarkIndex = -1;";
    html += L"function selectBookmarkRow(index) {";
    html += L"  let rows = document.querySelectorAll('tr.bookmark-row');";
    html += L"  rows.forEach((r, i) => r.classList.toggle('selected', i === index));";
    html += L"  selectedBookmarkIndex = index;";
    html += L"}";
    html += L"function onBookmarkRowClick(index) {";
    html += L"  selectBookmarkRow(index);";
    html += L"}";
    html += L"function onBookmarkRowDblClick(index) {";
    html += L"  if (window.chrome && window.chrome.webview) {";
    html += L"    window.chrome.webview.postMessage(JSON.stringify({type:'bookmarkDblClick', index:index}));";
    html += L"  }";
    html += L"}";
    html += L"function onAddBookmarkClick() {";
    html += L"  if (window.chrome && window.chrome.webview) {";
    html += L"    window.chrome.webview.postMessage(JSON.stringify({type:'addBookmark'}));";
    html += L"  }";
    html += L"}";
    html += L"</script>";
    html += L"</head><body>";
    
    html += L"<div class='mode-banner'>🔖 网址收藏模式 (wz) · 浏览和管理收藏的网址</div>";
    
    html += L"<div class='hint-banner'>💡 双击网址打开，输入关键词搜索，输入 q 退出模式，点击下方按钮添加新网址</div>";
    
    // 添加网址按钮
    html += L"<button class='add-button' onclick='onAddBookmarkClick()'>➕ 添加新网址</button>";
    
    if (displayBookmarks.empty())
    {
        html += L"<div class='empty'>暂无收藏的网址，点击上方按钮添加网址收藏</div>";
    }
    else
    {
        html += L"<table><thead><tr><th>名称</th><th>网址</th></tr></thead><tbody>";
        
        for (size_t i = 0; i < displayBookmarks.size(); i++)
        {
            html += L"<tr class='bookmark-row' onclick='onBookmarkRowClick(";
            html += std::to_wstring(i);
            html += L")' ondblclick='onBookmarkRowDblClick(";
            html += std::to_wstring(i);
            html += L")'>";
            html += L"<td>";
            html += displayBookmarks[i].first;
            html += L"</td><td class='url-cell'>";
            html += displayBookmarks[i].second;
            html += L"</td></tr>";
        }
        
        html += L"</tbody></table>";
    }
    
    html += L"</body></html>";
    
    UpdateWebView2Content(html.c_str());
}

// 显示基本用法界面
void ShowBasicUsage()
{
    LogToFile("ShowBasicUsage: 创建基本用法HTML内容");
    
    // 构建基本用法HTML内容
    std::wstring htmlContent = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    htmlContent += L"<style>";
    htmlContent += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; }";
    htmlContent += L".container { max-width: 800px; margin: 0 auto; background: rgba(255,255,255,0.1); border-radius: 15px; padding: 30px; backdrop-filter: blur(10px); }";
    htmlContent += L"h1 { text-align: center; margin-bottom: 30px; font-size: 2.5em; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }";
    htmlContent += L".section { margin-bottom: 25px; padding: 20px; background: rgba(255,255,255,0.1); border-radius: 10px; border-left: 4px solid #FFD700; }";
    htmlContent += L".section h2 { color: #FFD700; margin-top: 0; font-size: 1.4em; }";
    htmlContent += L".step { margin: 15px 0; padding: 10px; background: rgba(255,255,255,0.1); border-radius: 8px; border-left: 3px solid #32CD32; }";
    htmlContent += L".step-number { display: inline-block; width: 30px; height: 30px; background: #32CD32; border-radius: 50%; text-align: center; line-height: 30px; margin-right: 10px; font-weight: bold; }";
    htmlContent += L"code { background: rgba(0,0,0,0.3); padding: 2px 6px; border-radius: 4px; font-family: 'Courier New', monospace; }";
    htmlContent += L".warning { background: rgba(255,69,58,0.3); border-left-color: #FF4500; }";
    htmlContent += L".tip { background: rgba(50,205,50,0.3); border-left-color: #32CD32; }";
    htmlContent += L"ul { list-style: none; padding-left: 0; }";
    htmlContent += L"li { margin: 8px 0; padding-left: 25px; position: relative; }";
    htmlContent += L"li:before { content: '▶'; position: absolute; left: 0; color: #FFD700; }";
    htmlContent += L"</style>";
    htmlContent += L"</head><body>";
    
    htmlContent += L"<div class='container'>";
    htmlContent += L"<h1>🚀 Funny Quick 快速入门</h1>";
    
    htmlContent += L"<div class='section'>";
    htmlContent += L"<h2>📋 基本功能</h2>";
    htmlContent += L"<ul>";
    htmlContent += L"<li>快速搜索和启动应用程序</li>";
    htmlContent += L"<li>支持模糊搜索和快捷键操作</li>";
    htmlContent += L"<li>智能推荐和历史记录</li>";
    htmlContent += L"<li>自定义快捷方式和分类</li>";
    htmlContent += L"</ul>";
    htmlContent += L"</div>";
    
    htmlContent += L"<div class='section'>";
    htmlContent += L"<h2>⌨️ 快捷键操作</h2>";
    htmlContent += L"<div class='step'>";
    htmlContent += L"<span class='step-number'>1</span>按 <code>Ctrl + Space</code> 打开搜索窗口";
    htmlContent += L"</div>";
    htmlContent += L"<div class='step'>";
    htmlContent += L"<span class='step-number'>2</span>输入应用名称或关键词进行搜索";
    htmlContent += L"</div>";
    htmlContent += L"<div class='step'>";
    htmlContent += L"<span class='step-number'>3</span>使用方向键选择目标应用";
    htmlContent += L"</div>";
    htmlContent += L"<div class='step'>";
    htmlContent += L"<span class='step-number'>4</span>按 <code>Enter</code> 启动应用或 <code>Esc</code> 取消";
    htmlContent += L"</div>";
    htmlContent += L"</div>";
    
    htmlContent += L"<div class='section'>";
    htmlContent += L"<h2>🔍 搜索技巧</h2>";
    htmlContent += L"<ul>";
    htmlContent += L"<li>输入应用名称的部分字符即可匹配</li>";
    htmlContent += L"<li>支持中文和英文搜索</li>";
    htmlContent += L"<li>支持路径搜索，查找包含特定路径的程序</li>";
    htmlContent += L"<li>使用上方向键查看搜索历史</li>";
    htmlContent += L"</ul>";
    htmlContent += L"</div>";
    
    htmlContent += L"<div class='section warning'>";
    htmlContent += L"<h2>⚠️ WebView2 运行时问题</h2>";
    htmlContent += L"<p>当前未检测到 Microsoft Edge WebView2 运行时或初始化失败。</p>";
    htmlContent += L"<p><strong>解决方案：</strong></p>";
    htmlContent += L"<ul>";
    htmlContent += L"<li>下载并安装 Microsoft Edge WebView2 运行时</li>";
    htmlContent += L"<li>从 Microsoft Edge 官网获取最新版本</li>";
    htmlContent += L"<li>安装后重启应用程序</li>";
    htmlContent += L"</ul>";
    htmlContent += L"</div>";
    
    htmlContent += L"<div class='section tip'>";
    htmlContent += L"<h2>💡 使用提示</h2>";
    htmlContent += L"<ul>";
    htmlContent += L"<li>双击搜索结果可以直接启动应用</li>";
    htmlContent += L"<li>右键点击可以进行更多操作</li>";
    htmlContent += L"<li>程序会自动记忆使用习惯</li>";
    htmlContent += L"<li>定期更新可获得更好的体验</li>";
    htmlContent += L"</ul>";
    htmlContent += L"</div>";
    
    htmlContent += L"<div class='section'>";
    htmlContent += L"<h2>🎯 开始使用</h2>";
    htmlContent += L"<p>请先解决 WebView2 运行时问题，然后重新启动应用程序以获得完整的用户体验。</p>";
    htmlContent += L"<p>如果您看到此界面，说明应用程序的核心功能仍然可用，只是Web显示界面暂时无法使用。</p>";
    htmlContent += L"</div>";
    
    htmlContent += L"</div></body></html>";
    
    // 更新显示内容
    if (g_webView)
    {
        UpdateWebView2Content(htmlContent.c_str());
    }
    else
    {
        LogToFile("ShowBasicUsage: WebView2 未初始化，无法显示HTML内容");
        
        // 如果没有WebView2，我们可以尝试使用其他方式显示信息
        // 这里可以添加一个MessageBox来提示用户
        MessageBoxW(NULL, L"WebView2 运行时未安装或初始化失败！\n\n请下载安装 Microsoft Edge WebView2 运行时后再使用。\n\n下载地址: https://developer.microsoft.com/microsoft-edge/webview2/\n\n基本功能仍然可用，详情请查看日志文件。", L"Funny Quick - 基本用法", MB_OK | MB_ICONINFORMATION);
    }
}

// HTML模板读取辅助函数实现
std::wstring ReadHtmlTemplate(const std::wstring& filePath)
{
    // 使用二进制方式读取文件，然后手动转换为宽字符串
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        std::string errorMsg = "ReadHtmlTemplate: 无法打开HTML模板文件: " + std::string(filePath.begin(), filePath.end());
        LogToFile(errorMsg.c_str());
        return L"<html><body><h1>错误：无法加载模板文件</h1></body></html>";
    }
    
    // 读取文件内容到字节缓冲区
    std::string buffer;
    file.seekg(0, std::ios::end);
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&buffer[0], buffer.size());
    file.close();
    
    // 简单的UTF-8到宽字符串转换（假设文件是UTF-8编码）
    std::wstring content;
    for (size_t i = 0; i < buffer.size(); )
    {
        wchar_t wc = 0;
        unsigned char c = buffer[i];
        
        if ((c & 0x80) == 0) // 单字节字符
        {
            wc = c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0) // 双字节字符
        {
            if (i + 1 < buffer.size())
            {
                wc = ((c & 0x1F) << 6) | (buffer[i + 1] & 0x3F);
                i += 2;
            }
            else
            {
                wc = L'?'; // 不完整的字符
                i += 1;
            }
        }
        else if ((c & 0xF0) == 0xE0) // 三字节字符
        {
            if (i + 2 < buffer.size())
            {
                wc = ((c & 0x0F) << 12) | ((buffer[i + 1] & 0x3F) << 6) | (buffer[i + 2] & 0x3F);
                i += 3;
            }
            else
            {
                wc = L'?'; // 不完整的字符
                i += 1;
            }
        }
        else
        {
            wc = L'?'; // 无效的UTF-8字节
            i += 1;
        }
        
        content += wc;
    }
    
    std::string successMsg = "ReadHtmlTemplate: 成功读取HTML模板文件: " + std::string(filePath.begin(), filePath.end());
    LogToFile(successMsg.c_str());
    return content;
}

