#include <windows.h>
#include <imm.h>
#include <windowsx.h>  // 用于GET_X_LPARAM和GET_Y_LPARAM宏
#include <tchar.h>
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
#include "common.h"  // 公共定义和声明
#include "calculator.h"  // 计算器功能定义
#include "webview_manager.h"  // WebView2 管理功能
#include "dir_mode_manager.h"  // 目录浏览模式管理功能
#include "window_size_handler.h"  // 窗口大小处理功能
#include "bookmark_manager.h"  // 网址收藏管理功能

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

#define IDC_CALC_MENU_BUTTON 1016  // 计算模式操作菜单按钮ID
#define HOTKEY_ID 1
#define HOTKEY_ID_CTRL_F1 2
// 系统托盘相关常量
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW 1005
#define ID_TRAY_EXIT 1006

// 右键菜单常量
#define ID_CONTEXT_DELETE_ITEM 1007  // 删除单个计算结果
#define ID_CONTEXT_CLEAR_ALL 1008    // 清空所有历史记录

// Global data
std::vector<ShortcutItem> g_shortcuts;
std::vector<ShortcutItem> g_searchResults;
WCHAR g_currentSearch[1024] = {0};

// 计算模式相关变量
bool g_calculatorMode = false;  // 是否处于计算模式
bool g_updatingEditBox = false;  // 是否正在更新编辑框内容，防止触发EN_CHANGE
std::vector<CalculationRecord> g_calculationHistory;  // 计算历史记录

// 目录浏览模式相关变量
bool g_dirMode = false;  // 是否处于目录浏览模式

// 书签管理相关全局变量定义
std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks;  // 网址收藏列表
std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults;  // 网址收藏搜索结果
bool g_bookmarkMode = false;  // 书签模式标志

// 表达式解析辅助函数声明
void EnterCalculatorMode();
void ExitCalculatorMode();
void ShowCalculatorHelpInfo();
void ShowHelpInfo();
void EvaluateExpression(const WCHAR* expression);
void DisplayCalculationHistory();
void SaveCalculationHistory();
void LoadCalculationHistory();



// 目录浏览功能函数声明




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

// 收藏相关函数声明
void CopyToClipboard(const std::wstring& text);
LRESULT HandleWMContextMenu(HWND hwnd, WPARAM wParam);
void ReplaceStringInPlace(std::wstring& str, const std::wstring& from, const std::wstring& to);


// 窗口消息处理函数声明
LRESULT HandleWMKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

// ListView相关函数声明
void ClearListView();

// 窗口大小记忆功能函数声明
void SaveWindowSettings();
void LoadWindowSettings(int& x, int& y, int& width, int& height);

// HTML模板读取辅助函数声明
std::wstring ReadHtmlTemplate(const std::wstring& filePath);

