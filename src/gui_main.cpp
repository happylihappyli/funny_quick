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
#include <commdlg.h>   // 通用对话框
#include "resource.h"
#include "logger.h"
#include "common.h"  // 公共定义和声明
#include "calculator.h"  // 计算器功能定义
#include "webview_manager.h"  // WebView2 管理功能
#include "dir_mode_manager.h"  // 目录浏览模式管理功能
#include "window_size_handler.h"  // 窗口大小处理功能
#include "bookmark_manager.h"  // 网址收藏管理功能
#include "file_search_manager.h"  // 文件搜索管理功能
#include "file_manager.h"  // 文件模式管理功能
#include "tray_icon_manager.h" // 托盘图标管理功能
#include "ui_helpers.h" // UI辅助功能
#include "command_processor.h" // 命令处理功能
#include <WebView2.h>
#include <wrl.h>  // 用于 Microsoft::WRL::Callback
#include <wrl/event.h>  // 用于事件处理器
using namespace Microsoft::WRL;

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "imm32.lib") // 链接输入法库

// Enable Visual Styles
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Define notification codes if not defined
// EN_RETURN 定义已移至 common.h，避免与 EN_SETFOCUS 冲突

// Global variables
HINSTANCE g_hInstance = NULL;
HWND g_hMainWindow = NULL;
HWND g_hEdit = NULL;
HWND g_hListView = NULL;  // ListView控件（保留用于兼容）
HWND g_hExitCalcButton = NULL;  // 退出计算模式按钮
HWND g_hSettingsButton = NULL;   // 设置按钮

HWND g_hCalcMenuButton = NULL;  // 计算模式操作菜单按钮

// 工具栏按钮句柄
HWND g_hHomeBtn = NULL;
HWND g_hBookmarkBtn = NULL;
HWND g_hCalculatorBtn = NULL;
HWND g_hDirBtn = NULL;
HWND g_hFileBtn = NULL;      // 文件搜索模式按钮
HWND g_hShortcutBtn = NULL;  // 快捷方式管理模式按钮
// HIMAGELIST g_hImageList = NULL; // 全局图像列表 (已废弃，改为每按钮一个)
std::vector<HIMAGELIST> g_toolbarImageLists; // 存储每个按钮的图像列表

// Flag to ignore EN_RETURN notifications triggered by focus changes
bool g_ignoreNextReturn = false;
// WebView2 HTML内容缓存
std::wstring g_cachedHelpHtml;  // 缓存的帮助信息HTML
std::wstring g_cachedSettingsHtml;  // 缓存的设置菜单HTML
bool g_helpHtmlCached = false;  // 帮助信息是否已缓存
bool g_settingsHtmlCached = false;  // 设置菜单是否已缓存
std::wstring g_cachedWebViewHtml;  // 缓存的快捷方式列表模板HTML
bool g_webViewHtmlCached = false;  // 快捷方式列表模板是否已缓存
std::wstring g_cachedCalculatorHtml;  // 缓存的计算器模板HTML
bool g_calculatorHtmlCached = false;  // 计算器模板是否已缓存
std::wstring g_cachedBasicUsageHtml;  // 缓存的基本用法模板HTML
bool g_basicUsageHtmlCached = false;  // 基本用法模板是否已缓存
std::wstring g_cachedFileModeHtml;  // 缓存的文件模式模板HTML
bool g_fileModeHtmlCached = false;  // 文件模式模板是否已缓存
std::wstring g_cachedDirModeHtml;  // 缓存的目录模式模板HTML
bool g_dirModeHtmlCached = false;  // 目录模式模板是否已缓存
std::wstring g_cachedAddBookmarkHtml;  // 缓存的添加书签对话框模板HTML
bool g_addBookmarkHtmlCached = false;  // 添加书签对话框模板是否已缓存
std::wstring g_cachedEditBookmarkHtml;  // 缓存的编辑书签对话框模板HTML
bool g_editBookmarkHtmlCached = false;  // 编辑书签对话框模板是否已缓存
std::wstring g_cachedBookmarkHtml;  // 缓存的书签模式模板HTML
bool g_bookmarkHtmlCached = false;  // 书签模式模板是否已缓存
std::wstring g_cachedShortcutEditHtml;  // 缓存的快捷方式编辑/添加模板HTML
bool g_shortcutEditHtmlCached = false;  // 快捷方式编辑/添加模板是否已缓存
std::wstring g_cachedFormulaManagerHtml;  // 缓存的公式管理模板HTML
bool g_formulaManagerHtmlCached = false;  // 公式管理模板是否已缓存
bool g_formulaManagerMode = false; // 公式管理模式标志
bool g_windowInitializing = false;  // 窗口是否正在初始化，防止自动执行
bool g_minimizeToTray = true;  // 是否在最小化时隐藏到托盘
bool g_showStartPageOnLaunch = true;  // 显示窗口时是否优先展示开始页

// Subclassing procedure pointer for edit control
WNDPROC g_originalEditProc = NULL;

// Edit control subclassing procedure declaration
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// Constants
#define IDC_EDIT 1001
#define IDC_LISTVIEW 1002  // ListView控件ID，支持双列显示
#define IDC_EXIT_CALC_BUTTON 1003  // 退出计算模式按钮ID
#define IDC_SETTINGS_BUTTON 1004    // 设置按钮ID
#define IDC_EXIT_FILE_BUTTON 1017   // 退出文件模式按钮ID

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
#define ID_CONTEXT_EDIT_SHORTCUT 1009  // 编辑快捷方式
#define ID_CONTEXT_DELETE_SHORTCUT 1010  // 删除快捷方式
#define ID_CONTEXT_ADD_SHORTCUT 1011  // 添加快捷方式

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

// 文件搜索管理相关全局变量定义
bool g_fileMode = false;  // 文件模式标志
FileSearchManager g_fileSearchManager; // 文件搜索管理器实例
std::vector<FileSearchResult> g_fileSearchResults; // 文件搜索结果列表
HWND g_hExitFileButton = NULL; // 退出文件模式按钮
UINT_PTR g_fileSearchTimerId = 0;  // 文件搜索定时器ID
WCHAR g_pendingFileSearchQuery[1024] = {0};  // 待处理的文件搜索查询

// 表达式解析辅助函数声明
void EnterCalculatorMode();
void ExitCalculatorMode();
void EnterFileMode();
void ExitFileMode();
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
void UpdateWindowTitle(); // 根据当前模式更新窗口标题

// 收藏相关函数声明
void CopyToClipboard(const std::wstring& text);
LRESULT HandleWMContextMenu(HWND hwnd, WPARAM wParam);



// 窗口消息处理函数声明
LRESULT HandleWMKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);

// ListView相关函数声明
void ClearListView();

// 窗口大小记忆功能函数声明
void SaveWindowSettings();
void LoadWindowSettings(int& x, int& y, int& width, int& height);
void SaveAppSettings();
void LoadAppSettings();

// HTML模板读取辅助函数声明


// 日志功能已移至 logger.cpp

// Forward declarations
void ShowLauncherWindow();
void HideLauncherWindow();
void SetEnglishInputMethod();


// WebView2 相关函数声明
void InitializeWebView2(HWND hwnd);  // 初始化 WebView2
void UpdateWebView2Content(const WCHAR* htmlContent);  // 更新 WebView2 内容
void CreateWebView2HTML(const std::vector<ShortcutItem>& items, const std::vector<std::wstring>& hints, std::wstring& html);  // 创建 HTML 内容
void UpdateCalculatorModeWebView();  // 刷新计算模式的 WebView2 显示
void UpdateSettingsMenuWebView();  // 刷新设置菜单的 WebView2 显示
void UpdateHelpInfoWebView();  // 刷新帮助信息的 WebView2 显示
void UpdateFileModeWebView();  // 刷新文件模式的 WebView2 显示
void ShowBasicUsage();  // 显示基本用法界面
void RefreshCurrentView();  // 恢复当前页面显示







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
    
    // 普通模式下优先显示开始页，其他模式恢复当前内容
    if (!g_calculatorMode && !g_dirMode && !g_bookmarkMode && !g_fileMode)
    {
        if (g_showStartPageOnLaunch)
        {
            LogToFile("ShowLauncherWindow: 显示开始页");
            UpdateInitialWebViewContent();
        }
        else
        {
            LogToFile("ShowLauncherWindow: 显示基础说明页");
            ShowBasicUsage();
        }
    }
    else
    {
        LogToFile("ShowLauncherWindow: 恢复当前模式页面");
        RefreshCurrentView();
    }
    
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
    // 用户反馈需要输入中文，因此移除了强制切换英文和禁用输入法的代码
    // 改为确保输入法可用
    if (g_hEdit)
    {
        HIMC hIMC = ImmGetContext(g_hEdit);
        if (hIMC)
        {
            ImmSetOpenStatus(hIMC, TRUE);
            ImmReleaseContext(g_hEdit, hIMC);
            LogToFile("SetEnglishInputMethod: 已确保输入法开启");
        }
        else
        {
            LogToFile("SetEnglishInputMethod: 获取输入法上下文失败，尝试重新关联");
            // 尝试获取默认IME窗口并关联
            // 注意：这里不做过多干预，避免副作用，只是记录日志
        }
    }
}









// Search processing functions moved to command_processor.cpp








/**
 * @brief 处理WM_CREATE消息，创建窗口控件
 * @param hwnd 窗口句柄
 * @param lpCreateStruct 创建结构体指针
 * @return 成功返回0，失败返回-1
 */
// 设置按钮图标的函数
/**
 * @brief 为工具栏按钮设置图标
 * @param hwnd 窗口句柄
 */
