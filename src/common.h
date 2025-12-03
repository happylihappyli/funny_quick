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
extern HWND g_hExitBookmarkButton;
extern HWND g_hExitFileButton;
extern HWND g_hSettingsButton;
extern HWND g_hCalcMenuButton;
extern HWND g_hInputHintLabel;
extern HFONT g_hFont;

extern bool g_ignoreNextReturn;
extern bool g_windowInitializing;
extern bool g_calculatorMode;
extern bool g_dirMode;
extern bool g_updatingEditBox;
extern bool g_settingsMenuMode;

extern std::vector<ShortcutItem> g_shortcuts;
extern std::vector<ShortcutItem> g_searchResults;
extern WCHAR g_currentSearch[1024];
extern std::vector<CalculationRecord> g_calculationHistory;
extern std::wstring g_cachedHelpHtml;  // 缓存的帮助信息HTML
extern std::wstring g_cachedSettingsHtml;  // 缓存的设置菜单HTML
extern bool g_helpHtmlCached;  // 帮助信息是否已缓存
extern bool g_settingsHtmlCached;  // 设置菜单是否已缓存

// 书签管理相关全局变量声明
extern std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks;  // 网址收藏列表
extern std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults;  // 网址收藏搜索结果
extern bool g_bookmarkMode;  // 书签模式标志

// 文件搜索管理相关全局变量声明
extern bool g_fileMode;  // 文件模式标志
extern UINT_PTR g_fileSearchTimerId;  // 文件搜索定时器ID
extern WCHAR g_pendingFileSearchQuery[1024];  // 待处理的文件搜索查询

// 常量定义
#define IDC_EDIT 1001
#define IDC_LISTVIEW 1002
#define IDC_EXIT_CALC_BUTTON 1003
#define IDC_SETTINGS_BUTTON 1004
#define IDC_EXIT_FILE_BUTTON 1017   // 退出文件模式按钮ID
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
#define ID_CONTEXT_COPY 1011
#define ID_SETTINGS_EXIT 1012

// 新增的标识符定义
#define ID_CALC_MENU_CLEAR 1019
#define ID_CALC_MENU_COPY_RESULT 1020
#define ID_CALC_MENU_EXIT 1021
#define IDC_CALC_MENU_BUTTON 1016  // 计算模式操作菜单按钮ID

#ifndef EN_RETURN
#define EN_RETURN 0x0100
#endif

// 函数声明
void InitializeWebView2(HWND hwnd);  // 初始化 WebView2
void UpdateWebView2Content(const WCHAR* htmlContent);  // 更新 WebView2 内容
void CreateWebView2HTML(const std::vector<ShortcutItem>& items, const std::vector<std::wstring>& hints, std::wstring& html);  // 创建 HTML 内容
void UpdateCalculatorModeWebView();  // 更新计算模式下的 WebView2 显示
void ShowBasicUsage();  // 显示基本用法界面

// 其他函数声明（在gui_main.cpp中实现）
void ExecuteSelectedItem(INT_PTR index);  // 执行选中的项目
void SearchAndDisplayResults(const WCHAR* query);  // 搜索并显示结果
void DisplayCalculationHistory();  // 显示计算历史
void SaveCalculationHistory();  // 保存计算历史
void HandleSettingsMenuItemClick(INT_PTR itemIndex);  // 处理设置菜单项点击
int GetHintRowCount();  // 获取提示行数量
INT_PTR GetFirstActualItemIndex();  // 获取第一个实际项目（跳过提示行）的索引
void UpdateSettingsMenuWebView();  // 更新设置菜单的 WebView2 显示
void UpdateHelpInfoWebView();  // 更新帮助信息的 WebView2 显示
void UpdateDirModeWebView();  // 更新目录浏览模式的 WebView2 显示
std::wstring ReadHtmlTemplate(const std::wstring& filePath);  // 读取HTML模板文件内容
void SaveWindowSettings();  // 保存窗口设置
void LoadWindowSettings(int& x, int& y, int& width, int& height);  // 加载窗口设置

// 新增函数声明（在gui_main.cpp中实现）
void ShowHelpInfo();  // 显示使用帮助信息
void EvaluateExpression(const WCHAR* expression);  // 计算表达式

void ShowSettingsMenu();  // 显示设置菜单
void CopySelectedListItem();  // 复制选中的列表项
void ShowLauncherWindow();  // 显示启动器窗口
void LogListViewContents();  // 打印ListView所有内容到日志

// 文件搜索相关函数声明
void EnterFileMode();  // 进入文件搜索模式
void ExitFileMode();  // 退出文件搜索模式
void HandleFileSearch(const WCHAR* query);  // 处理文件搜索
void UpdateFileModeWebView();  // 更新文件模式的 WebView2 显示
void ExecuteFileModeItem(INT_PTR index);  // 执行文件模式下的选中项

// SearchAndDisplayResults函数分解后的子函数声明
bool InitializeListViewForSearch();  // 初始化ListView用于搜索显示
void ProcessSearchQuery(const WCHAR* query);  // 处理搜索查询
void HandleShortcutSearch(const WCHAR* query);  // 处理快捷方式搜索
void DisplaySearchResults();  // 显示搜索结果
void UpdateWebViewForSearch();  // 更新WebView2显示搜索结果

// WindowProc函数分解后的子函数声明
LRESULT HandleWMCreate(HWND hwnd, LPCREATESTRUCTW lpCreateStruct);  // 处理WM_CREATE消息
LRESULT HandleWMHotkey(HWND hwnd, WPARAM wParam);  // 处理WM_HOTKEY消息
LRESULT HandleWMDestroy(HWND hwnd);  // 处理WM_DESTROY消息
LRESULT HandleWMTimer(HWND hwnd, WPARAM wParam);  // 处理WM_TIMER消息
LRESULT HandleWMSize(HWND hwnd, WPARAM wParam, LPARAM lParam);  // 处理WM_SIZE消息
LRESULT HandleWMExitSizeMove(HWND hwnd);  // 处理WM_EXITSIZEMOVE消息
LRESULT HandleWMSetFocus(HWND hwnd, WPARAM wParam);  // 处理WM_SETFOCUS消息
LRESULT HandleWMNotify(HWND hwnd, WPARAM wParam, LPARAM lParam);  // 处理WM_NOTIFY消息
LRESULT HandleWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);  // 处理WM_COMMAND消息
LRESULT HandleWMKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);  // 处理WM_KEYDOWN消息
LRESULT HandleWMContextMenu(HWND hwnd, WPARAM wParam);  // 处理WM_CONTEXTMENU消息

// 类型定义
struct ShortcutItem {
    WCHAR name[256];
    WCHAR path[256];
    WCHAR comment[512]; // 备注信息
    WCHAR iconPath[512]; // 图标路径
    int type; // 0 = directory, 1 = URL, 2 = application
    int usageCount;
};

#endif // COMMON_H