// 日志功能已移至 logger.cpp

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
    
    // 检查是否是"wz"命令，用于进入网址收藏模式
    if (wcscmp(command, L"wz") == 0)
    {
        LogToFile("ProcessCommand: 识别为'wz'命令，进入网址收藏模式");
        EnterBookmarkMode();
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
// 初始化ListView用于搜索
bool InitializeListViewForSearch()
{
    // 检查ListView是否有效
    if (!g_hListView || !IsWindow(g_hListView))
    {
        LogToFile("InitializeListViewForSearch: ListView句柄无效或窗口不存在");
        return false;
    }
    
    // 检查ListView是否有列（如果没有列，需要先初始化列）
    HWND hHeader = ListView_GetHeader(g_hListView);
    int columnCount = 0;
    if (hHeader)
    {
        columnCount = Header_GetItemCount(hHeader);
    }
    if (columnCount == 0)
    {
        LogToFile("InitializeListViewForSearch: ListView没有列，初始化列");
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
        
        LogToFile("InitializeListViewForSearch: ListView列初始化完成（名称 | 路径）");
    }
    
    return true;
}

// 处理搜索查询和模式判断
void ProcessSearchQuery(const WCHAR* query, std::vector<std::wstring>& webViewHints)
{
    // 进入搜索结果模式，退出设置菜单状态
    g_settingsMenuMode = false;
    
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
    sprintf(logMsg, "ProcessSearchQuery: 搜索查询 '%s'", queryLog);
    LogToFile(logMsg);
    
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
        // wz模式（网址收藏模式）：添加多行提示
        const WCHAR* hints[] = {
            L"💡 网址收藏模式",
            L"💡 输入网址名称或URL搜索",
            L"💡 按回车或双击打开网址",
            L"💡 输入 q 退出网址收藏模式"
        };
        AddMultiLineHintsToListView(hints, 4);
        for (int i = 0; i < 4; ++i)
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
}



// 处理快捷方式搜索
void HandleShortcutSearch(const WCHAR* query)
{
    if (!query || wcslen(query) == 0)
    {
        // 空查询时显示最常用的项目
        std::vector<ShortcutItem> sorted = g_shortcuts;
        std::sort(sorted.begin(), sorted.end(), 
            [](const ShortcutItem& a, const ShortcutItem& b) { 
                return a.usageCount > b.usageCount; 
            });
        
        g_searchResults = sorted;
        
        char logMsg[1100] = {0};
        sprintf(logMsg, "HandleShortcutSearch: 显示 %zu 个最常用项目", sorted.size());
        LogToFile(logMsg);
        return;
    }
    
    char logMsg[1100] = {0};
    sprintf(logMsg, "HandleShortcutSearch: 在 %zu 个快捷方式中搜索匹配项", g_shortcuts.size());
    LogToFile(logMsg);
    
    // Search for matching items using case-insensitive comparison
    for (size_t i = 0; i < g_shortcuts.size(); i++)
    {
        // 记录当前检查的项目
        char itemNameLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, g_shortcuts[i].name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
        
        // Check for exact match (already case-insensitive)
        if (_wcsicmp(g_shortcuts[i].name, query) == 0)
        {
            sprintf(logMsg, "HandleShortcutSearch: 找到精确匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
            
            g_searchResults.push_back(g_shortcuts[i]);
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
                sprintf(logMsg, "HandleShortcutSearch: 找到前缀匹配 '%s'", itemNameLog);
                LogToFile(logMsg);
                
                g_searchResults.push_back(g_shortcuts[i]);
            }
            else if (queryLen <= nameLen)
            {
                // Also check for substring match anywhere in the name
                for (size_t j = 0; j <= nameLen - queryLen; j++)
                {
                    if (_wcsnicmp(&g_shortcuts[i].name[j], query, queryLen) == 0)
                    {
                        sprintf(logMsg, "HandleShortcutSearch: 找到子字符串匹配 '%s'", itemNameLog);
                        LogToFile(logMsg);
                        
                        g_searchResults.push_back(g_shortcuts[i]);
                        break;
                    }
                }
            }
        }
    }
}

// 显示搜索结果到ListView
void DisplaySearchResults()
{
    char logMsg[1100] = {0};
    
    // If no results found
    if (g_searchResults.empty())
    {
        LogToFile("DisplaySearchResults: 未找到匹配项，显示'未找到匹配项'消息");
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)L"No matching items found";
        ListView_InsertItem(g_hListView, &lvi);
    }
    else
    {
        sprintf(logMsg, "DisplaySearchResults: 找到 %zu 个匹配项", g_searchResults.size());
        LogToFile(logMsg);
        
        // 显示所有搜索结果到ListView
        for (size_t i = 0; i < g_searchResults.size(); i++)
        {
            WCHAR display[1024] = {0};
            if (g_searchResults[i].type == 0) // Directory
                wsprintfW(display, L"DIR: %s", g_searchResults[i].name);
            else if (g_searchResults[i].type == 1) // URL
                wsprintfW(display, L"URL: %s", g_searchResults[i].name);
            else // Application
                wsprintfW(display, L"APP: %s", g_searchResults[i].name);
            
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
                sprintf(errorLog, "DisplaySearchResults: ListView_InsertItem失败，错误代码: %lu, ListView项目数: %d", 
                        error, currentItemCount);
                LogToFile(errorLog);
            }
            else
            {
                // 记录插入位置用于调试
                char insertLog[300] = {0};
                sprintf(insertLog, "DisplaySearchResults: 插入到ListView位置 %d, g_searchResults索引 %zu", 
                        actualIndex, i);
                LogToFile(insertLog);
                
                // 添加路径到第二列
                lvi.iSubItem = 1;
                lvi.pszText = g_searchResults[i].path;
                ListView_SetItem(g_hListView, &lvi);
            }
        }
        
        // 搜索完成后立即打印ListView内容用于调试
        LogToFile("DisplaySearchResults: 搜索完成，打印ListView和g_searchResults内容:");
        LogListViewContents();
    }
}