void SetupToolbarButtonIcons(HWND hwnd)
{
    // 清理旧的 ImageLists
    for (auto himl : g_toolbarImageLists) {
        ImageList_Destroy(himl);
    }
    g_toolbarImageLists.clear();

    // 获取系统目录路径
    WCHAR systemDir[MAX_PATH];
    GetSystemDirectoryW(systemDir, MAX_PATH);
    
    // 组合完整路径
    WCHAR iconPath[MAX_PATH];
    wcscpy_s(iconPath, systemDir);
    wcscat_s(iconPath, L"\\shell32.dll");

    WCHAR calcPath[MAX_PATH];
    wcscpy_s(calcPath, systemDir);
    wcscat_s(calcPath, L"\\calc.exe");
    
    HICON hIconLarge = NULL;
    HICON hIconSmall = NULL;

    // 辅助lambda：从指定路径获取图标并添加到新创建的ImageList中
    auto CreateListWithIcon = [&](const WCHAR* path, int index) -> HIMAGELIST {
        HIMAGELIST hList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 1, 0);
        if (!hList) return NULL;

        if (ExtractIconExW(path, index, &hIconLarge, &hIconSmall, 1) > 0) {
            if (hIconLarge) {
                ImageList_AddIcon(hList, hIconLarge);
                DestroyIcon(hIconLarge);
            } else if (hIconSmall) {
                // Fallback to small if large not available (unlikely for 32x32 request but safe)
                ImageList_AddIcon(hList, hIconSmall);
            }
            if (hIconSmall) DestroyIcon(hIconSmall);
        } else {
            // Fallback: try default icon if extraction fails
            // Just return empty list or list with system default?
            // For now, if fail, the list is empty and button won't show icon.
        }
        return hList;
    };

    // 定义按钮配置
    struct ButtonConfig {
        HWND hBtn;
        const WCHAR* path;
        int iconIndex;
    } configs[] = {
        { g_hHomeBtn, iconPath, 238 },      // 首页
        { g_hBookmarkBtn, iconPath, 43 },   // 收藏
        { g_hCalculatorBtn, calcPath, 0 },  // 计算器
        { g_hDirBtn, iconPath, 3 },         // 目录
        { g_hFileBtn, iconPath, 22 },       // 文件
        { g_hShortcutBtn, iconPath, 25 }    // 快捷
    };

    // 为每个按钮创建独立的 ImageList
    for (const auto& cfg : configs) {
        HIMAGELIST hList = NULL;
        
        // 特殊处理计算器：先试 calc.exe，失败试 shell32
        if (cfg.hBtn == g_hCalculatorBtn) {
            hList = CreateListWithIcon(cfg.path, cfg.iconIndex);
            if (ImageList_GetImageCount(hList) == 0) {
                ImageList_Destroy(hList);
                hList = CreateListWithIcon(iconPath, 24); // Fallback
            }
        } else {
            hList = CreateListWithIcon(cfg.path, cfg.iconIndex);
        }

        if (hList && ImageList_GetImageCount(hList) > 0) {
            g_toolbarImageLists.push_back(hList); // 存入全局列表以便清理

            BUTTON_IMAGELIST bil = {0};
            bil.himl = hList;
            bil.uAlign = BUTTON_IMAGELIST_ALIGN_TOP;
            bil.margin = {0, 2, 0, 0};
            
            SendMessage(cfg.hBtn, BCM_SETIMAGELIST, 0, (LPARAM)&bil);
            
            // 尝试设置 Explorer 主题以修复可能的绘制问题
            // 需要 uxtheme.h, 但如果不想引入依赖，可以尝试 SetWindowTheme
            // 这里假设 SetWindowTheme 可用 (user32.dll / uxtheme.dll)
            // 实际上 SetWindowTheme 需要链接 uxtheme.lib 并包含 uxtheme.h
            // 为了避免编译错误，先不加 SetWindowTheme，因为独立的 ImageList 应该能解决索引混淆问题。
            
            InvalidateRect(cfg.hBtn, NULL, TRUE);
            UpdateWindow(cfg.hBtn);
        } else {
            if (hList) ImageList_Destroy(hList);
        }
    }
    
    LogToFile("Toolbar icons setup complete using separate ImageLists");
}

/**
 * @brief 处理WM_CREATE消息，创建窗口控件
 * @param hwnd 窗口句柄
 * @param lpCreateStruct 创建结构体指针
 * @return 成功返回0，失败返回-1
 */