// 更新WebView2搜索相关内容
void UpdateWebViewForSearch(const std::vector<std::wstring>& webViewHints)
{
    // 无论是否有结果，WebView2 都显示提示信息和最新列表
    std::wstring html;
    CreateWebView2HTML(g_searchResults, webViewHints, html);
    UpdateWebView2Content(html.c_str());
}

void SearchAndDisplayResults(const WCHAR* query)
{
    // 初始化ListView
    if (!InitializeListViewForSearch())
    {
        return;
    }
    
    std::vector<std::wstring> webViewHints;
    
    // 处理搜索查询和模式判断
    ProcessSearchQuery(query, webViewHints);
    
    if (g_bookmarkMode)
    {
        // wz模式：只搜索网址收藏
        SearchBookmarks(query);
        
        // 将网址收藏搜索结果转换为ShortcutItem格式并添加到g_searchResults
        g_searchResults.clear();
        for (const auto& bookmark : g_bookmarkSearchResults)
        {
            ShortcutItem item;
            wcscpy_s(item.name, bookmark.first.c_str());
            wcscpy_s(item.path, bookmark.second.c_str());
            item.type = 1; // URL类型
            item.usageCount = 0;
            g_searchResults.push_back(item);
        }
    }
    else
    {
        // 普通模式：同时搜索快捷方式和网址收藏
        SearchBookmarks(query);
        HandleShortcutSearch(query);
    }
    
    // 显示搜索结果到ListView
    DisplaySearchResults();
    
    // 更新WebView2内容
    UpdateWebViewForSearch(webViewHints);
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

/**
 * @brief 处理WM_CREATE消息，创建窗口控件
 * @param hwnd 窗口句柄
 * @param lpCreateStruct 创建结构体指针
 * @return 成功返回0，失败返回-1
 */
LRESULT HandleWMCreate(HWND hwnd, LPCREATESTRUCTW lpCreateStruct)
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
    

    
    // Create calculator mode menu button (initially hidden)
    g_hCalcMenuButton = CreateWindowExW(
          0,
          L"BUTTON",
          L"操作 ▼",
          WS_CHILD | BS_PUSHBUTTON,
          200, 10, 80, 25,
          hwnd, (HMENU)IDC_CALC_MENU_BUTTON,
          g_hInstance, NULL);
    
    // Initially hide the exit calculator button and calculator menu button
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    ShowWindow(g_hCalcMenuButton, SW_HIDE);
    
    // 应用字体到所有控件
    if (g_hFont != NULL)
    {
        ApplyFontToControl(g_hEdit);
        ApplyFontToControl(g_hListView);
        ApplyFontToControl(g_hExitCalcButton);
        ApplyFontToControl(g_hCalcMenuButton);
        LogToFile("字体已应用到所有控件");
    }
    else
    {
        LogToFile("警告：字体句柄为空，无法应用字体");
    }
    
    return 0;
}

/**
 * @brief 处理WM_HOTKEY消息，处理全局热键
 * @param hwnd 窗口句柄
 * @param wParam 热键ID
 * @return 成功返回0
 */
LRESULT HandleWMHotkey(HWND hwnd, WPARAM wParam)
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

/**
 * @brief 处理WM_DESTROY消息，清理程序资源
 * @param hwnd 窗口句柄
 * @return 成功返回0
 */
LRESULT HandleWMDestroy(HWND hwnd)
{
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
}

/**
 * @brief 处理WM_TIMER消息，处理定时器事件
 * @param hwnd 窗口句柄
 * @param wParam 定时器ID
 * @return 成功返回0
 */
LRESULT HandleWMTimer(HWND hwnd, WPARAM wParam)
{
    // Timer is no longer needed since we're handling Enter key directly
    if (wParam == 1) {
        KillTimer(hwnd, 1);
        LogToFile("Timer killed - no longer needed for EN_RETURN handling");
    }
    return 0;
}

/**
 * @brief 处理WM_SIZE消息，处理窗口大小改变事件
 * @param hwnd 窗口句柄
 * @param wParam 大小调整类型
 * @param lParam 新的窗口大小
 * @return 成功返回0
 */
LRESULT HandleWMSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
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
    
    return 0;
}



/**
 * @brief 处理WM_SETFOCUS消息，处理窗口获得焦点事件
 * @param hwnd 窗口句柄
 * @param wParam 获得焦点的控件句柄
 * @return 默认窗口过程处理结果
 */
LRESULT HandleWMSetFocus(HWND hwnd, WPARAM wParam)
{
    // Log focus event with detailed information
    LogToFile("WM_SETFOCUS received for main window - setting ignore flag for next EN_RETURN");
    // Always set the ignore flag when the main window gets focus
    // This prevents EN_RETURN events from being triggered when clicking on the edit control
    g_ignoreNextReturn = true;
    LogToFile("  Setting ignore flag for next EN_RETURN due to focus change");
    // Allow normal focus behavior but ensure no auto-execution happens
    // Call default handler to ensure normal focus functionality
    return DefWindowProcW(hwnd, WM_SETFOCUS, wParam, 0);
}

/**
 * @brief 处理WM_NOTIFY消息，处理通知消息（如ListView双击事件）
 * @param hwnd 窗口句柄
 * @param wParam 控件ID
 * @param lParam 通知消息结构体指针
 * @return 处理结果或默认处理结果
 */