LRESULT HandleWMCreate(HWND hwnd, LPCREATESTRUCTW lpCreateStruct)
{
    // 文本框位置调整：移除左边标签，文本框靠左显示
    g_hEdit = CreateWindowExW(
          0,
          WC_EDITW,
          L"",
          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
          10, 10, 280, 25,  // 调整位置和宽度：x=10, y=10, 宽度=280
          hwnd, (HMENU)IDC_EDIT,
          g_hInstance, NULL);
    
    // 确保编辑框关联输入法上下文
    HIMC hIMC = ImmGetContext(g_hEdit);
    if (hIMC)
    {
        ImmAssociateContext(g_hEdit, hIMC);
        ImmReleaseContext(g_hEdit, hIMC);
        LogToFile("HandleWMCreate: 已关联输入法上下文到搜索框");
    }
    else
    {
        LogToFile("HandleWMCreate: 无法获取输入法上下文");
    }
    
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
          10, 120, 360, 200,  // 初始位置与 LayoutControls 保持一致 (10 + 50 + 10 + 50)
          hwnd, NULL,
          g_hInstance, NULL);
    
    LogToFile("WebView2 占位窗口已创建");
    
    // 创建工具栏（确保在 WebView2 之后创建，这样工具栏会显示在顶部）
    // 增加高度以容纳更大的按钮 (30 -> 80)
    HWND hToolBar = CreateWindowExW(
        0,
        L"BUTTON",
        L"",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        10, 40, 360, 80,
        hwnd, NULL,
        g_hInstance, NULL);
    
    // 按钮参数调整：
    // 宽度 50 -> 55
    // 高度 25 -> 60
    // Y坐标 45 -> 55 (Groupbox从40开始，内部留边距)
    // 移除 BS_ICON 样式，以便同时显示文字 (将使用 ImageList 关联图标)
    
    // 创建首页按钮
    g_hHomeBtn = CreateWindowExW(
        0,
        L"BUTTON",
        L"首页",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        15, 55, 55, 60,
        hwnd, (HMENU)IDC_HOME_BTN,
        g_hInstance, NULL);
    
    // 创建收藏按钮
    g_hBookmarkBtn = CreateWindowExW(
        0,
        L"BUTTON",
        L"收藏",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        75, 55, 55, 60,
        hwnd, (HMENU)IDC_BOOKMARK_BTN,
        g_hInstance, NULL);
    
    // 创建计算器按钮
    g_hCalculatorBtn = CreateWindowExW(
        0,
        L"BUTTON",
        L"计算器",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        135, 55, 55, 60,
        hwnd, (HMENU)IDC_CALCULATOR_BTN,
        g_hInstance, NULL);
    
    // 创建目录按钮
    g_hDirBtn = CreateWindowExW(
        0,
        L"BUTTON",
        L"目录",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        195, 55, 55, 60,
        hwnd, (HMENU)IDC_DIR_BTN,
        g_hInstance, NULL);
    
    // 创建文件按钮
    g_hFileBtn = CreateWindowExW(
        0,
        L"BUTTON",
        L"文件",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        255, 55, 55, 60,
        hwnd, (HMENU)IDC_FILE_BTN,
        g_hInstance, NULL);
    
    // 创建快捷方式管理按钮
    g_hShortcutBtn = CreateWindowExW(
        0,
        L"BUTTON",
        L"快捷",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
        315, 55, 55, 60,
        hwnd, (HMENU)IDC_SHORTCUT_BTN,
        g_hInstance, NULL);
    
    // 确保工具栏按钮始终显示在所有控件之上（Z 顺序的顶部）
    SetWindowPos(g_hHomeBtn, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(g_hBookmarkBtn, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(g_hCalculatorBtn, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(g_hDirBtn, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(g_hFileBtn, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(g_hShortcutBtn, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    
    // 初始化 WebView2（异步创建，需要时间）
    InitializeWebView2(hwnd);
    
    // 设置工具栏按钮图标
    SetupToolbarButtonIcons(hwnd);
    
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
    
    // Create exit file mode button (initially hidden)
    g_hExitFileButton = CreateWindowExW(
          0,
          L"BUTTON",
          L"退出文件",
          WS_CHILD | BS_PUSHBUTTON,
          300, 10, 80, 25,
          hwnd, (HMENU)IDC_EXIT_FILE_BUTTON,
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
    
    // Initially hide the exit calculator button, exit file button and calculator menu button
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    ShowWindow(g_hExitFileButton, SW_HIDE);
    ShowWindow(g_hCalcMenuButton, SW_HIDE);
    
    // 应用字体到所有控件
    if (g_hFont != NULL)
    {
        ApplyFontToControl(g_hEdit);
        ApplyFontToControl(g_hListView);
        ApplyFontToControl(g_hExitCalcButton);
        ApplyFontToControl(g_hExitFileButton);
        ApplyFontToControl(g_hCalcMenuButton);
        ApplyFontToControl(g_hHomeBtn);
        ApplyFontToControl(g_hBookmarkBtn);
        ApplyFontToControl(g_hCalculatorBtn);
        ApplyFontToControl(g_hDirBtn);
        ApplyFontToControl(g_hFileBtn);
        ApplyFontToControl(g_hShortcutBtn);
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
        // 如果窗口可见且非最小化，则隐藏
        if (IsWindowVisible(hwnd) && !IsIconic(hwnd))
        {
            ShowWindow(hwnd, SW_HIDE);
        }
        else
        {
            // 如果是最小化状态，先还原
            if (IsIconic(hwnd))
            {
                ShowWindow(hwnd, SW_RESTORE);
            }
            
            ShowLauncherWindow();
            
            // 确保窗口在屏幕可见区域内
            // 获取当前窗口位置和大小
            RECT rc;
            GetWindowRect(hwnd, &rc);
            int width = rc.right - rc.left;
            int height = rc.bottom - rc.top;
            
            // 获取屏幕尺寸
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            
            // 重新计算居中位置
            int x = (screenWidth - width) / 2;
            int y = (screenHeight - height) / 2;
            
            // 强制设置窗口位置，确保在桌面上可见
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
            LogToFile("Ctrl+Alt+Q pressed: Window positioned to center to ensure visibility");
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
    SaveAppSettings();
    
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
    
    // 销毁所有工具栏图像列表
    for (auto himl : g_toolbarImageLists)
    {
        ImageList_Destroy(himl);
    }
    g_toolbarImageLists.clear();

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
    // 处理文件搜索定时器（ID为2）
    if (wParam == 2) {
        LogToFile("WM_TIMER: 文件搜索定时器触发");
        
        // 取消定时器
        KillTimer(hwnd, 2);
        g_fileSearchTimerId = 0;
        
        // 检查是否有待处理的搜索查询
        if (wcslen(g_pendingFileSearchQuery) > 0) {
            char logMsg[512] = {0};
            char searchTextLog[256] = {0};
            WideCharToMultiByte(CP_UTF8, 0, g_pendingFileSearchQuery, -1, searchTextLog, sizeof(searchTextLog), NULL, NULL);
            sprintf(logMsg, "WM_TIMER: 执行延迟文件搜索，查询: '%s'", searchTextLog);
            LogToFile(logMsg);
            
            // 执行文件搜索
            SearchFiles(g_pendingFileSearchQuery);
            
            // 清空待处理的查询
            g_pendingFileSearchQuery[0] = L'\0';
            
            LogToFile("WM_TIMER: 延迟文件搜索完成");
        } else {
            LogToFile("WM_TIMER: 没有待处理的文件搜索查询");
        }
        
        return 0;
    }
    
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
    
    // 当窗口最小化时按设置决定是否隐藏到托盘
    if (wParam == SIZE_MINIMIZED && g_minimizeToTray)
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
    else if (wParam == SIZE_MINIMIZED)
    {
        LogToFile("WM_SIZE: 窗口最小化，保留任务栏窗口");
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
    
    // 如果是计算器模式，可能需要确保不触发不必要的计算逻辑
    // 但核心的计算逻辑现在只在 EN_RETURN 中触发，所以这里设置 ignore 主要是防止回车误触（虽然通常是点击导致的）
    // 对于文本变化导致的自动计算，我们已经在 HandleEditControlChange 中屏蔽了计算模式下的自动计算

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
                    // 检查是否是文件模式
                    if (g_fileMode)
                    {
                        LogToFile("WM_NOTIFY: 文件模式下双击执行选中的文件");
                        // 在文件模式下，双击执行文件或打开文件夹
                        ExecuteFileModeItem(selIndex);
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

        case WM_APP_WEBVIEW_READY:
        {
            // WebView2准备就绪，现在可以安全地进行UI更新
            LogToFile("WM_APP_WEBVIEW_READY: WebView2 is ready, performing initial UI setup");
            ShowHelpInfo();
            std::vector<std::wstring> hints;
            ProcessSearchQuery(L"", hints);
            return 0;
        }
            
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
            return HandleWMContextMenu(hwnd, wParam);
            
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
        sprintf(logMsg, "CoInitializeEx failed: 0x%08lX", hrCoInit);
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
    
    // 初始化文件搜索管理器
    if (g_fileSearchManager.Initialize()) {
        LogToFile("文件搜索管理器初始化成功");
    } else {
        LogToFile("文件搜索管理器初始化失败");
    }
    
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
    
    // 如果加载自定义图标失败，使用资源文件中的图标作为备选
    if (!wc.hIcon) {
        wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    }
    if (!wc.hIconSm) {
        wc.hIconSm = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
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
    int windowHeight = 800;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    // 从注册表加载窗口设置
    LoadWindowSettings(x, y, windowWidth, windowHeight);
    LoadAppSettings();
    
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
    
    // 立即设置英文输入法
    SetEnglishInputMethod();
    
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
    
    // 先显示默认页面，避免启动时空白
    if (g_hListView)
    {
        if (g_showStartPageOnLaunch)
        {
            LogToFile("显示启动开始页占位");
            ShowBasicUsage();
        }
        else
        {
            LogToFile("显示启动帮助信息");
            ShowHelpInfo();
        }
    }
    
    // 然后初始化快捷方式（延迟初始化，确保WebView2已准备好）
    LogToFile("初始化快捷方式");
    InitializeCommonShortcuts();
    
    // 初始化完成后，显示首页快捷方式（如果存在）
    if (g_hListView)
    {
        LogToFile("显示首页快捷方式");
        UpdateInitialWebViewContent();
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
static void HandleReturnKey(HWND hwnd)
{
    LogToFile("EditSubclassProc: WM_KEYDOWN with VK_RETURN received");
    WCHAR currentText[1024] = {0};
    GetWindowTextW(hwnd, currentText, sizeof(currentText)/sizeof(WCHAR));
    char currentTextLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, currentText, -1, currentTextLog, sizeof(currentTextLog), NULL, NULL);
    char enterLog[1100] = {0};
    sprintf(enterLog, "  EditSubclassProc: Current edit text: '%s'", currentTextLog);
    LogToFile(enterLog);
    LogToFile("  EditSubclassProc: 回车键按下，打印ListView内容:");
    LogListViewContents();
    int itemCount = ListView_GetItemCount(g_hListView);
    char logMsg[200] = {0};
    sprintf(logMsg, "  EditSubclassProc: Listbox item count: %d", itemCount);
    LogToFile(logMsg);
    if (itemCount > 0)
    {
        if (wcscmp(currentText, L"js") == 0 || wcscmp(currentText, L"wz") == 0 || wcscmp(currentText, L"dir") == 0 || wcscmp(currentText, L"file") == 0 || wcscmp(currentText, L"set") == 0 || wcscmp(currentText, L"help") == 0)
        {
            LogToFile("  EditSubclassProc: 检测到特殊命令，调用ProcessCommand处理");
            ProcessCommand(currentText);
            return;
        }
        if (g_calculatorMode || g_dirMode || g_bookmarkMode || g_fileMode)
        {
            if (wcscmp(currentText, L"q") == 0)
            {
                LogToFile("  EditSubclassProc: 检测到特殊模式下输入'q'，退出当前模式");
                if (g_calculatorMode) ExitCalculatorMode();
                if (g_dirMode) ExitDirMode();
                if (g_bookmarkMode) ExitBookmarkMode();
                if (g_fileMode) ExitFileMode();
            }
            else if (g_bookmarkMode)
            {
                LogToFile("  EditSubclassProc: WZ模式下用户按回车键，打开第一个网址收藏");
                INT_PTR firstSelIndex = 0;
                const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                if (!displayBookmarks.empty() && firstSelIndex < (INT_PTR)displayBookmarks.size())
                {
                    ShellExecuteW(NULL, L"open", displayBookmarks[firstSelIndex].second.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    char logMsg2[500] = {0};
                    char nameLog[256] = {0};
                    char urlLog[256] = {0};
                    WideCharToMultiByte(CP_UTF8, 0, displayBookmarks[firstSelIndex].first.c_str(), -1, nameLog, sizeof(nameLog), NULL, NULL);
                    WideCharToMultiByte(CP_UTF8, 0, displayBookmarks[firstSelIndex].second.c_str(), -1, urlLog, sizeof(urlLog), NULL, NULL);
                    sprintf(logMsg2, "  EditSubclassProc: 已打开网址收藏[%Id] '%s' -> '%s'", firstSelIndex, nameLog, urlLog);
                    LogToFile(logMsg2);
                }
                else
                {
                    LogToFile("  EditSubclassProc: WZ模式下没有可用的网址收藏");
                }
            }
            else if (g_fileMode)
            {
                LogToFile("  EditSubclassProc: 文件模式下用户按回车键，进行文件搜索");
                SearchFiles(currentText);
            }
            else if (g_calculatorMode)
            {
                LogToFile("  EditSubclassProc: 计算模式下，忽略列表项，调用EvaluateExpression");
                EvaluateExpression(currentText);
            }
            return;
        }
        INT_PTR firstSelIndex = GetFirstActualItemIndex();
        if (firstSelIndex == -1)
        {
            LogToFile("  EditSubclassProc: 只有提示行，没有实际项目");
            return;
        }
        ListView_SetItemState(g_hListView, firstSelIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        LogToFile("  EditSubclassProc: Force selecting first actual item (跳过提示行)");
        WCHAR firstItemText[1024] = {0};
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = (int)firstSelIndex;
        lvItem.iSubItem = 0;
        lvItem.pszText = firstItemText;
        lvItem.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
        int getItemResult = ListView_GetItem(g_hListView, &lvItem);
        char firstItemLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
        sprintf(logMsg, "  EditSubclassProc: First actual item text: '%s' (GetItem返回值: %d, 文本长度: %zu)", firstItemLog, getItemResult, wcslen(firstItemText));
        LogToFile(logMsg);
        if (wcslen(firstItemText) == 0 && !g_searchResults.empty())
        {
            char fallbackLog[300] = {0};
            char fallbackName[256] = {0};
            WideCharToMultiByte(CP_UTF8, 0, g_searchResults[0].name, -1, fallbackName, sizeof(fallbackName), NULL, NULL);
            sprintf(fallbackLog, "  EditSubclassProc: ListView获取失败，使用g_searchResults[0]: '%s'", fallbackName);
            LogToFile(fallbackLog);
        }
        if (!g_windowInitializing && !g_searchResults.empty() && g_searchResults.size() > 0)
        {
            if (g_bookmarkMode)
            {
                LogToFile("  EditSubclassProc: WZ模式下用户按回车键，打开第一个网址收藏");
                const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                if (firstSelIndex >= 0 && firstSelIndex < (INT_PTR)displayBookmarks.size())
                {
                    ShellExecuteW(NULL, L"open", displayBookmarks[firstSelIndex].second.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    char logMsg2[500] = {0};
                    char nameLog[256] = {0};
                    char urlLog[256] = {0};
                    WideCharToMultiByte(CP_UTF8, 0, displayBookmarks[firstSelIndex].first.c_str(), -1, nameLog, sizeof(nameLog), NULL, NULL);
                    WideCharToMultiByte(CP_UTF8, 0, displayBookmarks[firstSelIndex].second.c_str(), -1, urlLog, sizeof(urlLog), NULL, NULL);
                    sprintf(logMsg2, "  EditSubclassProc: 已打开网址收藏[%Id] '%s' -> '%s'", firstSelIndex, nameLog, urlLog);
                    LogToFile(logMsg2);
                }
                else
                {
                    LogToFile("  EditSubclassProc: WZ模式下索引无效，无法打开网址");
                }
            }
            else
            {
                LogToFile("  EditSubclassProc: 用户按回车键，执行第一个搜索结果");
                ExecuteSelectedItem(firstSelIndex);
            }
        }
        else if (g_windowInitializing)
        {
            LogToFile("  EditSubclassProc: 窗口初始化中，跳过自动执行");
        }
        else
        {
            if (wcscmp(firstItemText, L"No matching items found") == 0)
            {
                LogToFile("  EditSubclassProc: First item is 'No matching items found' message, not executing");
            }
            else
            {
                LogToFile("  EditSubclassProc: 搜索结果为空，不执行");
            }
        }
        return;
    }
    if (GetWindowTextLengthW(hwnd) > 0)
    {
        char logMsg3[1100] = {0};
        sprintf(logMsg3, "  EditSubclassProc: List empty, processing input as command: '%s'", currentTextLog);
        LogToFile(logMsg3);
        if ((g_calculatorMode || g_dirMode || g_bookmarkMode || g_fileMode) && wcscmp(currentText, L"q") == 0)
        {
            LogToFile("  EditSubclassProc: 检测到特殊模式下输入'q'，退出当前模式");
            if (g_calculatorMode) ExitCalculatorMode();
            if (g_dirMode) ExitDirMode();
            if (g_bookmarkMode) ExitBookmarkMode();
            if (g_fileMode) ExitFileMode();
            return;
        }
        LogToFile("  EditSubclassProc: 调用ProcessCommand处理命令");
        ProcessCommand(currentText);
        if (g_calculatorMode && wcscmp(currentText, L"js") != 0 && wcscmp(currentText, L"wz") != 0 && wcscmp(currentText, L"q") != 0)
        {
            LogToFile("  EditSubclassProc: 计算模式下，调用EvaluateExpression");
            EvaluateExpression(currentText);
        }
        else if (g_bookmarkMode && wcscmp(currentText, L"js") != 0 && wcscmp(currentText, L"wz") != 0 && wcscmp(currentText, L"q") != 0)
        {
            LogToFile("  EditSubclassProc: WZ模式下，进行书签搜索");
            SearchBookmarks(currentText);
        }
        else if (g_fileMode && wcscmp(currentText, L"js") != 0 && wcscmp(currentText, L"wz") != 0 && wcscmp(currentText, L"q") != 0)
        {
            LogToFile("  EditSubclassProc: 文件模式下，进行文件搜索");
            SearchFiles(currentText);
        }
    }
    else
    {
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

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                HandleReturnKey(hwnd);
                return 0;
            }
            break;
        case WM_SETFOCUS:
            if (g_calculatorMode)
            {
                LogToFile("EditSubclassProc: 计算模式下文本框获得焦点，允许正常处理");
            }
            break;
        case WM_KILLFOCUS:
            if (g_calculatorMode)
            {
                LogToFile("EditSubclassProc: 计算模式下文本框失去焦点，正常处理");
            }
            break;
    }
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
    
    // 常用快捷方式
    lvi.iItem = 12;
    lvi.pszText = (WCHAR*)L"常用快捷方式：";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 13;
    lvi.pszText = (WCHAR*)L"Ctrl+Alt+Q - 快速显示/隐藏窗口";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 14;
    lvi.pszText = (WCHAR*)L"Ctrl+F1 - 将窗口定位到桌面中央";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 15;
    lvi.pszText = (WCHAR*)L"Ctrl+F2 - 快速显示窗口";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 16;
    lvi.pszText = (WCHAR*)L"ESC - 隐藏窗口";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 17;
    lvi.pszText = (WCHAR*)L"Tab - 在编辑框和列表框之间切换焦点";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 18;
    lvi.pszText = (WCHAR*)L"F3 - 编辑当前选中的快捷方式";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = 19;
    lvi.pszText = (WCHAR*)L"Delete - 删除当前选中的快捷方式";
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
    lvi.pszText = (WCHAR*)L"快捷方式管理";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"导入桌面快捷方式";
    ListView_InsertItem(g_hListView, &lvi);

    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"同步开始菜单快捷方式";
    ListView_InsertItem(g_hListView, &lvi);

    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"系统设置";
    ListView_InsertItem(g_hListView, &lvi);
    
    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"关于软件";
    ListView_InsertItem(g_hListView, &lvi);

    lvi.iItem = ListView_GetItemCount(g_hListView);
    lvi.pszText = (WCHAR*)L"退出程序";
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
    
    if (g_shortcuts.empty())
    {
        MessageBoxW(g_hMainWindow, L"没有可管理的快捷方式\n\n请先添加一些快捷方式！", L"快捷方式管理", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // 创建管理对话框
    WCHAR dialogTitle[] = L"快捷方式管理";
    int dialogWidth = 600;
    int dialogHeight = 500;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    HWND hDlg = CreateWindowExW(0, L"#32770", dialogTitle,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                               dialogX, dialogY, dialogWidth, dialogHeight,
                               g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hDlg)
    {
        MessageBoxW(g_hMainWindow, L"无法创建管理窗口", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 创建字体
    HFONT hFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
    SendMessageW(hDlg, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // 创建标题
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"快捷方式管理 - 选择要编辑的快捷方式:",
                                 WS_VISIBLE | WS_CHILD,
                                 15, 10, 560, 25,
                                 hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 为标题设置字体
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // 创建ListView用于显示快捷方式列表
    HWND hListView = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", NULL,
                                    WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHAREIMAGELISTS | LVS_EX_FULLROWSELECT,
                                    15, 40, 560, 350,
                                    hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 为ListView设置字体
    SendMessageW(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // 确保ListView使用Unicode
    ListView_SetUnicodeFormat(hListView, TRUE);
    
    // 设置ListView样式
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    
    // 添加列
    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    
    lvc.pszText = (LPWSTR)L"名称";
    lvc.cx = 150;
    lvc.iSubItem = 0;
    ListView_InsertColumn(hListView, 0, &lvc);
    
    lvc.pszText = (LPWSTR)L"路径";
    lvc.cx = 200;
    lvc.iSubItem = 1;
    ListView_InsertColumn(hListView, 1, &lvc);
    
    lvc.pszText = (LPWSTR)L"首页显示";
    lvc.cx = 80;
    lvc.iSubItem = 2;
    ListView_InsertColumn(hListView, 2, &lvc);
    
    lvc.pszText = (LPWSTR)L"类型";
    lvc.cx = 80;
    lvc.iSubItem = 3;
    ListView_InsertColumn(hListView, 3, &lvc);
    
    // 填充数据
    LVITEMW lvItem = {0};
    for (int i = 0; i < (int)g_shortcuts.size(); i++)
    {
        const ShortcutItem& shortcut = g_shortcuts[i];
        
        // 名称
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPWSTR)shortcut.name;
        ListView_InsertItem(hListView, &lvItem);
        
        // 路径
        lvItem.iSubItem = 1;
        lvItem.pszText = (LPWSTR)shortcut.path;
        ListView_SetItem(hListView, &lvItem);
        
        // 首页显示状态
        lvItem.iSubItem = 2;
        lvItem.pszText = (LPWSTR)(shortcut.showOnHome ? L"✓" : L"✗");
        ListView_SetItem(hListView, &lvItem);
        
        // 类型
        lvItem.iSubItem = 3;
        WCHAR typeText[20] = {0};
        if (shortcut.type == 0)
            wcscpy_s(typeText, L"文件夹");
        else if (shortcut.type == 1)
            wcscpy_s(typeText, L"URL");
        else
            wcscpy_s(typeText, L"程序");
        lvItem.pszText = typeText;
        ListView_SetItem(hListView, &lvItem);
    }
    
    // 创建按钮框架
    HWND hButtonGroup = CreateWindowExW(0, L"BUTTON", NULL,
                                       WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                       15, 400, 560, 80,
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 为分组框设置字体
    SendMessageW(hButtonGroup, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // 创建按钮
    HWND hEditBtn = CreateWindowExW(0, L"BUTTON", L"编辑",
                                   WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                   50, 420, 80, 30,
                                   hDlg, (HMENU)1001, GetModuleHandle(NULL), NULL);
    
    // 为按钮设置字体
    SendMessageW(hEditBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    HWND hAddBtn = CreateWindowExW(0, L"BUTTON", L"添加",
                                  WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                  140, 420, 80, 30,
                                  hDlg, (HMENU)1002, GetModuleHandle(NULL), NULL);
    
    // 为按钮设置字体
    SendMessageW(hAddBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    HWND hDeleteBtn = CreateWindowExW(0, L"BUTTON", L"删除",
                                     WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                     230, 420, 80, 30,
                                     hDlg, (HMENU)1003, GetModuleHandle(NULL), NULL);
    
    // 为按钮设置字体
    SendMessageW(hDeleteBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    HWND hCloseBtn = CreateWindowExW(0, L"BUTTON", L"关闭",
                                    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                    450, 420, 80, 30,
                                    hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
    
    // 为按钮设置字体
    SendMessageW(hCloseBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // 显示窗口
    ShowWindow(hDlg, SW_SHOW);
    SetFocus(hListView);
    
    // 设置对话框为模态窗口
    EnableWindow(g_hMainWindow, FALSE);
    
    // 消息循环
    MSG msg;
    BOOL bRunning = TRUE;
    BOOL bResult = FALSE;
    
    while (bRunning && GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_COMMAND)
        {
            if (LOWORD(msg.wParam) == 1001) // 编辑按钮
            {
                int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                if (selectedIndex >= 0)
                {
                    ShowEditShortcutDialog(selectedIndex);
                    // 刷新列表
                    ListView_DeleteAllItems(hListView);
                    for (int i = 0; i < (int)g_shortcuts.size(); i++)
                    {
                        const ShortcutItem& shortcut = g_shortcuts[i];
                        
                        lvItem.mask = LVIF_TEXT;
                        lvItem.iItem = i;
                        lvItem.iSubItem = 0;
                        lvItem.pszText = (LPWSTR)shortcut.name;
                        ListView_InsertItem(hListView, &lvItem);
                        
                        lvItem.iSubItem = 1;
                        lvItem.pszText = (LPWSTR)shortcut.path;
                        ListView_SetItem(hListView, &lvItem);
                        
                        lvItem.iSubItem = 2;
                        lvItem.pszText = (LPWSTR)(shortcut.showOnHome ? L"✓" : L"✗");
                        ListView_SetItem(hListView, &lvItem);
                        
                        lvItem.iSubItem = 3;
                        WCHAR typeText[20] = {0};
                        if (shortcut.type == 0)
                            wcscpy_s(typeText, L"文件夹");
                        else if (shortcut.type == 1)
                            wcscpy_s(typeText, L"URL");
                        else
                            wcscpy_s(typeText, L"程序");
                        lvItem.pszText = typeText;
                        ListView_SetItem(hListView, &lvItem);
                    }
                }
            }
            else if (LOWORD(msg.wParam) == 1002) // 添加按钮
            {
                ShowAddShortcutDialog();
                // 刷新列表
                ListView_DeleteAllItems(hListView);
                for (int i = 0; i < (int)g_shortcuts.size(); i++)
                {
                    const ShortcutItem& shortcut = g_shortcuts[i];
                    
                    lvItem.mask = LVIF_TEXT;
                    lvItem.iItem = i;
                    lvItem.iSubItem = 0;
                    lvItem.pszText = (LPWSTR)shortcut.name;
                    ListView_InsertItem(hListView, &lvItem);
                    
                    lvItem.iSubItem = 1;
                    lvItem.pszText = (LPWSTR)shortcut.path;
                    ListView_SetItem(hListView, &lvItem);
                    
                    lvItem.iSubItem = 2;
                    lvItem.pszText = (LPWSTR)(shortcut.showOnHome ? L"✓" : L"✗");
                    ListView_SetItem(hListView, &lvItem);
                    
                    lvItem.iSubItem = 3;
                    WCHAR typeText[20] = {0};
                    if (shortcut.type == 0)
                        wcscpy_s(typeText, L"文件夹");
                    else if (shortcut.type == 1)
                        wcscpy_s(typeText, L"URL");
                    else
                        wcscpy_s(typeText, L"程序");
                    lvItem.pszText = typeText;
                    ListView_SetItem(hListView, &lvItem);
                }
            }
            else if (LOWORD(msg.wParam) == 1003) // 删除按钮
            {
                int selectedIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                if (selectedIndex >= 0)
                {
                    WCHAR msgText[256];
                    wsprintfW(msgText, L"确定要删除快捷方式 '%s' 吗？", g_shortcuts[selectedIndex].name);
                    if (MessageBoxW(hDlg, msgText, L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES)
                    {
                        // 删除快捷方式
                        g_shortcuts.erase(g_shortcuts.begin() + selectedIndex);
                        SaveShortcuts();
                        
                        // 刷新列表
                        ListView_DeleteAllItems(hListView);
                        for (int i = 0; i < (int)g_shortcuts.size(); i++)
                        {
                            const ShortcutItem& shortcut = g_shortcuts[i];
                            
                            lvItem.mask = LVIF_TEXT;
                            lvItem.iItem = i;
                            lvItem.iSubItem = 0;
                            lvItem.pszText = (LPWSTR)shortcut.name;
                            ListView_InsertItem(hListView, &lvItem);
                            
                            lvItem.iSubItem = 1;
                            lvItem.pszText = (LPWSTR)shortcut.path;
                            ListView_SetItem(hListView, &lvItem);
                            
                            lvItem.iSubItem = 2;
                            lvItem.pszText = (LPWSTR)(shortcut.showOnHome ? L"✓" : L"✗");
                            ListView_SetItem(hListView, &lvItem);
                            
                            lvItem.iSubItem = 3;
                            WCHAR typeText[20] = {0};
                            if (shortcut.type == 0)
                                wcscpy_s(typeText, L"文件夹");
                            else if (shortcut.type == 1)
                                wcscpy_s(typeText, L"URL");
                            else
                                wcscpy_s(typeText, L"程序");
                            lvItem.pszText = typeText;
                            ListView_SetItem(hListView, &lvItem);
                        }
                    }
                }
            }
            else if (LOWORD(msg.wParam) == IDCANCEL) // 关闭按钮
            {
                bRunning = FALSE;
            }
        }
        else if (msg.message == WM_CLOSE)
        {
            bRunning = FALSE;
        }
        
        if (!IsDialogMessage(hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    // 恢复主窗口
    EnableWindow(g_hMainWindow, TRUE);
    SetFocus(g_hMainWindow);
    
    // 清理资源
    DeleteObject(hFont);  // 删除创建的字体对象
    DestroyWindow(hDlg);
}

struct SystemSettingsDialogState
{
    HFONT font;
    HWND chkMinimizeToTray;
    HWND chkShowStartPage;
    bool* saved;
};

static LRESULT CALLBACK SystemSettingsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return TRUE;
    }

    auto* state = reinterpret_cast<SystemSettingsDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE:
    {
        if (!state) return -1;

        state->font = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

        HWND hTitle = CreateWindowExW(0, L"STATIC", L"界面与启动设置",
                                      WS_VISIBLE | WS_CHILD,
                                      20, 20, 300, 28,
                                      hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)state->font, TRUE);

        HWND hDesc = CreateWindowExW(0, L"STATIC",
                                     L"参考 Win11 开始菜单体验，控制窗口最小化行为和默认打开页面。",
                                     WS_VISIBLE | WS_CHILD,
                                     20, 55, 460, 24,
                                     hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hDesc, WM_SETFONT, (WPARAM)state->font, TRUE);

        state->chkMinimizeToTray = CreateWindowExW(0, L"BUTTON",
                                                   L"最小化时隐藏到托盘",
                                                   WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                                   20, 95, 260, 28,
                                                   hwnd, (HMENU)2001, GetModuleHandle(NULL), NULL);
        SendMessageW(state->chkMinimizeToTray, WM_SETFONT, (WPARAM)state->font, TRUE);
        SendMessageW(state->chkMinimizeToTray, BM_SETCHECK, g_minimizeToTray ? BST_CHECKED : BST_UNCHECKED, 0);

        state->chkShowStartPage = CreateWindowExW(0, L"BUTTON",
                                                  L"显示窗口时优先打开开始页",
                                                  WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                                  20, 130, 280, 28,
                                                  hwnd, (HMENU)2002, GetModuleHandle(NULL), NULL);
        SendMessageW(state->chkShowStartPage, WM_SETFONT, (WPARAM)state->font, TRUE);
        SendMessageW(state->chkShowStartPage, BM_SETCHECK, g_showStartPageOnLaunch ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hTip = CreateWindowExW(0, L"STATIC",
                                    L"开始页会展示固定快捷方式、推荐项目和开始菜单程序。",
                                    WS_VISIBLE | WS_CHILD,
                                    40, 165, 420, 24,
                                    hwnd, NULL, GetModuleHandle(NULL), NULL);
        SendMessageW(hTip, WM_SETFONT, (WPARAM)state->font, TRUE);

        HWND hOkBtn = CreateWindowExW(0, L"BUTTON", L"保存",
                                      WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                      300, 205, 90, 32,
                                      hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
        SendMessageW(hOkBtn, WM_SETFONT, (WPARAM)state->font, TRUE);

        HWND hCancelBtn = CreateWindowExW(0, L"BUTTON", L"取消",
                                          WS_VISIBLE | WS_CHILD,
                                          400, 205, 90, 32,
                                          hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
        SendMessageW(hCancelBtn, WM_SETFONT, (WPARAM)state->font, TRUE);

        SetFocus(state->chkMinimizeToTray);
        return 0;
    }
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (code != BN_CLICKED && id != IDOK && id != IDCANCEL)
        {
            break;
        }

        if (id == IDOK)
        {
            g_minimizeToTray = (SendMessageW(state->chkMinimizeToTray, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_showStartPageOnLaunch = (SendMessageW(state->chkShowStartPage, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveAppSettings();
            if (state->saved) *state->saved = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (id == IDCANCEL)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_NCDESTROY:
        if (state)
        {
            if (state->font) DeleteObject(state->font);
            delete state;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowSystemSettingsDialog() {
    LogToFile("ShowSystemSettingsDialog: 显示系统设置对话框");

    static bool s_registered = false;
    if (!s_registered)
    {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = SystemSettingsDialogProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"BVSystemSettingsDialog";
        RegisterClassExW(&wc);
        s_registered = true;
    }

    bool saved = false;
    auto* state = new SystemSettingsDialogState();
    state->font = NULL;
    state->chkMinimizeToTray = NULL;
    state->chkShowStartPage = NULL;
    state->saved = &saved;

    const int dialogWidth = 520;
    const int dialogHeight = 280;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"BVSystemSettingsDialog",
        L"系统设置",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dialogX, dialogY, dialogWidth, dialogHeight,
        g_hMainWindow, NULL, GetModuleHandle(NULL), state);

    if (!hDlg)
    {
        delete state;
        MessageBoxW(g_hMainWindow, L"无法创建系统设置窗口", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    EnableWindow(g_hMainWindow, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(g_hMainWindow, TRUE);
    SetFocus(g_hMainWindow);

    if (saved)
    {
        if (!g_calculatorMode && !g_dirMode && !g_bookmarkMode && !g_fileMode)
        {
            if (g_showStartPageOnLaunch)
            {
                UpdateInitialWebViewContent();
            }
            else
            {
                ShowBasicUsage();
            }
        }
        MessageBoxW(g_hMainWindow, L"系统设置已保存。", L"系统设置", MB_OK | MB_ICONINFORMATION);
    }
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
    else if (wcscmp(itemText, L"导入桌面快捷方式") == 0) {
        // 导入桌面快捷方式
        LogToFile("HandleSettingsMenuItemClick: 用户选择导入桌面快捷方式");
        int count = ImportDesktopShortcuts();
        WCHAR msg[256];
        wsprintfW(msg, L"成功导入 %d 个桌面快捷方式！", count);
        MessageBoxW(g_hMainWindow, msg, L"导入完成", MB_OK | MB_ICONINFORMATION);
    }
    else if (wcscmp(itemText, L"同步开始菜单快捷方式") == 0) {
        LogToFile("HandleSettingsMenuItemClick: 用户选择同步开始菜单快捷方式");
        int count = ImportStartMenuShortcuts();
        WCHAR msg[256];
        wsprintfW(msg, L"成功同步 %d 个开始菜单快捷方式！", count);
        MessageBoxW(g_hMainWindow, msg, L"同步完成", MB_OK | MB_ICONINFORMATION);
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

/**
 * @brief 保存应用设置到注册表
 */
void SaveAppSettings()
{
    HKEY hKey;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, NULL, 0,
                        KEY_SET_VALUE, NULL, &hKey, &disposition) == ERROR_SUCCESS)
    {
        DWORD minimizeToTray = g_minimizeToTray ? 1 : 0;
        DWORD showStartPage = g_showStartPageOnLaunch ? 1 : 0;
        RegSetValueExW(hKey, L"MinimizeToTray", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&minimizeToTray), sizeof(DWORD));
        RegSetValueExW(hKey, L"ShowStartPageOnLaunch", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&showStartPage), sizeof(DWORD));
        RegCloseKey(hKey);
        LogToFile("SaveAppSettings: 应用设置已保存");
    }
}

/**
 * @brief 从注册表加载应用设置
 */
void LoadAppSettings()
{
    g_minimizeToTray = true;
    g_showStartPageOnLaunch = true;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\BVQuickLauncher", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        DWORD value = 0;
        DWORD valueSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"MinimizeToTray", 0, NULL, reinterpret_cast<LPBYTE>(&value), &valueSize) == ERROR_SUCCESS)
        {
            g_minimizeToTray = (value != 0);
        }

        value = 0;
        valueSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"ShowStartPageOnLaunch", 0, NULL, reinterpret_cast<LPBYTE>(&value), &valueSize) == ERROR_SUCCESS)
        {
            g_showStartPageOnLaunch = (value != 0);
        }

        RegCloseKey(hKey);
    }

    LogToFile(g_minimizeToTray ? "LoadAppSettings: 最小化到托盘已启用" : "LoadAppSettings: 最小化到托盘已关闭");
    LogToFile(g_showStartPageOnLaunch ? "LoadAppSettings: 启动开始页已启用" : "LoadAppSettings: 启动开始页已关闭");
}

// 从注册表加载窗口大小和位置
void LoadWindowSettings(int& x, int& y, int& width, int& height) {
    // 默认窗口大小和位置
    width = 800;
    height = 800;
    
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
            sprintf(errorMsg, "UpdateWebView2Content: 更新失败，错误代码: 0x%08lX", hr);
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
    // 使用缓存，避免重复读取文件
    if (!g_webViewHtmlCached)
    {
        // 从外部模板文件读取HTML内容
        std::wstring templatePath = L"data/webview_template.html";
        g_cachedWebViewHtml = ReadHtmlTemplate(templatePath);
        
        // 如果读取失败（返回空字符串），记录日志并使用错误提示
        if (g_cachedWebViewHtml.empty())
        {
            LogToFile("CreateWebView2HTML: 模板文件读取失败 (data/webview_template.html)");
            html = L"<html><body><h3 style='color:red;'>错误：无法加载模板文件 (data/webview_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
            return;
        }
        
        g_webViewHtmlCached = true;
        LogToFile("CreateWebView2HTML: 模板文件已缓存");
    }
    
    // 使用缓存的模板
    html = g_cachedWebViewHtml;

    // 替换模板中的占位符
    std::wstring hintsHtml;
    if (!hints.empty())
    {
        hintsHtml = L"<div class='hint-banner'>";
        hintsHtml += L"<div class='banner-header'>";
        hintsHtml += L"<div class='banner-title'>💡 操作提示</div>";
        hintsHtml += L"<button class='home-button' onclick='onHomeClick()'>🏠 首页</button>";
        hintsHtml += L"<button class='add-button' onclick='onAddClick()'>➕ 添加快捷方式</button>";
        hintsHtml += L"</div>";
        hintsHtml += L"<ul>";
        for (const auto& hint : hints)
        {
            hintsHtml += L"<li>";
            hintsHtml += hint;
            hintsHtml += L"</li>";
        }
        hintsHtml += L"</ul></div>";
    }
    else
    {
        // 即使没有提示，也显示添加按钮
            hintsHtml = L"<div class='hint-banner'>";
        hintsHtml += L"<div class='banner-header'>";
        hintsHtml += L"<div class='banner-title'>💡 操作提示</div>";
        hintsHtml += L"<button class='home-button' onclick='onHomeClick()'>🏠 首页</button>";
        hintsHtml += L"<button class='add-button' onclick='onAddClick()'>➕ 添加快捷方式</button>";
        hintsHtml += L"</div>";
        hintsHtml += L"</div>";
    }
    
    std::wstring itemsHtml;
    if (items.empty())
    {
        itemsHtml = L"<tr class='empty-row'><td colspan='3'>未找到匹配项，试试其他关键字，或输入 <strong>help</strong> 查看可用命令。</td></tr>";
    }
    else
    {
        for (size_t i = 0; i < items.size(); i++)
        {
            itemsHtml += L"<tr class='item-row' data-index='";
            itemsHtml += std::to_wstring(i);
            itemsHtml += L"' ondblclick='onRowDblClick(";
            itemsHtml += std::to_wstring(i);
            itemsHtml += L")'>";
            
            // 名称列：显示图标和名称
            itemsHtml += L"<td><div class='item-name'>";
            
            // 提取并显示图标（优先显示真实程序图标）
            if (wcsncmp(items[i].iconPath, L"emoji:", 6) == 0)
            {
                itemsHtml += L"<span class='item-icon' style='font-size:20px; display:inline-block; width:24px; height:24px; text-align:center; vertical-align:middle; line-height:24px; margin-right:8px;'>";
                itemsHtml += (items[i].iconPath + 6);
                itemsHtml += L"</span>";
            }
            else
            {
                std::wstring iconDataUri = GetShortcutIconDataUri(items[i], 24);
                if (!iconDataUri.empty())
                {
                    itemsHtml += L"<img src='";
                    itemsHtml += iconDataUri;
                    itemsHtml += L"' class='item-icon' alt='图标'>";
                }
                else if (items[i].type == 1 && wcsstr(items[i].iconPath, L"http") != NULL)
                {
                    itemsHtml += L"<img src='";
                    itemsHtml += items[i].iconPath;
                    itemsHtml += L"' class='item-icon' alt='图标'>";
                }
                else
                {
                    itemsHtml += L"<img src=\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16' viewBox='0 0 24 24'><rect x='3' y='3' width='18' height='18' rx='4' ry='4' fill='%23e9f0ff' stroke='%23008cff' stroke-width='2'/><path d='M9 9h6v6' fill='none' stroke='%23008cff' stroke-width='2'/><path d='M9 15l6-6' fill='none' stroke='%23008cff' stroke-width='2'/></svg>\" class='item-icon' alt='图标'>";
                }
            }
            
            itemsHtml += items[i].name;
            itemsHtml += L"</div></td>";
            
            // 备注列：显示备注信息
            itemsHtml += L"<td>";
            if (wcslen(items[i].comment) > 0)
            {
                itemsHtml += items[i].comment;
            }
            else
            {
                itemsHtml += L"-";
            }
            itemsHtml += L"</td>";
            
            // 操作列：显示编辑按钮
            itemsHtml += L"<td><button class='edit-button' onclick='onEditClick(";
            itemsHtml += std::to_wstring(i);
            itemsHtml += L", event)'>编辑</button></td></tr>";
        }
    }
    
    // 替换占位符
    ReplaceStringInPlace(html, L"<!-- HINTS_PLACEHOLDER -->", hintsHtml);
    ReplaceStringInPlace(html, L"<!-- ITEMS_PLACEHOLDER -->", itemsHtml);
}

void UpdateCalculatorModeWebView()
{
    g_currentViewMode = ViewMode::CALCULATOR;
    if (!g_webView)
    {
        LogToFile("UpdateCalculatorModeWebView: WebView2 未初始化，无法显示计算模式内容");
        return;
    }
    
    // 使用缓存
    if (!g_calculatorHtmlCached)
    {
        std::wstring templatePath = L"data/calculator_template.html";
        g_cachedCalculatorHtml = ReadHtmlTemplate(templatePath);
        
        if (g_cachedCalculatorHtml.empty())
        {
            LogToFile("UpdateCalculatorModeWebView: 模板文件读取失败 (data/calculator_template.html)");
            std::wstring errorHtml = L"<html><body><h3 style='color:red;'>错误：无法加载计算器模板文件 (data/calculator_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
            UpdateWebView2Content(errorHtml.c_str());
            return;
        }
        
        g_calculatorHtmlCached = true;
        LogToFile("UpdateCalculatorModeWebView: 模板文件已缓存");
    }
    
    std::wstring html = g_cachedCalculatorHtml;
    
    // 生成 g_formulas JS 对象
    auto EscapeJson = [](const std::wstring& str) -> std::wstring {
        std::wstring result;
        for (wchar_t c : str) {
            if (c == L'\\') result += L"\\\\";
            else if (c == L'"') result += L"\\\"";
            else if (c == L'\n') result += L"\\n";
            else if (c == L'\r') result += L"\\r";
            else if (c == L'\t') result += L"\\t";
            else result += c;
        }
        return result;
    };

    std::wstring formulasJs = L"const g_formulas = {\n";
    for (const auto& formula : g_customFormulas) {
        formulasJs += L"  \"" + EscapeJson(formula.name) + L"\": { ";
        formulasJs += L"expr: \"" + EscapeJson(formula.expression) + L"\", ";
        formulasJs += L"desc: \"" + EscapeJson(formula.description) + L"\" },\n";
    }
    formulasJs += L"};\n";
    ReplaceStringInPlace(html, L"<!-- FORMULA_DATA_PLACEHOLDER -->", formulasJs);

    // 生成自定义公式HTML
    std::wstring customFormulasHtml;
    if (!g_customFormulas.empty())
    {
        for (const auto& formula : g_customFormulas)
        {
            // 在JS中，我们将解析这个表达式
            customFormulasHtml += L"<button class='formula-btn' onclick='useCustomFormula(\"";
            customFormulasHtml += EscapeJson(formula.expression);
            customFormulasHtml += L"\")' oncontextmenu='showFormulaContextMenu(event, \"";
            customFormulasHtml += EscapeJson(formula.name);
            customFormulasHtml += L"\")'>";
            customFormulasHtml += formula.name;
            customFormulasHtml += L"</button>";
        }
    }
    
    // 替换模板中的占位符
    ReplaceStringInPlace(html, L"<!-- CUSTOM_FORMULAS_PLACEHOLDER -->", customFormulasHtml);

    // 替换模板中的占位符
    std::wstring historyHtml;
    if (g_calculationHistory.empty())
    {
        historyHtml = L"<div class='empty'>暂无计算记录，试着输入表达式开始计算吧。</div>";
    }
    else
    {
        historyHtml = L"<table><thead><tr><th>表达式</th><th>结果</th><th>备注</th><th style='width: 40px;'>操作</th></tr></thead><tbody>";
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
            historyHtml += L"</td><td style='text-align:center;'><span class='delete-btn' onclick='deleteHistory(";
            historyHtml += std::to_wstring(displayIndex);
            historyHtml += L", event)'>×</span></td></tr>";
        }
        historyHtml += L"</tbody></table>";
    }
    ReplaceStringInPlace(html, L"<!-- HISTORY_PLACEHOLDER -->", historyHtml);
    
    UpdateWebView2Content(html.c_str());
    InjectCustomFormulasToWebView();
}

/**
 * @brief 更新设置菜单WebView显示
 * 
 * 此函数更新WebView2中设置菜单的显示内容
 */
void UpdateSettingsMenuWebView()
{
    g_currentViewMode = ViewMode::SETTINGS;
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
        
        // 如果读取失败，显示错误信息
        if (g_cachedSettingsHtml.empty())
        {
            LogToFile("UpdateSettingsMenuWebView: 模板文件读取失败 (data/settings_template.html)");
            std::wstring errorHtml = L"<html><body><h3 style='color:red;'>错误：无法加载设置菜单模板文件 (data/settings_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
            UpdateWebView2Content(errorHtml.c_str());
            return;
        }
        
        // 模板文件读取成功，需要替换占位符
        LogToFile("UpdateSettingsMenuWebView: 模板文件读取成功，开始替换占位符");
        
        // 生成菜单项HTML内容
        std::wstring menuItemsHtml;
        
        // 菜单项列表
        const WCHAR* menuItems[] = {
            L"快捷方式管理",
            L"导入桌面快捷方式",
            L"同步开始菜单快捷方式",
            L"系统设置",
            L"关于软件",
            L"退出程序"
        };
        
        const WCHAR* menuIcons[] = {
            L"📁",
            L"⬇️",
            L"🪟",
            L"⚙️",
            L"ℹ️",
            L"🚪"
        };
        
        for (int i = 0; i < 6; i++)
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
        
        g_settingsHtmlCached = true;
        LogToFile("UpdateSettingsMenuWebView: 设置菜单HTML已缓存");
    }
    
    UpdateWebView2Content(g_cachedSettingsHtml.c_str());
}

void UpdateHelpInfoWebView()
{
    g_currentViewMode = ViewMode::HELP;
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
        
        if (g_cachedHelpHtml.empty())
        {
            LogToFile("UpdateHelpInfoWebView: 模板文件读取失败 (data/help_template.html)");
            std::wstring errorHtml = L"<html><body><h3 style='color:red;'>错误：无法加载帮助信息模板文件 (data/help_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
            UpdateWebView2Content(errorHtml.c_str());
            return;
        }
        
        g_helpHtmlCached = true;
        LogToFile("UpdateHelpInfoWebView: 帮助信息HTML已缓存");
    }
    
    UpdateWebView2Content(g_cachedHelpHtml.c_str());
}

/**
 * @brief 更新文件模式的WebView2显示
 */
void UpdateFileModeWebView()
{
    g_currentViewMode = ViewMode::FILE_SEARCH;
    if (!g_webView)
    {
        LogToFile("UpdateFileModeWebView: WebView2 未初始化，无法显示文件模式内容");
        return;
    }
    
    // 使用缓存
    if (!g_fileModeHtmlCached)
    {
        g_cachedFileModeHtml = ReadHtmlTemplate(L"data/file_mode_template.html");
        
        if (g_cachedFileModeHtml.empty())
        {
            LogToFile("UpdateFileModeWebView: 模板文件读取失败 (data/file_mode_template.html)");
            std::wstring errorHtml = L"<html><body><h3 style='color:red;'>错误：无法加载文件模式模板文件 (data/file_mode_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
            UpdateWebView2Content(errorHtml.c_str());
            return;
        }
        
        g_fileModeHtmlCached = true;
        LogToFile("UpdateFileModeWebView: 模板文件已缓存");
    }
    
    std::wstring htmlContent = g_cachedFileModeHtml;
    
    // 生成搜索结果HTML
    std::wstring resultsHtml;
    
    if (g_fileSearchResults.empty())
    {
        resultsHtml = L"<div class='no-results'>"
                      L"🔍 请输入文件名或路径关键字开始搜索"
                      L"</div>";
    }
    else
    {
        // 显示搜索结果
        for (const auto& file : g_fileSearchResults)
        {
            // 根据文件类型选择图标
            std::wstring icon = L"📄"; // 默认文件图标
            if (file.fileType.find(L"文件夹") != std::wstring::npos || 
                file.fileType.find(L"Directory") != std::wstring::npos)
            {
                icon = L"📁";
            }
            else if (file.fileType.find(L"图像") != std::wstring::npos || 
                     file.fileType.find(L"Image") != std::wstring::npos)
            {
                icon = L"🖼️";
            }
            else if (file.fileType.find(L"视频") != std::wstring::npos || 
                     file.fileType.find(L"Video") != std::wstring::npos)
            {
                icon = L"🎬";
            }
            else if (file.fileType.find(L"音频") != std::wstring::npos || 
                     file.fileType.find(L"Audio") != std::wstring::npos)
            {
                icon = L"🎵";
            }
            else if (file.fileType.find(L"文档") != std::wstring::npos || 
                     file.fileType.find(L"Document") != std::wstring::npos)
            {
                icon = L"📝";
            }
            
            resultsHtml += L"<div class='file-item' onclick='openFile(\"" + file.fullPath + L"\")'>"
                          L"<span class='file-icon'>" + icon + L"</span>"
                          L"<span class='file-name'>" + file.fileName + L"</span>"
                          L"<span class='file-path'>" + file.fullPath + L"</span>"
                          L"<span class='file-size'>" + file.size + L"</span>"
                          L"<span class='file-type'>" + file.fileType + L"</span>"
                          L"</div>";
        }
    }
    
    ReplaceStringInPlace(htmlContent, L"<!-- RESULTS_PLACEHOLDER -->", resultsHtml);
    
    UpdateWebView2Content(htmlContent.c_str());
    LogToFile("UpdateFileModeWebView: 文件模式WebView显示已更新，显示搜索结果");
}



// Refresh current view based on state
void RefreshCurrentView()
{
    char logMsg[100];
    sprintf(logMsg, "RefreshCurrentView: Restoring view mode %d", (int)g_currentViewMode);
    LogToFile(logMsg);

    switch (g_currentViewMode)
    {
        case ViewMode::SHORTCUT_LIST:
            ShowBasicUsage();
            break;
        case ViewMode::SEARCH:
            SearchAndDisplayResults(g_lastSearchQuery.c_str());
            break;
        case ViewMode::DIR_MODE:
            UpdateDirModeWebView();
            break;
        case ViewMode::FILE_SEARCH:
            UpdateFileModeWebView();
            break;
        case ViewMode::BOOKMARK:
            UpdateBookmarkModeWebView();
            break;
        case ViewMode::CALCULATOR:
            UpdateCalculatorModeWebView();
            break;
        case ViewMode::SETTINGS:
            ShowSettingsMenu();
            break;
        case ViewMode::HELP:
            UpdateHelpInfoWebView(); // Note: UpdateHelpInfoWebView sets mode to HELP, which is fine
            break;
        default:
            ShowBasicUsage();
            break;
    }
}

// 显示基本用法界面
void ShowBasicUsage()
{
    g_currentViewMode = ViewMode::SHORTCUT_LIST;
    LogToFile("ShowBasicUsage: 创建基本用法HTML内容");
    
    // 更新显示内容
    if (g_webView)
    {
        // 使用缓存
        if (!g_basicUsageHtmlCached)
        {
            g_cachedBasicUsageHtml = ReadHtmlTemplate(L"data/basic_usage_template.html");
            
            if (g_cachedBasicUsageHtml.empty())
            {
                LogToFile("ShowBasicUsage: 模板文件读取失败 (data/basic_usage_template.html)");
                std::wstring errorHtml = L"<html><body><h3 style='color:red;'>错误：无法加载基本用法模板文件 (data/basic_usage_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
                UpdateWebView2Content(errorHtml.c_str());
                return;
            }
            
            g_basicUsageHtmlCached = true;
            LogToFile("ShowBasicUsage: 模板文件已缓存");
        }
        
        UpdateWebView2Content(g_cachedBasicUsageHtml.c_str());
    }
    else
    {
        if (g_webViewInitInProgress && !g_webViewInitFailed)
        {
            LogToFile("ShowBasicUsage: WebView2 正在初始化，暂不弹窗提示");
            return;
        }
        
        LogToFile("ShowBasicUsage: WebView2 初始化失败或不可用");
        
        if (!g_webViewInitErrorNotified)
        {
            g_webViewInitErrorNotified = true;
            
            WCHAR msg[512] = {0};
            wsprintfW(msg,
                      L"WebView2 运行时未安装或初始化失败！\n\n"
                      L"请下载安装 Microsoft Edge WebView2 运行时后再使用。\n"
                      L"下载地址: https://developer.microsoft.com/microsoft-edge/webview2/\n\n"
                      L"错误码: 0x%08lX\n\n"
                      L"基本功能仍然可用，详情请查看日志文件。",
                      (unsigned long)g_webViewInitHr);
            MessageBoxW(g_hMainWindow ? g_hMainWindow : NULL, msg, L"Funny Quick", MB_OK | MB_ICONINFORMATION);
        }
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

/**
 * @brief 根据当前模式更新窗口标题
 * 根据当前激活的模式（计算模式、目录浏览模式、网址收藏模式、文件模式、普通模式）设置不同的窗口标题
 */
void UpdateWindowTitle()
{
    if (!g_hMainWindow)
    {
        LogToFile("UpdateWindowTitle: 主窗口句柄为空，无法更新标题");
        return;
    }
    
    std::wstring title;
    
    if (g_calculatorMode)
    {
        title = L"快速启动--计算模式";
        LogToFile("UpdateWindowTitle: 设置为计算模式标题");
    }
    else if (g_dirMode)
    {
        title = L"快速启动--目录模式";
        LogToFile("UpdateWindowTitle: 设置为目录模式标题");
    }
    else if (g_bookmarkMode)
    {
        title = L"快速启动--网址模式";
        LogToFile("UpdateWindowTitle: 设置为网址模式标题");
    }
    else if (g_fileMode)
    {
        title = L"快速启动--文件模式";
        LogToFile("UpdateWindowTitle: 设置为文件模式标题");
    }
    else
    {
        title = L"快速启动";
        LogToFile("UpdateWindowTitle: 设置为普通模式标题");
    }
    
    SetWindowTextW(g_hMainWindow, title.c_str());
    LogToFile("UpdateWindowTitle: 窗口标题已更新");
}

/**
 * @brief 处理WM_CONTEXTMENU消息，显示快捷方式右键菜单
 * @param hwnd 窗口句柄
 * @param wParam 消息参数
 * @return 消息处理结果
 */
LRESULT HandleWMContextMenu(HWND hwnd, WPARAM wParam)
{
    LogToFile("HandleWMContextMenu: 收到右键菜单消息");
    
    // 检查是否在普通模式下（快捷方式模式）
    if (g_calculatorMode || g_dirMode || g_bookmarkMode || g_fileMode)
    {
        LogToFile("HandleWMContextMenu: 当前处于特殊模式，不显示快捷方式右键菜单");
        return DefWindowProcW(hwnd, WM_CONTEXTMENU, wParam, 0);
    }
    
    // 获取鼠标位置
    POINT pt;
    GetCursorPos(&pt);
    
    // 检查鼠标是否在ListView上
    HWND hwndUnderCursor = WindowFromPoint(pt);
    if (hwndUnderCursor != g_hListView)
    {
        LogToFile("HandleWMContextMenu: 鼠标不在ListView上，返回默认处理");
        return DefWindowProcW(hwnd, WM_CONTEXTMENU, wParam, 0);
    }
    
    // 获取ListView中选中的项目索引
    INT_PTR selectedIndex = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
    if (selectedIndex == -1)
    {
        // 如果没有选中项，尝试获取焦点项
        selectedIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
    }
    
    // 检查是否是提示行（提示行不能编辑）
    INT_PTR hintCount = GetHintRowCount();
    if (selectedIndex < hintCount)
    {
        LogToFile("HandleWMContextMenu: 选中的是提示行，不显示编辑菜单");
        return DefWindowProcW(hwnd, WM_CONTEXTMENU, wParam, 0);
    }
    
    // 调整索引（跳过提示行）
    INT_PTR adjustedIndex = selectedIndex - hintCount;
    
    // 检查索引是否有效
    if (adjustedIndex < 0 || adjustedIndex >= (INT_PTR)g_searchResults.size())
    {
        LogToFile("HandleWMContextMenu: 无效的快捷方式索引");
        return DefWindowProcW(hwnd, WM_CONTEXTMENU, wParam, 0);
    }
    
    // 创建右键菜单
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
    {
        LogToFile("HandleWMContextMenu: 创建右键菜单失败");
        return DefWindowProcW(hwnd, WM_CONTEXTMENU, wParam, 0);
    }
    
    // 添加菜单项
    AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_EDIT_SHORTCUT, L"编辑快捷方式");
    AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_ADD_SHORTCUT, L"添加快捷方式");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_DELETE_SHORTCUT, L"删除快捷方式");
    
    // 显示右键菜单
    SetForegroundWindow(hwnd);
    UINT command = TrackPopupMenu(hMenu, 
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
        pt.x, pt.y, 0, hwnd, NULL);
    
    // 销毁菜单
    DestroyMenu(hMenu);
    
    // 处理菜单命令
    if (command == ID_CONTEXT_EDIT_SHORTCUT)
    {
        LogToFile("HandleWMContextMenu: 用户选择编辑快捷方式");
        
        // 查找原始快捷方式索引（从搜索结果映射到原始列表）
        const ShortcutItem& selectedShortcut = g_searchResults[adjustedIndex];
        int originalIndex = -1;
        
        for (size_t i = 0; i < g_shortcuts.size(); i++)
        {
            if (wcscmp(g_shortcuts[i].name, selectedShortcut.name) == 0 &&
                wcscmp(g_shortcuts[i].path, selectedShortcut.path) == 0)
            {
                originalIndex = (int)i;
                break;
            }
        }
        
        if (originalIndex != -1)
        {
            // 调用编辑对话框
            ShowEditShortcutDialog(originalIndex);
        }
        else
        {
            MessageBoxW(hwnd, L"无法找到对应的快捷方式", L"错误", MB_OK | MB_ICONERROR);
            LogToFile("HandleWMContextMenu: 无法找到对应的快捷方式");
        }
    }
    else if (command == ID_CONTEXT_ADD_SHORTCUT)
    {
        LogToFile("HandleWMContextMenu: 用户选择添加快捷方式");
        ShowAddShortcutDialog();
    }
    else if (command == ID_CONTEXT_DELETE_SHORTCUT)
    {
        LogToFile("HandleWMContextMenu: 用户选择删除快捷方式");
        
        // 查找原始快捷方式索引
        const ShortcutItem& selectedShortcut = g_searchResults[adjustedIndex];
        int originalIndex = -1;
        
        for (size_t i = 0; i < g_shortcuts.size(); i++)
        {
            if (wcscmp(g_shortcuts[i].name, selectedShortcut.name) == 0 &&
                wcscmp(g_shortcuts[i].path, selectedShortcut.path) == 0)
            {
                originalIndex = (int)i;
                break;
            }
        }
        
        if (originalIndex != -1)
        {
            // 确认删除
            WCHAR message[512] = {0};
            wsprintfW(message, L"确定要删除快捷方式 '%s' 吗？", g_shortcuts[originalIndex].name);
            
            if (MessageBoxW(hwnd, message, L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                // 从快捷方式列表中删除
                g_shortcuts.erase(g_shortcuts.begin() + originalIndex);
                
                // 保存更改
                SaveShortcuts();
                
                // 刷新显示
                SearchAndDisplayResults(g_currentSearch);
                
                LogToFile("HandleWMContextMenu: 快捷方式删除成功");
            }
        }
        else
        {
            MessageBoxW(hwnd, L"无法找到对应的快捷方式", L"错误", MB_OK | MB_ICONERROR);
            LogToFile("HandleWMContextMenu: 无法找到对应的快捷方式");
        }
    }
    
    return 0;
}