LRESULT HandleWMNotify(HWND hwnd, WPARAM wParam, LPARAM lParam)
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
    // 对于其他WM_NOTIFY消息，调用默认处理
    return DefWindowProcW(hwnd, WM_NOTIFY, wParam, lParam);
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return HandleWMCreate(hwnd, (LPCREATESTRUCTW)lParam);
            
        case WM_HOTKEY:
            return HandleWMHotkey(hwnd, wParam);
            
        case WM_DESTROY:
            return HandleWMDestroy(hwnd);
            
        case WM_TIMER:
            return HandleWMTimer(hwnd, wParam);
            
        case WM_SIZE:
            return HandleWMSize(hwnd, wParam, lParam);
            
        case WM_EXITSIZEMOVE:
            return HandleWMExitSizeMove(hwnd);
            
        case WM_TRAYICON:
            // 处理系统托盘图标消息
            HandleTrayMessage(lParam);
            return 0;
            
        case WM_SETFOCUS:
            return HandleWMSetFocus(hwnd, wParam);
            
        case WM_NOTIFY:
            return HandleWMNotify(hwnd, wParam, lParam);
            
        case WM_COMMAND:
            return HandleWMCommand(hwnd, wParam, lParam);

        case WM_KEYDOWN:
            return HandleWMKeyDown(hwnd, wParam, lParam);
            
        case WM_CONTEXTMENU:
            // 暂时不处理WM_CONTEXTMENU消息，返回默认处理
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
                        if (wcscmp(currentText, L"js") == 0 || wcscmp(currentText, L"wz") == 0 || wcscmp(currentText, L"dir") == 0 || wcscmp(currentText, L"set") == 0 || wcscmp(currentText, L"help") == 0)
                        {
                            LogToFile("  EditSubclassProc: 检测到特殊命令，调用ProcessCommand处理");
                            ProcessCommand(currentText);
                            return 0; // 特殊命令处理完成，不执行搜索结果
                        }
                        
                        // 检查是否在计算模式或目录浏览模式，优先处理"q"退出命令
                        if (g_calculatorMode || g_dirMode)
                        {
                            // 在特殊模式下，首先检查"q"退出命令
                            if (wcscmp(currentText, L"q") == 0)
                            {
                                LogToFile("  EditSubclassProc: 检测到特殊模式下输入'q'，退出当前模式");
                                if (g_calculatorMode)
                                {
                                    ExitCalculatorMode();
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
                            
                            // 检查是否在计算模式或目录浏览模式，优先处理"q"退出命令
                            if ((g_calculatorMode || g_dirMode) && wcscmp(currentText, L"q") == 0)
                            {
                                LogToFile("  EditSubclassProc: 检测到特殊模式下输入'q'，退出当前模式");
                                if (g_calculatorMode)
                                {
                                    ExitCalculatorMode();
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
    MessageBoxW(g_hMainWindow, L"快捷方式管理功能开发中...", L"快捷方式管理", MB_OK | MB_ICONINFORMATION);
}

// 显示系统设置对话框
void ShowSystemSettingsDialog() {
    LogToFile("ShowSystemSettingsDialog: 显示系统设置对话框");
    MessageBoxW(g_hMainWindow, L"系统设置功能开发中...", L"系统设置", MB_OK | MB_ICONINFORMATION);
}

// 显示关于对话框
void ShowAboutDialog() {
    LogToFile("ShowAboutDialog: 显示关于对话框");
    MessageBoxW(g_hMainWindow, 
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
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"WindowLeft", 0, REG_DWORD, (BYTE*)&windowRect.left, sizeof(DWORD));
            RegSetValueExW(hKey, L"WindowTop", 0, REG_DWORD, (BYTE*)&windowRect.top, sizeof(DWORD));
            DWORD windowWidth = (DWORD)(windowRect.right - windowRect.left);
            DWORD windowHeight = (DWORD)(windowRect.bottom - windowRect.top);
            RegSetValueExW(hKey, L"WindowWidth", 0, REG_DWORD, (BYTE*)&windowWidth, sizeof(DWORD));
            RegSetValueExW(hKey, L"WindowHeight", 0, REG_DWORD, (BYTE*)&windowHeight, sizeof(DWORD));
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
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue;
        DWORD dwSize = sizeof(DWORD);
        
        if (RegQueryValueExW(hKey, L"WindowWidth", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            width = dwValue;
        }
        if (RegQueryValueExW(hKey, L"WindowHeight", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            height = dwValue;
        }
        if (RegQueryValueExW(hKey, L"WindowLeft", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            x = dwValue;
        }
        if (RegQueryValueExW(hKey, L"WindowTop", 0, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
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
    // 从外部模板文件读取HTML内容
    std::wstring templatePath = L"data/webview_template.html";
    html = ReadHtmlTemplate(templatePath);
    
    // 如果读取失败，使用原来的硬编码HTML内容作为后备
    if (html.find(L"错误：无法加载模板文件") != std::wstring::npos)
    {
        LogToFile("CreateWebView2HTML: 模板文件读取失败，使用默认HTML内容");
        
        // 预分配内存，减少重新分配开销（估算：每个项目约100字符，基础HTML约2000字符）
        html.reserve(items.size() * 100 + 2000);
        html = L"模板文件读取失败，使用默认HTML内容";
    }
    else
    {
        // 替换模板中的占位符
        std::wstring hintsHtml;
        if (!hints.empty())
        {
            hintsHtml = L"<div class='hint-banner'><div class='banner-title'>💡 操作提示</div><ul>";
            for (const auto& hint : hints)
            {
                hintsHtml += L"<li>";
                hintsHtml += hint;
                hintsHtml += L"</li>";
            }
            hintsHtml += L"</ul></div>";
        }
        
        std::wstring itemsHtml;
        if (items.empty())
        {
            itemsHtml = L"<tr class='empty-row'><td colspan='2'>未找到匹配项，试试其他关键字，或输入 <strong>help</strong> 查看可用命令。</td></tr>";
        }
        else
        {
            for (size_t i = 0; i < items.size(); i++)
            {
                itemsHtml += L"<tr class='item-row' onclick='onRowClick(";
                itemsHtml += std::to_wstring(i);
                itemsHtml += L")' ondblclick='onRowDblClick(";
                itemsHtml += std::to_wstring(i);
                itemsHtml += L")'>";
                itemsHtml += L"<td>";
                itemsHtml += items[i].name;
                itemsHtml += L"</td><td>";
                itemsHtml += items[i].path;
                itemsHtml += L"</td></tr>";
            }
        }
        
        // 替换占位符
        ReplaceStringInPlace(html, L"<!-- HINTS_PLACEHOLDER -->", hintsHtml);
        ReplaceStringInPlace(html, L"<!-- ITEMS_PLACEHOLDER -->", itemsHtml);
    }
}

void UpdateCalculatorModeWebView()
{
    if (!g_webView)
    {
        LogToFile("UpdateCalculatorModeWebView: WebView2 未初始化，无法显示计算模式内容");
        return;
    }
    
    // 从外部模板文件读取HTML内容
    std::wstring templatePath = L"data/calculator_template.html";
    std::wstring html = ReadHtmlTemplate(templatePath);
    
    // 如果读取失败，使用原来的硬编码HTML内容作为后备
    if (html.find(L"错误：无法加载模板文件") != std::wstring::npos)
    {
        LogToFile("UpdateCalculatorModeWebView: 模板文件读取失败，使用默认HTML内容");
        
        // 生成默认的计算模式HTML内容
        html = L"<!DOCTYPE html><html><head><meta charset='UTF-8'>";
        html += L"<style>";
        html += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 10px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #f5f8ff; }";
        html += L".mode-banner { background: linear-gradient(90deg, #4a90e2, #357abd); padding: 16px; border-radius: 10px; font-size: 18px; font-weight: bold; box-shadow: 0 4px 12px rgba(0,0,0,0.3); margin-bottom: 20px; }";
        html += L".hint-banner { background: rgba(255,255,255,0.15); border-left: 4px solid #FFD700; padding: 12px 16px; margin-bottom: 20px; border-radius: 6px; }";
        html += L".history-container { background: rgba(255,255,255,0.95); border-radius: 10px; padding: 20px; box-shadow: 0 4px 12px rgba(0,0,0,0.2); }";
        html += L"table { width: 100%; border-collapse: collapse; }";
        html += L"th { background: #4a90e2; color: white; padding: 12px; text-align: left; }";
        html += L"td { padding: 10px; border-bottom: 1px solid #ddd; }";
        html += L".history-row:hover { background: #f5f5f5; cursor: pointer; }";
        html += L".empty { text-align: center; padding: 40px; color: #666; font-size: 16px; }";
        html += L"</style>";
        html += L"<script>";
        html += L"function onHistoryRowClick(index) {";
        html += L"  if (window.chrome && window.chrome.webview) {";
        html += L"    window.chrome.webview.postMessage(JSON.stringify({type:'calculatorHistory', index:index}));";
        html += L"  }";
        html += L"}";
        html += L"</script>";
        html += L"</head><body>";
        
        html += L"<div class='mode-banner'>🧮 计算模式 (js) · 输入数学表达式进行计算</div>";
        html += L"<div class='hint-banner'>💡 提示：支持加减乘除运算，以#开头添加注释，输入'q'退出计算模式</div>";
        html += L"<div class='history-container'>";
        
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
    }
    else
    {
        // 替换模板中的占位符
        std::wstring historyHtml;
        if (g_calculationHistory.empty())
        {
            historyHtml = L"<div class='empty'>暂无计算记录，试着输入表达式开始计算吧。</div>";
        }
        else
        {
            historyHtml = L"<table><thead><tr><th>表达式</th><th>结果</th><th>备注</th></tr></thead><tbody>";
            for (size_t i = g_calculationHistory.size(); i > 0; --i)
            {
                size_t displayIndex = g_calculationHistory.size() - i;
                historyHtml += L"<tr class='history-row' onclick='onHistoryRowClick(";
                historyHtml += std::to_wstring(displayIndex);
                historyHtml += L")'><td>";
                historyHtml += g_calculationHistory[i - 1].expression;
                historyHtml += L"</td><td>";
                historyHtml += g_calculationHistory[i - 1].result;
                historyHtml += L"</td><td>";
                if (g_calculationHistory[i - 1].comment.empty())
                {
                    historyHtml += L"-";
                }
                else
                {
                    historyHtml += g_calculationHistory[i - 1].comment;
                }
                historyHtml += L"</td></tr>";
            }
            historyHtml += L"</tbody></table>";
        }
        ReplaceStringInPlace(html, L"<!-- HISTORY_PLACEHOLDER -->", historyHtml);
    }
    
    UpdateWebView2Content(html.c_str());
}

/**
 * @brief 更新设置菜单WebView显示
 * 
 * 此函数更新WebView2中设置菜单的显示内容
 */
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
        // 从外部模板文件读取HTML内容
        std::wstring templatePath = L"data/settings_template.html";
        g_cachedSettingsHtml = ReadHtmlTemplate(templatePath);
        
        // 如果读取失败，使用原来的硬编码HTML内容作为后备
        if (g_cachedSettingsHtml.find(L"错误：无法加载模板文件") != std::wstring::npos)
        {
            LogToFile("UpdateSettingsMenuWebView: 模板文件读取失败，使用默认HTML内容");
            
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
                L"快捷方式管理",
                L"系统设置",
                L"关于软件"
            };
            
            const WCHAR* menuIcons[] = {
                L"🚪",
                L"📁",
                L"⚙️",
                L"ℹ️"
            };
            
            for (int i = 0; i < 4; i++)
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
        }
        else
        {
            // 模板文件读取成功，需要替换占位符
            LogToFile("UpdateSettingsMenuWebView: 模板文件读取成功，开始替换占位符");
            
            // 生成菜单项HTML内容
            std::wstring menuItemsHtml;
            
            // 菜单项列表
            const WCHAR* menuItems[] = {
                L"退出程序",
                L"快捷方式管理",
                L"系统设置",
                L"关于软件"
            };
            
            const WCHAR* menuIcons[] = {
                L"🚪",
                L"📁",
                L"⚙️",
                L"ℹ️"
            };
            
            for (int i = 0; i < 4; i++)
            {
                menuItemsHtml += L"<div class='menu-item' onclick='onMenuItemClick(";
                menuItemsHtml += std::to_wstring(i);
                menuItemsHtml += L")' ondblclick='onMenuItemClick(";
                menuItemsHtml += std::to_wstring(i);
                menuItemsHtml += L")'>";
                menuItemsHtml += L"<span class='menu-icon'>";
                menuItemsHtml += menuIcons[i];
                menuItemsHtml += L"</span>";
                menuItemsHtml += menuItems[i];
                menuItemsHtml += L"</div>";
            }
            
            // 替换模板中的占位符
            ReplaceStringInPlace(g_cachedSettingsHtml, L"<!-- MENU_ITEMS_PLACEHOLDER -->", menuItemsHtml);
            LogToFile("UpdateSettingsMenuWebView: 占位符替换完成");
        }
        
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
        }
        
        g_helpHtmlCached = true;
        LogToFile("UpdateHelpInfoWebView: 帮助信息HTML已缓存");
    }
    
    UpdateWebView2Content(g_cachedHelpHtml.c_str());
}



// 显示基本用法界面
void ShowBasicUsage()
{
    LogToFile("ShowBasicUsage: 创建基本用法HTML内容");
    
    // 尝试从外部模板文件读取HTML内容
    std::wstring htmlContent = ReadHtmlTemplate(L"data/basic_usage_template.html");
    
    // 如果模板读取失败，使用硬编码的HTML内容作为后备方案
    if (htmlContent.empty())
    {
        LogToFile("ShowBasicUsage: 模板文件读取失败，使用硬编码HTML内容");
        
        htmlContent = L"模板文件读取失败，使用硬编码HTML内容";
    }
    
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

/**
 * @brief 替换字符串中的子字符串（原地替换）
 * @param str 要替换的字符串
 * @param from 要查找的子字符串
 * @param to 要替换为的子字符串
 */
void ReplaceStringInPlace(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::wstring::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}






/**
 * @brief 复制文本到剪贴板
 * @param text 要复制的文本
 */
void CopyToClipboard(const std::wstring& text)
{
    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        
        // 分配全局内存
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
        if (hMem)
        {
            wchar_t* pMem = static_cast<wchar_t*>(GlobalLock(hMem));
            wcscpy_s(pMem, text.length() + 1, text.c_str());
            GlobalUnlock(hMem);
            
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        
        CloseClipboard();
    }
}

/**
 * @brief 清空ListView控件中的所有项目
 */
void ClearListView()
{
    if (g_hListView)
    {
        ListView_DeleteAllItems(g_hListView);
        LogToFile("ClearListView: ListView已清空");
    }
    else
    {
        LogToFile("ClearListView: ListView句柄为空，无法清空");
    }
}

// 全局变量定义
HWND g_hExitBookmarkButton = NULL;



