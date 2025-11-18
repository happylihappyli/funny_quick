#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>

// Define max and min macros if not already defined
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#include <windowsx.h>  // 用于GET_X_LPARAM和GET_Y_LPARAM宏
#include <tchar.h>
#include <imm.h>
#include <vector>
#include <algorithm>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <shlobj.h>
#include <strsafe.h>
#include <functional>
#include "resource.h"
#include "translation_manager.h"
#include "internationalization.h"

// Define notification codes if not defined
#ifndef EN_RETURN
#define EN_RETURN 0x0100
#endif

// Global variables
HINSTANCE g_hInstance = NULL;
HWND g_hMainWindow = NULL;
HWND g_hEdit = NULL;
HWND g_hListBox = NULL;
HWND g_hExitCalcButton = NULL;  // 退出计算模式按钮
HWND g_hSettingsButton = NULL;   // 设置按钮
HWND g_hExitBookmarkButton = NULL;  // 退出网址收藏模式按钮
HWND g_hExitDirModeButton = NULL;  // 退出dir模式按钮
// HWND g_hHelpButton = NULL;       // 帮助按钮（已移除独立按钮）
// Flag to ignore EN_RETURN notifications triggered by focus changes
bool g_ignoreNextReturn = false;
bool g_dirMode = false;  // 是否处于dir模式

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
#define IDC_LISTBOX 1002
#define IDC_EXIT_CALC_BUTTON 1003  // 退出计算模式按钮ID
#define IDC_SETTINGS_BUTTON 1004    // 设置按钮ID
#define IDC_EXIT_BOOKMARK_BUTTON 1013  // 退出网址收藏模式按钮ID
#define IDC_EXIT_DIR_MODE_BUTTON 1016  // 退出dir模式按钮ID
#define IDC_HELP_BUTTON 1014        // 帮助按钮ID（已移除独立按钮）
#define ID_SETTINGS_HELP 1015        // 设置菜单帮助ID
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

// 计算模式相关变量
bool g_calculatorMode = false;  // 是否处于计算模式
bool g_updatingEditBox = false;  // 是否正在更新编辑框内容，防止触发EN_CHANGE
std::vector<std::wstring> g_calculationHistory;  // 计算历史记录
HWND g_hModeLabel = NULL;  // 模式标签控件（已移除）

// 网址收藏模式相关变量
bool g_bookmarkMode = false;  // 是否处于网址收藏模式
std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks;  // 网址收藏列表 (名称, URL)
std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults;  // 网址搜索结果
HWND g_hAddBookmarkButton = NULL;  // 添加网址按钮

// 表达式解析辅助函数声明
void EnterCalculatorMode();
void ExitCalculatorMode();
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

// 添加ExecuteSelectedItem函数声明
void ExecuteSelectedItem(int index);

// 网址管理对话框函数声明
INT_PTR CALLBACK BookmarkDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowBookmarkDialog();
void RefreshBookmarkList(HWND hList);
void AddBookmarkFromDialog(HWND hDlg);
void UpdateBookmarkFromDialog(HWND hDlg);
void DeleteBookmarkFromDialog(HWND hDlg);

// 快捷方式管理对话框函数声明
INT_PTR CALLBACK ShortcutDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowShortcutDialog();
void RefreshShortcutList(HWND hList);
void AddShortcutFromDialog(HWND hDlg);
void UpdateShortcutFromDialog(HWND hDlg);
void DeleteShortcutFromDialog(HWND hDlg);
void SaveShortcuts();
void LoadShortcuts();

// dir模式函数声明
void EnterDirMode();
void ExitDirMode();

// 设置下拉菜单相关函数声明
void ShowSettingsDropdownMenu();
HMENU CreateSettingsMenu();
void HandleSettingsMenuCommand(WPARAM wParam);

// 帮助功能相关函数声明
void ShowHelpDialog();
std::wstring GetCurrentMode();

// 添加ExecuteSelectedItem函数声明
void ExecuteSelectedItem(int index);

// 表达式解析辅助函数声明
double parseNumber(const std::wstring& expr, size_t& pos);
double parseTerm(const std::wstring& expr, size_t& pos);
double parseExpression(const std::wstring& expr, size_t& pos);

// 字体相关函数声明
void CreateUIFont();
void ApplyFontToControl(HWND hWnd);

// Log function
void LogToFile(const char* message)
{
    // Create log directory if it doesn't exist
    CreateDirectoryW(L"bin\\log", NULL);
    
    // Generate unique log filename based on current date and time
    static WCHAR logFileName[MAX_PATH] = {0};
    static bool fileNameGenerated = false;
    static bool bomWritten = false;
    
    if (!fileNameGenerated) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        // Format: bin\log\quick_launcher_YYYYMMDD_HHMMSS.log
        wsprintfW(logFileName, L"bin\\log\\quick_launcher_%04d%02d%02d_%02d%02d%02d.log",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
        fileNameGenerated = true;
    }
    
    // Open log file in binary append mode
    FILE* file = _wfopen(logFileName, L"ab");
    if (file)
    {
        // Write BOM only once when file is created
        if (!bomWritten)
        {
            // Check if file is empty
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            if (fileSize == 0)
            {
                // Write UTF-8 BOM
                const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
                fwrite(bom, 1, 3, file);
            }
            bomWritten = true;
        }
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        // Create full log entry with timestamp and message
        WCHAR fullLogEntry[2048] = {0};
        wsprintfW(fullLogEntry, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] %hs\n", 
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, message);
        
        // Convert to UTF-8 and write
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, fullLogEntry, -1, NULL, 0, NULL, NULL);
        if (utf8Size > 0)
        {
            char* utf8Buffer = new char[utf8Size];
            WideCharToMultiByte(CP_UTF8, 0, fullLogEntry, -1, utf8Buffer, utf8Size, NULL, NULL);
            fwrite(utf8Buffer, 1, utf8Size - 1, file); // -1 to exclude null terminator
            delete[] utf8Buffer;
        }
        
        fflush(file);
        fclose(file);
    }
    else
    {
        // If we can't open the log file, try to create a simple error log
        FILE* errorFile = fopen("log_error.txt", "a");
        if (errorFile)
        {
            fprintf(errorFile, "Failed to open log file. Message: %s\n", message);
            fclose(errorFile);
        }
    }
}

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
    double result = parseNumber(expr, pos);
    while (pos < expr.length()) {
        if (expr[pos] == L'*') {
            pos++;
            result *= parseNumber(expr, pos);
        }
        else if (expr[pos] == L'/') {
            pos++;
            result /= parseNumber(expr, pos);
        }
        else {
            break;
        }
    }
    return result;
}

double parseExpression(const std::wstring& expr, size_t& pos) {
    double result = parseTerm(expr, pos);
    while (pos < expr.length()) {
        if (expr[pos] == L'+') {
            pos++;
            result += parseTerm(expr, pos);
        }
        else if (expr[pos] == L'-') {
            pos++;
            result -= parseTerm(expr, pos);
        }
        else {
            break;
        }
    }
    return result;
}

// 字体相关函数实现
void CreateUIFont() {
    // 直接创建支持中文的字体，不使用系统默认字体
    g_hFont = CreateFontW(
        16,  // 字体高度
        0,   // 字体宽度（0表示使用默认宽高比）
        0,   // 文本角度
        0,   // 字符角度
        FW_NORMAL,  // 字体粗细
        FALSE,      // 非斜体
        FALSE,      // 非下划线
        FALSE,      // 非删除线
        DEFAULT_CHARSET,  // 字符集，支持中文
        OUT_DEFAULT_PRECIS,  // 输出精度
        CLIP_DEFAULT_PRECIS, // 裁剪精度
        DEFAULT_QUALITY,     // 输出质量
        DEFAULT_PITCH | FF_DONTCARE,  // 字体系列
        L"Microsoft YaHei"  // 使用微软雅黑字体
    );
    
    // 如果微软雅黑字体创建失败，尝试使用SimSun（宋体）
    if (!g_hFont) {
        g_hFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"SimSun"  // 使用宋体作为备选
        );
    }
    
    // 如果仍然失败，使用系统默认字体
    if (!g_hFont) {
        g_hFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            NULL  // 使用系统默认字体
        );
    }
}

void ApplyFontToControl(HWND hWnd) {
    if (g_hFont) {
        SendMessageW(hWnd, WM_SETFONT, (WPARAM)g_hFont, MAKELPARAM(TRUE, 0));
    }
}

// Function to list directory contents
void ListDirectoryContents(const WCHAR* path)
{
    LogToFile("ListDirectoryContents: 开始列出目录内容");
    
    // Convert path to wide string for logging
    char pathLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, path, -1, pathLog, sizeof(pathLog), NULL, NULL);
    char logMsg[1100] = {0};
    sprintf(logMsg, "ListDirectoryContents: 列出目录 '%s' 的内容", pathLog);
    LogToFile(logMsg);
    
    // Check if path is valid
    DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        LogToFile("ListDirectoryContents: 无效的目录路径");
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"Invalid directory path");
        return;
    }
    
    if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        LogToFile("ListDirectoryContents: 路径不是目录");
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"Path is not a directory");
        return;
    }
    
    // Clear previous results
    ListView_DeleteAllItems(g_hListBox);
    
    // Show directory path
    WCHAR header[1024] = {0};
    wsprintfW(header, L"Directory contents of: %s", path);
    // 使用ListView显示目录路径
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = header;
    ListView_InsertItem(g_hListBox, &lvi);
    
    // 添加空行
    lvi.iItem = 1;
    lvi.pszText = L"";
    ListView_InsertItem(g_hListBox, &lvi);
    
    // Prepare search pattern
    WCHAR searchPath[MAX_PATH] = {0};
    wcscpy(searchPath, path);
    
    // Ensure path ends with backslash
    size_t len = wcslen(searchPath);
    if (len > 0 && searchPath[len - 1] != L'\\')
    {
        wcscat(searchPath, L"\\");
    }
    
    wcscat(searchPath, L"*");
    
    // Find files and directories
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
    {
        LogToFile("ListDirectoryContents: 无法列出目录内容");
        // 使用ListView显示错误信息
        lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = L"Unable to list directory contents";
        ListView_InsertItem(g_hListBox, &lvi);
        return;
    }
    
    int fileCount = 0;
    int dirCount = 0;
    
    do
    {
        // Skip . and .. directories
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
        {
            continue;
        }
        
        // Create display string with [DIR] or [FILE] prefix
        WCHAR display[1024] = {0};
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            wsprintfW(display, L"[DIR]  %s", findData.cFileName);
            dirCount++;
        }
        else
        {
            wsprintfW(display, L"[FILE] %s", findData.cFileName);
            fileCount++;
        }
        
        // 使用ListView显示文件/目录项
        lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = fileCount + dirCount + 2; // +2是因为前面有标题和空行
        lvi.iSubItem = 0;
        lvi.pszText = display;
        ListView_InsertItem(g_hListBox, &lvi);
        
    } while (FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    
    // Show summary
    WCHAR summary[1024] = {0};
    // 添加空行
    lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = fileCount + dirCount + 2;
    lvi.iSubItem = 0;
    lvi.pszText = L"";
    ListView_InsertItem(g_hListBox, &lvi);
    
    // 显示统计信息
    wsprintfW(summary, L"Total: %d directories, %d files", dirCount, fileCount);
    lvi.iItem = fileCount + dirCount + 3;
    lvi.pszText = summary;
    ListView_InsertItem(g_hListBox, &lvi);
    
    sprintf(logMsg, "ListDirectoryContents: 列出完成，共 %d 个目录，%d 个文件", dirCount, fileCount);
    LogToFile(logMsg);
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
    
    // 检查是否是"dir"命令
    if (wcscmp(command, L"dir") == 0)
    {
        LogToFile("ProcessCommand: 识别为'dir'命令");
        // 使用ListView显示提示信息
        ListView_DeleteAllItems(g_hListBox);
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = L"Please enter directory path:";
        ListView_InsertItem(g_hListBox, &lvi);
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
    
    // Clear previous results
    ListView_DeleteAllItems(g_hListBox);
    
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
            // 使用ListView显示提示信息
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = feedback;
            ListView_InsertItem(g_hListBox, &lvi);
            LogToFile("ProcessCommand: 添加http://前缀");
        }
        else
        {
            wcscpy(fullUrl, command);
            LogToFile("ProcessCommand: 使用原始URL");
        }
        
        // Show URL being opened
        wsprintfW(feedback, L"Opening URL: %s", fullUrl);
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
        
        // Open URL with default browser
        HINSTANCE result = ShellExecuteW(NULL, L"open", fullUrl, NULL, NULL, SW_SHOWNORMAL);
        
        // Log the URL opening attempt
        char urlLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, fullUrl, -1, urlLog, sizeof(urlLog), NULL, NULL);
        char finalLog[1024] = {0};
        sprintf(finalLog, "ProcessCommand: 打开URL '%s', ShellExecuteW返回值: %p", urlLog, result);
        LogToFile(finalLog);
        
        if ((INT_PTR)result > 32)
        {
            wsprintfW(feedback, L"Success! URL opened in default browser");
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            LogToFile("ProcessCommand: URL打开成功");
        }
        else
        {
            wsprintfW(feedback, L"Failed to open URL: error code %p", result);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            sprintf(finalLog, "ProcessCommand: URL打开失败，错误代码 %p", result);
            LogToFile(finalLog);
        }
    }
    // Handle file paths
    else if (GetFileAttributesW(command) != INVALID_FILE_ATTRIBUTES)
    {
        // Check if it's a directory
        DWORD attributes = GetFileAttributesW(command);
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            LogToFile("ProcessCommand: 识别为目录路径，列出目录内容");
            ListDirectoryContents(command);
        }
        else
        {
            LogToFile("ProcessCommand: 识别为文件路径");
            WCHAR feedback[1024] = {0};
            wsprintfW(feedback, L"Opening file: %s", command);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            
            HINSTANCE result = ShellExecuteW(NULL, L"open", command, NULL, NULL, SW_SHOWNORMAL);
            
            sprintf(logMsg, "ProcessCommand: 打开文件 '%s', ShellExecuteW返回值: %p", commandLog, result);
            LogToFile(logMsg);
            
            if ((INT_PTR)result > 32)
            {
                wsprintfW(feedback, L"File opened successfully");
                SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
                LogToFile("ProcessCommand: 文件打开成功");
            }
            else
            {
                wsprintfW(feedback, L"Failed to open file: error code %p", result);
                SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
                sprintf(logMsg, "ProcessCommand: 文件打开失败，错误代码 %p", result);
                LogToFile(logMsg);
            }
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
                ExecuteSelectedItem((int)i);
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            sprintf(logMsg, "ProcessCommand: 未找到匹配的命令或文件 '%s'", commandLog);
            LogToFile(logMsg);
            // 使用ListView显示提示信息
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            lvi.iSubItem = 0;
            lvi.pszText = L"No matching command or file found";
            ListView_InsertItem(g_hListBox, &lvi);
        }
    }
}

// Execute selected item
void ExecuteSelectedItem(int index)
{
    if (index < 0 || index >= static_cast<int>(g_shortcuts.size()))
    {
        LogToFile("ExecuteSelectedItem: 索引超出范围");
        return;
    }
    
    // Get the shortcut item
    ShortcutItem& item = g_shortcuts[index];
    
    // Log execution details
    char nameLog[256] = {0};
    char pathLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, item.name, -1, nameLog, sizeof(nameLog), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, item.path, -1, pathLog, sizeof(pathLog), NULL, NULL);
    char logMsg[1024] = {0};
    sprintf(logMsg, "ExecuteSelectedItem: 执行项目 '%s' (路径: '%s', 类型: %d, 使用次数: %d)", 
            nameLog, pathLog, item.type, item.usageCount);
    LogToFile(logMsg);
    
    // Execute based on type
    HINSTANCE result;
    if (item.type == 0) 
    {
        // Directory
        LogToFile("ExecuteSelectedItem: 执行目录");
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    else if (item.type == 1) 
    {
        // URL
        LogToFile("ExecuteSelectedItem: 执行URL");
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    else 
    {
        // Application
        LogToFile("ExecuteSelectedItem: 执行应用程序");
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    
    // Log the result
    sprintf(logMsg, "ExecuteSelectedItem: ShellExecuteW 返回值: %p", result);
    LogToFile(logMsg);
    
    // Update usage count
    item.usageCount++;
    sprintf(logMsg, "ExecuteSelectedItem: 更新使用次数为 %d", item.usageCount);
    LogToFile(logMsg);
    
    // Save shortcuts
    SaveShortcuts();
    
    // Check if execution was successful
    if ((INT_PTR)result > 32)
    {
        LogToFile("ExecuteSelectedItem: 执行成功");
        // Clear the edit box
        SetWindowTextW(g_hEdit, L"");
        // Clear the listview
        ListView_DeleteAllItems(g_hListBox);
    }
    else
    {
        LogToFile("ExecuteSelectedItem: 执行失败");
        WCHAR errorMsg[1024] = {0};
        wsprintfW(errorMsg, L"Failed to execute item: error code %p", result);
        // 使用ListView显示错误信息
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = errorMsg;
        ListView_InsertItem(g_hListBox, &lvi);
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
}

// Evaluate expression and display result
void EvaluateExpression(const WCHAR* expression)
{
    LogToFile("EvaluateExpression: 开始计算表达式");
    
    // 打印到控制台（用于调试）
    wprintf(L"[DEBUG] EvaluateExpression 开始处理: '%s'\n", expression);
    
    // Convert to std::wstring for easier manipulation
    std::wstring fullExpr(expression);
    std::wstring expr = fullExpr;
    std::wstring comment;
    
    // Log the expression being evaluated
    char exprLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, expression, -1, exprLog, sizeof(exprLog), NULL, NULL);
    char logMsg[1100] = {0};
    sprintf(logMsg, "EvaluateExpression: 计算表达式 '%s'", exprLog);
    LogToFile(logMsg);
    
    // Check for comment
    wprintf(L"[DEBUG] 检查#注释...\n");
    size_t commentPos = fullExpr.find(L"#");
    if (commentPos != std::wstring::npos)
    {
        // Extract comment
        comment = fullExpr.substr(commentPos + 1);
        // Remove comment from expression
        expr = fullExpr.substr(0, commentPos);
        
        // 调试日志
        wprintf(L"[DEBUG] 发现注释: commentPos=%zu, comment='%s', 处理后表达式='%s'\n", commentPos, comment.c_str(), expr.c_str());
    }
    else
    {
        wprintf(L"[DEBUG] 未发现#注释\n");
    }
    
    // Remove spaces from expression
    expr.erase(std::remove(expr.begin(), expr.end(), L' '), expr.end());
    
    // Check if expression is empty
    if (expr.empty())
    {
        LogToFile("EvaluateExpression: 表达式为空");
        return;
    }
    
    try
    {
        // Parse and evaluate expression
        size_t pos = 0;
        double result = parseExpression(expr, pos);
        
        // Check if entire expression was parsed
        if (pos != expr.length())
        {
            LogToFile("EvaluateExpression: 表达式解析不完整");
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"Invalid expression");
            return;
        }
        
        // Format result
        WCHAR resultStr[1024] = {0};
        wprintf(L"[DEBUG] 格式化结果: expr='%s', result=%.6f, comment='%s'\n", expr.c_str(), result, comment.c_str());
        
        if (result == (int)result)
        {
            // If result is an integer, display without decimal places
            if (!comment.empty())
            {
                wsprintfW(resultStr, L"%s = %d # %s", expr.c_str(), (int)result, comment.c_str());
            }
            else
            {
                wsprintfW(resultStr, L"%s = %d", expr.c_str(), (int)result);
            }
        }
        else
        {
            // If result is a floating point number, display with 6 decimal places
            if (!comment.empty())
            {
                swprintf(resultStr, 1024, L"%s = %.6f # %s", expr.c_str(), result, comment.c_str());
            }
            else
            {
                swprintf(resultStr, 1024, L"%s = %.6f", expr.c_str(), result);
            }
        }
        
        wprintf(L"[DEBUG] 格式化后的resultStr='%s'\n", resultStr);
        
        // Add to calculation history
        g_calculationHistory.push_back(resultStr);
        wprintf(L"[DEBUG] 添加到历史记录，当前历史记录大小: %zu\n", g_calculationHistory.size());
        
        // Keep only last 50 calculations
        if (g_calculationHistory.size() > 50)
        {
            g_calculationHistory.erase(g_calculationHistory.begin());
        }
        
        // Save calculation history
        SaveCalculationHistory();
        wprintf(L"[DEBUG] 保存历史记录完成\n");
        
        // Display result using DisplayCalculationHistory to properly format columns
        wprintf(L"[DEBUG] 调用DisplayCalculationHistory()显示计算结果\n");
        DisplayCalculationHistory();
        
        // Convert resultStr to UTF-8 for logging
        char resultLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, resultStr, -1, resultLog, sizeof(resultLog), NULL, NULL);
        sprintf(logMsg, "EvaluateExpression: 计算完成，结果: '%s'", resultLog);
        LogToFile(logMsg);
    }
    catch (...)
    {
        LogToFile("EvaluateExpression: 计算过程中发生异常");
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"Error evaluating expression");
    }
}

// Display calculation history
void DisplayCalculationHistory()
{
    LogToFile("DisplayCalculationHistory: 显示计算历史");
    wprintf(L"[DEBUG] DisplayCalculationHistory: 开始显示计算历史，历史记录数量: %zu\n", g_calculationHistory.size());
    
    // Clear current list
    ListView_DeleteAllItems(g_hListBox);
    wprintf(L"[DEBUG] 清空ListView完成\n");
    
    // 如果历史记录为空，显示提示信息
    if (g_calculationHistory.empty()) {
        // 使用ListView显示提示信息，确保正确设置列
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = L"=== 已进入计算模式 ===";
        int itemIndex = ListView_InsertItem(g_hListBox, &lvi);
        
        // 为第一行添加第二列内容
        LVITEMW lvi2 = {0};
        lvi2.mask = LVIF_TEXT;
        lvi2.iItem = itemIndex;
        lvi2.iSubItem = 1;
        lvi2.pszText = L"提示信息";
        ListView_SetItem(g_hListBox, &lvi2);
        
        lvi.iItem = 1;
        lvi.iSubItem = 0;
        lvi.pszText = L"请输入数学表达式进行计算";
        itemIndex = ListView_InsertItem(g_hListBox, &lvi);
        
        lvi.iItem = 2;
        lvi.pszText = L"例如: 2+3*4 或 1+2 #测试中文";
        itemIndex = ListView_InsertItem(g_hListBox, &lvi);
        
        lvi.iItem = 3;
        lvi.pszText = L"按Enter键计算，输入'q'退出计算器模式";
        itemIndex = ListView_InsertItem(g_hListBox, &lvi);
        
        lvi.iItem = 4;
        lvi.pszText = L"========================";
        ListView_InsertItem(g_hListBox, &lvi);
        
        return;
    }
    
    // Display history in reverse order (newest first)
    for (int i = (int)g_calculationHistory.size() - 1; i >= 0; i--)
    {
        wprintf(L"[DEBUG] 处理历史记录项 %d: '%s'\n", i, g_calculationHistory[i].c_str());
        // Parse the calculation result to separate expression and comment
        std::wstring calcResult = g_calculationHistory[i];
        std::wstring expression;
        std::wstring comment;
        
        // Find the comment separator "#"
        size_t commentPos = calcResult.find(L" # ");
        if (commentPos != std::wstring::npos)
        {
            // Separate expression and comment
            expression = calcResult.substr(0, commentPos);
            comment = calcResult.substr(commentPos + 3);  // Skip " # "
        }
        else
        {
            // Try finding "#" without spaces
            commentPos = calcResult.find(L"#");
            if (commentPos != std::wstring::npos)
            {
                // Separate expression and comment
                expression = calcResult.substr(0, commentPos);
                // Remove any leading whitespace from comment
                size_t commentStart = commentPos + 1;
                while (commentStart < calcResult.length() && iswspace(calcResult[commentStart]))
                {
                    commentStart++;
                }
                comment = calcResult.substr(commentStart);
            }
            else
            {
                // No comment, use the whole string as expression
                expression = calcResult;
            }
        }
        
        // Add item to listview
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = ListView_GetItemCount(g_hListBox);
        lvi.iSubItem = 0;
        lvi.pszText = expression.c_str();
        int itemIndex = ListView_InsertItem(g_hListBox, &lvi);
        
        // Add comment to the second column
        if (!comment.empty())
        {
            LVITEMW lviComment = {0};
            lviComment.mask = LVIF_TEXT;
            lviComment.iItem = itemIndex;
            lviComment.iSubItem = 1;
            lviComment.pszText = comment.c_str();
            ListView_SetItem(g_hListBox, &lviComment);
        }
    }
    
    LogToFile("DisplayCalculationHistory: 计算历史显示完成");
}

// Save calculation history to file
void SaveCalculationHistory()
{
    LogToFile("SaveCalculationHistory: 开始保存计算历史");
    
    // Create data directory if it doesn't exist
    CreateDirectoryW(L"data", NULL);
    
    // Open history file
    FILE* file = _wfopen(L"data\\calc_history.txt", L"w, ccs=UTF-8");
    if (file)
    {
        // Write each calculation to file
        for (const auto& calc : g_calculationHistory)
        {
            fwprintf(file, L"%s\n", calc.c_str());
        }
        
        fclose(file);
        LogToFile("SaveCalculationHistory: 计算历史保存成功");
    }
    else
    {
        LogToFile("SaveCalculationHistory: 无法打开计算历史文件进行写入");
    }
}

// Load calculation history from file
void LoadCalculationHistory()
{
    LogToFile("LoadCalculationHistory: 开始加载计算历史");
    
    // Clear current history
    g_calculationHistory.clear();
    
    // Open history file
    FILE* file = _wfopen(L"data\\calc_history.txt", L"r, ccs=UTF-8");
    if (file)
    {
        // Read each line from file
        WCHAR line[1024] = {0};
        while (fgetws(line, sizeof(line)/sizeof(WCHAR), file))
        {
            // Remove newline character if present
            size_t len = wcslen(line);
            if (len > 0 && line[len-1] == L'\n')
            {
                line[len-1] = L'\0';
            }
            
            // Add to history if not empty
            if (wcslen(line) > 0)
            {
                g_calculationHistory.push_back(line);
            }
        }
        
        fclose(file);
        LogToFile("LoadCalculationHistory: 计算历史加载成功");
    }
    else
    {
        LogToFile("LoadCalculationHistory: 无法打开计算历史文件进行读取");
    }
}

// Enter calculator mode
void EnterCalculatorMode()
{
    LogToFile("EnterCalculatorMode: 进入计算模式");
    
    // 设置计算模式标志
    g_calculatorMode = true;
    
    // 更新模式标签文本
    // 移除了g_hModeLabel，不再设置标签文字
    
    // 显示退出计算模式按钮（保留但不使用，通过菜单和'q'键退出）
    ShowWindow(g_hExitCalcButton, SW_SHOW);
    
    // 显示设置按钮（在计算模式下用户仍需要访问设置）
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 隐藏退出网址收藏模式按钮（如果显示）
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 显示计算历史记录
    DisplayCalculationHistory();
    
    // 添加友好的状态栏提示（使用ListView正确显示）
    // 注意：g_hListBox是ListView控件，不是ListBox控件
    // 提示信息将在DisplayCalculationHistory中显示
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Exit calculator mode
void ExitCalculatorMode()
{
    LogToFile("ExitCalculatorMode: 退出计算模式");
    
    // 清除计算模式标志
    g_calculatorMode = false;
    
    // 更新模式标签文本
    // 移除了g_hModeLabel，不再设置标签文字
    
    // 隐藏退出计算模式按钮
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    
    // 显示设置按钮
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 隐藏退出网址收藏模式按钮（如果显示）
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListBox);
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Enter bookmark mode
void EnterBookmarkMode()
{
    LogToFile("EnterBookmarkMode: 进入网址收藏模式");
    
    // 设置网址收藏模式标志
    g_bookmarkMode = true;
    
    // 更新模式标签文本
    // 移除了g_hModeLabel，不再设置标签文字
    
    // 显示退出网址收藏模式按钮
    ShowWindow(g_hExitBookmarkButton, SW_SHOW);
    
    // 显示设置按钮（在网址收藏模式下也显示）
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 隐藏退出计算模式按钮（如果显示）
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    
    // 显示列表框
    ShowWindow(g_hListBox, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 加载并显示网址收藏
    LoadBookmarks();
    DisplayBookmarkResults();
    
    // 添加提示信息
    MessageBoxW(g_hMainWindow, L"已进入网址收藏模式\n请输入网址名称进行搜索\n按Enter键打开选中的网址\n输入'q'退出网址收藏模式", L"提示", MB_OK | MB_ICONINFORMATION);
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Exit bookmark mode
void ExitBookmarkMode()
{
    LogToFile("ExitBookmarkMode: 退出网址收藏模式");
    
    // 清除网址收藏模式标志
    g_bookmarkMode = false;
    
    // 更新模式标签文本
    // 移除了g_hModeLabel，不再设置标签文字
    
    // 隐藏退出网址收藏模式按钮
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    
    // 显示设置按钮
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 清空列表框
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Enter dir mode
void EnterDirMode()
{
    LogToFile("EnterDirMode: 进入dir模式");
    
    // 设置dir模式标志
    g_dirMode = true;
    
    // 修改窗口标题
    SetWindowTextW(g_hMainWindow, L"脚本交互模式");
    
    // 显示退出dir模式按钮
    ShowWindow(g_hExitDirModeButton, SW_SHOW);
    
    // 隐藏设置按钮
    ShowWindow(g_hSettingsButton, SW_HIDE);
    
    // 隐藏退出计算模式按钮（如果显示）
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    
    // 隐藏退出网址收藏模式按钮（如果显示）
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 在列表框中显示提示信息
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"请输入目录路径，例如: C:\\Users");
    SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"输入 'q' 退出该模式");
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Exit dir mode
void ExitDirMode()
{
    LogToFile("ExitDirMode: 退出dir模式");
    
    // 清除dir模式标志
    g_dirMode = false;
    
    // 恢复窗口标题
    SetWindowTextW(g_hMainWindow, L"快捷启动器");
    
    // 隐藏退出dir模式按钮
    ShowWindow(g_hExitDirModeButton, SW_HIDE);
    
    // 显示设置按钮
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 清空列表框
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

// Add bookmark
void AddBookmark(const WCHAR* name, const WCHAR* url)
{
    LogToFile("AddBookmark: 添加网址收藏");
    
    // 验证URL格式
    if (!IsURL(url))
    {
        LogToFile("AddBookmark: URL格式无效");
        MessageBoxW(g_hMainWindow, TTR(L"Invalid URL format").c_str(), TTR(L"Add Bookmark Failed").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 检查是否已存在相同的网址
    for (const auto& bookmark : g_bookmarks)
    {
        if (bookmark.second == url)
        {
            LogToFile("AddBookmark: 网址已存在");
            MessageBoxW(g_hMainWindow, TTR(L"URL already exists").c_str(), TTR(L"Add Bookmark Failed").c_str(), MB_OK | MB_ICONWARNING);
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

// Delete bookmark
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

// Save bookmarks to file
void SaveBookmarks()
{
    LogToFile("SaveBookmarks: 开始保存网址收藏");
    
    // 创建数据目录（如果不存在）
    CreateDirectoryW(L"data", NULL);
    
    // 打开网址收藏文件
    FILE* file = _wfopen(L"data\\bookmarks.txt", L"w, ccs=UTF-8");
    if (file)
    {
        // 写入每个网址收藏项
        for (const auto& bookmark : g_bookmarks)
        {
            fwprintf(file, L"%s|%s\n", bookmark.first.c_str(), bookmark.second.c_str());
        }
        
        fclose(file);
        LogToFile("SaveBookmarks: 网址收藏保存成功");
    }
    else
    {
        LogToFile("SaveBookmarks: 无法打开网址收藏文件进行写入");
    }
}

// Load bookmarks from file
void LoadBookmarks()
{
    LogToFile("LoadBookmarks: 开始加载网址收藏");
    
    // 清空当前网址收藏列表
    g_bookmarks.clear();
    
    // 打开网址收藏文件
    FILE* file = _wfopen(L"data\\bookmarks.txt", L"r, ccs=UTF-8");
    if (file)
    {
        // 读取每一行
        WCHAR line[2048] = {0};
        while (fgetws(line, sizeof(line)/sizeof(WCHAR), file))
        {
            // 移除换行符（如果存在）
            size_t len = wcslen(line);
            if (len > 0 && line[len-1] == L'\n')
            {
                line[len-1] = L'\0';
            }
            
            // 分割名称和URL
            WCHAR* separator = wcschr(line, L'|');
            if (separator)
            {
                *separator = L'\0';  // 分割字符串
                WCHAR* name = line;
                WCHAR* url = separator + 1;
                
                // 添加到网址收藏列表
                if (wcslen(name) > 0 && wcslen(url) > 0)
                {
                    g_bookmarks.push_back(std::make_pair(std::wstring(name), std::wstring(url)));
                }
            }
        }
        
        fclose(file);
        LogToFile("LoadBookmarks: 网址收藏加载成功");
    }
    else
    {
        LogToFile("LoadBookmarks: 无法打开网址收藏文件进行读取");
    }
}

// Sync Chrome bookmarks
void SyncChromeBookmarks()
{
    LogToFile("SyncChromeBookmarks: 开始同步Chrome书签");
    
    // 获取Chrome书签文件路径
    WCHAR chromeBookmarksPath[MAX_PATH] = {0};
    if (SHGetSpecialFolderPathW(NULL, chromeBookmarksPath, CSIDL_LOCAL_APPDATA, FALSE))
    {
        wcscat(chromeBookmarksPath, L"\\Google\\Chrome\\User Data\\Default\\Bookmarks");
        
        // 检查文件是否存在
        if (GetFileAttributesW(chromeBookmarksPath) != INVALID_FILE_ATTRIBUTES)
        {
            // TODO: 解析Chrome书签文件并同步
            // 这需要JSON解析功能，暂时显示提示信息
            MessageBoxW(g_hMainWindow, TTR(L"Chrome书签同步功能尚未实现").c_str(), TTR(L"功能提示").c_str(), MB_OK | MB_ICONINFORMATION);
            LogToFile("SyncChromeBookmarks: Chrome书签同步功能尚未实现");
        }
        else
        {
            LogToFile("SyncChromeBookmarks: 未找到Chrome书签文件");
            MessageBoxW(g_hMainWindow, TTR(L"未找到Chrome书签文件").c_str(), TTR(L"同步失败").c_str(), MB_OK | MB_ICONERROR);
        }
    }
    else
    {
        LogToFile("SyncChromeBookmarks: 无法获取Chrome书签路径");
        MessageBoxW(g_hMainWindow, TTR(L"无法获取Chrome书签路径").c_str(), TTR(L"同步失败").c_str(), MB_OK | MB_ICONERROR);
    }
}

// Search bookmarks
void SearchBookmarks(const WCHAR* query)
{
    LogToFile("SearchBookmarks: 开始搜索网址收藏");
    
    // 清空搜索结果
    g_bookmarkSearchResults.clear();
    
    // 如果查询为空，显示所有书签
    if (wcslen(query) == 0)
    {
        g_bookmarkSearchResults = g_bookmarks;
    }
    else
    {
        // 转换查询为小写以便进行不区分大小写的搜索
        std::wstring lowerQuery(query);
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
        
        // 搜索匹配的书签
        for (const auto& bookmark : g_bookmarks)
        {
            // 转换书签名称为小写
            std::wstring lowerName = bookmark.first;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);
            
            // 检查是否匹配
            if (lowerName.find(lowerQuery) != std::wstring::npos)
            {
                g_bookmarkSearchResults.push_back(bookmark);
            }
        }
    }
    
    LogToFile("SearchBookmarks: 网址收藏搜索完成");
}

// Display bookmark results
void DisplayBookmarkResults()
{
    LogToFile("DisplayBookmarkResults: 显示网址收藏结果");
    
    // 清空列表框
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    
    // 显示搜索结果
    for (const auto& bookmark : g_bookmarkSearchResults)
    {
        WCHAR displayText[512] = {0};
        wsprintfW(displayText, L"收藏: %s", bookmark.first.c_str());
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)displayText);
    }
    
    LogToFile("DisplayBookmarkResults: 网址收藏结果显示完成");
}

// Check if text is URL
bool IsURL(const WCHAR* text)
{
    return (wcsstr(text, L"http://") == text || 
            wcsstr(text, L"https://") == text || 
            wcsstr(text, L"www.") == text);
}

// Bookmark dialog procedure
INT_PTR CALLBACK BookmarkDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            LogToFile("BookmarkDialogProc: 初始化网址管理对话框");
            // 初始化对话框控件
            RefreshBookmarkList(GetDlgItem(hwnd, IDC_BOOKMARK_LIST));
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    LogToFile("BookmarkDialogProc: 用户点击确定按钮");
                    EndDialog(hwnd, IDOK);
                    return TRUE;
                    
                case IDCANCEL:
                    LogToFile("BookmarkDialogProc: 用户点击取消按钮");
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
                    
                case IDC_BOOKMARK_ADD:
                    LogToFile("BookmarkDialogProc: 用户点击添加网址按钮");
                    AddBookmarkFromDialog(hwnd);
                    return TRUE;
                    
                case IDC_BOOKMARK_UPDATE:
                    LogToFile("BookmarkDialogProc: 用户点击更新网址按钮");
                    UpdateBookmarkFromDialog(hwnd);
                    return TRUE;
                    
                case IDC_BOOKMARK_DELETE:
                    LogToFile("BookmarkDialogProc: 用户点击删除网址按钮");
                    DeleteBookmarkFromDialog(hwnd);
                    return TRUE;
                    
                case IDC_SYNC_CHROME_BUTTON:
                    LogToFile("BookmarkDialogProc: 用户点击同步Chrome书签按钮");
                    SyncChromeBookmarks();
                    return TRUE;
            }
            break;
            
        case WM_CLOSE:
            LogToFile("BookmarkDialogProc: 用户关闭对话框");
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
    }
    
    return FALSE;
}

// Show bookmark dialog
void ShowBookmarkDialog()
{
    LogToFile("ShowBookmarkDialog: 显示网址管理对话框");
    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_BOOKMARK_DIALOG), g_hMainWindow, BookmarkDialogProc);
}

// Refresh bookmark list
void RefreshBookmarkList(HWND hList)
{
    LogToFile("RefreshBookmarkList: 刷新网址列表");
    
    // 清空列表
    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
    
    // 添加所有网址收藏项
    for (size_t i = 0; i < g_bookmarks.size(); i++)
    {
        WCHAR itemText[512] = {0};
        wsprintfW(itemText, L"%s - %s", g_bookmarks[i].first.c_str(), g_bookmarks[i].second.c_str());
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)itemText);
    }
    
    LogToFile("RefreshBookmarkList: 网址列表刷新完成");
}

// Add bookmark from dialog
void AddBookmarkFromDialog(HWND hDlg)
{
    LogToFile("AddBookmarkFromDialog: 从对话框添加网址");
    
    // 获取输入的名称和URL
    WCHAR name[256] = {0};
    WCHAR url[512] = {0};
    
    GetDlgItemTextW(hDlg, IDC_BOOKMARK_NAME, name, sizeof(name)/sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_BOOKMARK_URL, url, sizeof(url)/sizeof(WCHAR));
    
    // 检查输入是否为空
    if (wcslen(name) == 0 || wcslen(url) == 0)
    {
        LogToFile("AddBookmarkFromDialog: 名称或URL为空");
        MessageBoxW(hDlg, TTR(L"名称和URL不能为空").c_str(), TTR(L"输入错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 添加书签
    AddBookmark(name, url);
    
    // 刷新列表
    RefreshBookmarkList(GetDlgItem(hDlg, IDC_BOOKMARK_LIST));
    
    // 清空输入框
    SetDlgItemTextW(hDlg, IDC_BOOKMARK_NAME, L"");
    SetDlgItemTextW(hDlg, IDC_BOOKMARK_URL, L"");
    
    LogToFile("AddBookmarkFromDialog: 网址添加完成");
}

// Update bookmark from dialog
void UpdateBookmarkFromDialog(HWND hDlg)
{
    LogToFile("UpdateBookmarkFromDialog: 从对话框更新网址");
    
    // 获取选中的项目索引
    HWND hList = GetDlgItem(hDlg, IDC_BOOKMARK_LIST);
    int selectedIndex = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    
    if (selectedIndex == LB_ERR)
    {
        LogToFile("UpdateBookmarkFromDialog: 未选择任何项目");
        MessageBoxW(hDlg, TTR(L"请选择要更新的网址").c_str(), TTR(L"操作错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 获取输入的名称和URL
    WCHAR name[256] = {0};
    WCHAR url[512] = {0};
    
    GetDlgItemTextW(hDlg, IDC_BOOKMARK_NAME, name, sizeof(name)/sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_BOOKMARK_URL, url, sizeof(url)/sizeof(WCHAR));
    
    // 检查输入是否为空
    if (wcslen(name) == 0 || wcslen(url) == 0)
    {
        LogToFile("UpdateBookmarkFromDialog: 名称或URL为空");
        MessageBoxW(hDlg, TTR(L"名称和URL不能为空").c_str(), TTR(L"输入错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 更新书签
    if (selectedIndex < static_cast<int>(g_bookmarks.size()))
    {
        g_bookmarks[selectedIndex].first = name;
        g_bookmarks[selectedIndex].second = url;
        
        // 保存到文件
        SaveBookmarks();
        
        // 刷新列表
        RefreshBookmarkList(hList);
        
        // 清空输入框
        SetDlgItemTextW(hDlg, IDC_BOOKMARK_NAME, L"");
        SetDlgItemTextW(hDlg, IDC_BOOKMARK_URL, L"");
        
        LogToFile("UpdateBookmarkFromDialog: 网址更新完成");
    }
}

// Delete bookmark from dialog
void DeleteBookmarkFromDialog(HWND hDlg)
{
    LogToFile("DeleteBookmarkFromDialog: 从对话框删除网址");
    
    // 获取选中的项目索引
    HWND hList = GetDlgItem(hDlg, IDC_BOOKMARK_LIST);
    int selectedIndex = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    
    if (selectedIndex == LB_ERR)
    {
        LogToFile("DeleteBookmarkFromDialog: 未选择任何项目");
        MessageBoxW(hDlg, TTR(L"请选择要删除的网址").c_str(), TTR(L"操作错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 确认删除
    if (MessageBoxW(hDlg, TTR(L"确定要删除选中的网址吗？").c_str(), TTR(L"确认删除").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        // 删除书签
        DeleteBookmark(selectedIndex);
        
        // 刷新列表
        RefreshBookmarkList(hList);
        
        // 清空输入框
        SetDlgItemTextW(hDlg, IDC_BOOKMARK_NAME, L"");
        SetDlgItemTextW(hDlg, IDC_BOOKMARK_URL, L"");
        
        LogToFile("DeleteBookmarkFromDialog: 网址删除成功");
    }
}

// Shortcut dialog procedure
INT_PTR CALLBACK ShortcutDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            LogToFile("ShortcutDialogProc: 初始化快捷方式对话框");
            // 初始化对话框控件
            RefreshShortcutList(GetDlgItem(hwnd, IDC_SHORTCUT_LIST));
            return TRUE;
            
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    LogToFile("ShortcutDialogProc: 用户点击确定按钮");
                    EndDialog(hwnd, IDOK);
                    return TRUE;
                    
                case IDCANCEL:
                    LogToFile("ShortcutDialogProc: 用户点击取消按钮");
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
                    
                case IDC_SHORTCUT_ADD:
                    LogToFile("ShortcutDialogProc: 用户点击添加快捷方式按钮");
                    AddShortcutFromDialog(hwnd);
                    return TRUE;
                    
                case IDC_SHORTCUT_UPDATE:
                    LogToFile("ShortcutDialogProc: 用户点击更新快捷方式按钮");
                    UpdateShortcutFromDialog(hwnd);
                    return TRUE;
                    
                case IDC_SHORTCUT_DELETE:
                    LogToFile("ShortcutDialogProc: 用户点击删除快捷方式按钮");
                    DeleteShortcutFromDialog(hwnd);
                    return TRUE;
            }
            break;
            
        case WM_CLOSE:
            LogToFile("ShortcutDialogProc: 用户关闭对话框");
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
    }
    
    return FALSE;
}

// Show shortcut dialog
void ShowShortcutDialog()
{
    LogToFile("ShowShortcutDialog: 显示快捷方式对话框");
    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_SHORTCUT_DIALOG), g_hMainWindow, ShortcutDialogProc);
}

// Refresh shortcut list
void RefreshShortcutList(HWND hList)
{
    LogToFile("RefreshShortcutList: 刷新快捷方式列表");
    
    // 清空列表
    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
    
    // 添加所有快捷方式项
    for (size_t i = 0; i < g_shortcuts.size(); i++)
    {
        WCHAR itemText[512] = {0};
        wsprintfW(itemText, L"%s - %s", g_shortcuts[i].name, g_shortcuts[i].path);
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)itemText);
    }
    
    LogToFile("RefreshShortcutList: 快捷方式列表刷新完成");
}

// Add shortcut from dialog
void AddShortcutFromDialog(HWND hDlg)
{
    LogToFile("AddShortcutFromDialog: 从对话框添加快捷方式");
    
    // 获取输入的名称和路径
    WCHAR name[256] = {0};
    WCHAR path[512] = {0};
    
    GetDlgItemTextW(hDlg, IDC_SHORTCUT_NAME, name, sizeof(name)/sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_SHORTCUT_PATH, path, sizeof(path)/sizeof(WCHAR));
    
    // 检查输入是否为空
    if (wcslen(name) == 0 || wcslen(path) == 0)
    {
        LogToFile("AddShortcutFromDialog: 名称或路径为空");
        MessageBoxW(hDlg, TTR(L"名称和路径不能为空").c_str(), TTR(L"输入错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 创建快捷方式项
    ShortcutItem shortcut = {0};
    wcscpy(shortcut.name, name);
    wcscpy(shortcut.path, path);
    
    // 确定类型
    if (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY)
    {
        shortcut.type = 0; // 目录
    }
    else if (wcsstr(path, L"://") != NULL || wcsstr(path, L"www.") != NULL)
    {
        shortcut.type = 1; // URL
    }
    else
    {
        shortcut.type = 2; // 应用程序
    }
    
    shortcut.usageCount = 0;
    
    // 添加到快捷方式列表
    g_shortcuts.push_back(shortcut);
    
    // 保存快捷方式
    SaveShortcuts();
    
    // 刷新列表
    RefreshShortcutList(GetDlgItem(hDlg, IDC_SHORTCUT_LIST));
    
    // 清空输入框
    SetDlgItemTextW(hDlg, IDC_SHORTCUT_NAME, L"");
    SetDlgItemTextW(hDlg, IDC_SHORTCUT_PATH, L"");
    
    LogToFile("AddShortcutFromDialog: 快捷方式添加完成");
}

// Update shortcut from dialog
void UpdateShortcutFromDialog(HWND hDlg)
{
    LogToFile("UpdateShortcutFromDialog: 从对话框更新快捷方式");
    
    // 获取选中的项目索引
    HWND hList = GetDlgItem(hDlg, IDC_SHORTCUT_LIST);
    int selectedIndex = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    
    if (selectedIndex == LB_ERR)
    {
        LogToFile("UpdateShortcutFromDialog: 未选择任何项目");
        MessageBoxW(hDlg, TTR(L"请选择要更新的快捷方式").c_str(), TTR(L"操作错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 获取输入的名称和路径
    WCHAR name[256] = {0};
    WCHAR path[512] = {0};
    
    GetDlgItemTextW(hDlg, IDC_SHORTCUT_NAME, name, sizeof(name)/sizeof(WCHAR));
    GetDlgItemTextW(hDlg, IDC_SHORTCUT_PATH, path, sizeof(path)/sizeof(WCHAR));
    
    // 检查输入是否为空
    if (wcslen(name) == 0 || wcslen(path) == 0)
    {
        LogToFile("UpdateShortcutFromDialog: 名称或路径为空");
        MessageBoxW(hDlg, TTR(L"名称和路径不能为空").c_str(), TTR(L"输入错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 更新快捷方式
    if (selectedIndex < static_cast<int>(g_shortcuts.size()))
    {
        wcscpy(g_shortcuts[selectedIndex].name, name);
        wcscpy(g_shortcuts[selectedIndex].path, path);
        
        // 确定类型
        if (GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY)
        {
            g_shortcuts[selectedIndex].type = 0; // 目录
        }
        else if (wcsstr(path, L"://") != NULL || wcsstr(path, L"www.") != NULL)
        {
            g_shortcuts[selectedIndex].type = 1; // URL
        }
        else
        {
            g_shortcuts[selectedIndex].type = 2; // 应用程序
        }
        
        // 保存快捷方式
        SaveShortcuts();
        
        // 刷新列表
        RefreshShortcutList(hList);
        
        // 清空输入框
        SetDlgItemTextW(hDlg, IDC_SHORTCUT_NAME, L"");
        SetDlgItemTextW(hDlg, IDC_SHORTCUT_PATH, L"");
        
        LogToFile("UpdateShortcutFromDialog: 快捷方式更新完成");
    }
}

// Delete shortcut from dialog
void DeleteShortcutFromDialog(HWND hDlg)
{
    LogToFile("DeleteShortcutFromDialog: 从对话框删除快捷方式");
    
    // 获取选中的项目索引
    HWND hList = GetDlgItem(hDlg, IDC_SHORTCUT_LIST);
    int selectedIndex = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
    
    if (selectedIndex == LB_ERR)
    {
        LogToFile("DeleteShortcutFromDialog: 未选择任何项目");
        MessageBoxW(hDlg, TTR(L"请选择要删除的快捷方式").c_str(), TTR(L"操作错误").c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    
    // 确认删除
    if (MessageBoxW(hDlg, TTR(L"确定要删除选中的快捷方式吗？").c_str(), TTR(L"确认删除").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        // 从快捷方式列表中删除
        if (selectedIndex < static_cast<int>(g_shortcuts.size()))
        {
            g_shortcuts.erase(g_shortcuts.begin() + selectedIndex);
            
            // 保存快捷方式
            SaveShortcuts();
            
            // 刷新列表
            RefreshShortcutList(hList);
            
            // 清空输入框
            SetDlgItemTextW(hDlg, IDC_SHORTCUT_NAME, L"");
            SetDlgItemTextW(hDlg, IDC_SHORTCUT_PATH, L"");
            
            LogToFile("DeleteShortcutFromDialog: 快捷方式删除成功");
        }
    }
}

// Save shortcuts to file
void SaveShortcuts()
{
    LogToFile("SaveShortcuts: 开始保存快捷方式");
    
    // 创建数据目录（如果不存在）
    CreateDirectoryW(L"data", NULL);
    
    // 打开快捷方式文件
    FILE* file = _wfopen(L"data\\shortcuts.txt", L"w, ccs=UTF-8");
    if (file)
    {
        // 写入每个快捷方式项
        for (const auto& shortcut : g_shortcuts)
        {
            fwprintf(file, L"%s|%s|%d|%d\n", 
                    shortcut.name, 
                    shortcut.path, 
                    shortcut.type, 
                    shortcut.usageCount);
        }
        
        fclose(file);
        LogToFile("SaveShortcuts: 快捷方式保存成功");
    }
    else
    {
        LogToFile("SaveShortcuts: 无法打开快捷方式文件进行写入");
    }
}

// Load shortcuts from file
void LoadShortcuts()
{
    LogToFile("LoadShortcuts: 开始加载快捷方式");
    
    // 打开快捷方式文件
    FILE* file = _wfopen(L"data\\shortcuts.txt", L"r, ccs=UTF-8");
    if (file)
    {
        // 清空当前快捷方式列表
        g_shortcuts.clear();
        
        // 读取每一行
        WCHAR line[2048] = {0};
        while (fgetws(line, sizeof(line)/sizeof(WCHAR), file))
        {
            // 移除换行符（如果存在）
            size_t len = wcslen(line);
            if (len > 0 && line[len-1] == L'\n')
            {
                line[len-1] = L'\0';
            }
            
            // 分割字段
            WCHAR* name = line;
            WCHAR* path = wcschr(line, L'|');
            if (path)
            {
                *path = L'\0';
                path++;
                
                WCHAR* typeStr = wcschr(path, L'|');
                if (typeStr)
                {
                    *typeStr = L'\0';
                    typeStr++;
                    
                    WCHAR* usageCountStr = wcschr(typeStr, L'|');
                    if (usageCountStr)
                    {
                        *usageCountStr = L'\0';
                        usageCountStr++;
                        
                        // 创建快捷方式项
                        ShortcutItem shortcut = {0};
                        wcscpy(shortcut.name, name);
                        wcscpy(shortcut.path, path);
                        shortcut.type = _wtoi(typeStr);
                        shortcut.usageCount = _wtoi(usageCountStr);
                        
                        // 添加到快捷方式列表
                        g_shortcuts.push_back(shortcut);
                    }
                }
            }
        }
        
        fclose(file);
        LogToFile("LoadShortcuts: 快捷方式加载成功");
    }
    else
    {
        LogToFile("LoadShortcuts: 无法打开快捷方式文件进行读取");
    }
}

// Show settings dropdown menu
void ShowSettingsDropdownMenu()
{
    LogToFile("ShowSettingsDropdownMenu: 显示设置下拉菜单");
    
    // 创建设置菜单
    HMENU hMenu = CreateSettingsMenu();
    if (hMenu)
    {
        // 获取设置按钮的位置
        RECT rect;
        GetWindowRect(g_hSettingsButton, &rect);
        
        // 显示菜单
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, rect.left, rect.bottom, 0, g_hMainWindow, NULL);
        
        // 销毁菜单
        DestroyMenu(hMenu);
        
        LogToFile("ShowSettingsDropdownMenu: 设置下拉菜单显示完成");
    }
}

// Create settings menu
HMENU CreateSettingsMenu()
{
    LogToFile("CreateSettingsMenu: 创建设置菜单");
    
    // 创建弹出菜单
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        // 添加菜单项
        AppendMenuW(hMenu, MF_STRING, ID_SETTINGS_HELP, TTR(L"帮助").c_str());
        
        LogToFile("CreateSettingsMenu: 设置菜单创建完成");
    }
    else
    {
        LogToFile("CreateSettingsMenu: 创建设置菜单失败");
    }
    
    return hMenu;
}

// Handle settings menu command
void HandleSettingsMenuCommand(WPARAM wParam)
{
    LogToFile("HandleSettingsMenuCommand: 处理设置菜单命令");
    
    switch (LOWORD(wParam))
    {
        case ID_SETTINGS_HELP:
            LogToFile("HandleSettingsMenuCommand: 用户选择帮助菜单项");
            ShowHelpDialog();
            break;
    }
}

// Show help dialog
void ShowHelpDialog()
{
    LogToFile("ShowHelpDialog: 显示帮助对话框");
    
    // 创建帮助文本
    WCHAR helpText[2048] = {0};
    wsprintfW(helpText, 
        L"快捷启动器帮助\n\n"
        L"使用说明:\n"
        L"1. 在输入框中输入命令或文件名\n"
        L"2. 按回车键执行\n"
        L"3. 支持的命令:\n"
        L"   - js: 进入计算模式\n"
        L"   - wz: 进入网址收藏模式\n"
        L"   - dir: 进入目录浏览模式\n"
        L"   - q: 退出当前模式\n"
        L"4. 在计算模式下可进行数学运算\n"
        L"5. 在网址收藏模式下可管理网址\n"
        L"6. 在目录浏览模式下可查看文件夹内容\n\n"
        L"快捷键:\n"
        L"Ctrl+Alt+Q: 显示/隐藏窗口\n"
        L"Ctrl+F1: 将窗口定位到桌面可见位置\n"
        L"Ctrl+F2: 显示窗口\n\n"
        L"当前模式: %s",
        GetCurrentMode().c_str());
    
    // 显示帮助信息
    MessageBoxW(g_hMainWindow, helpText, TTR(L"帮助").c_str(), MB_OK | MB_ICONINFORMATION);
    
    LogToFile("ShowHelpDialog: 帮助对话框显示完成");
}

// Get current mode
std::wstring GetCurrentMode()
{
    if (g_calculatorMode)
    {
        return TTR(L"计算模式");
    }
    else if (g_bookmarkMode)
    {
        return TTR(L"网址收藏模式");
    }
    else if (g_dirMode)
    {
        return TTR(L"目录浏览模式");
    }
    else
    {
        return TTR(L"普通模式");
    }
}

// Show launcher window
void ShowLauncherWindow()
{
    LogToFile("ShowLauncherWindow: 显示启动器窗口");
    
    // 检查窗口是否已经创建
    if (!g_hMainWindow)
    {
        LogToFile("ShowLauncherWindow: 窗口句柄为空，无法显示窗口");
        return;
    }
    
    // 确保窗口显示并获得焦点
    ShowWindow(g_hMainWindow, SW_SHOW);
    SetWindowPos(g_hMainWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(g_hMainWindow);
    SetActiveWindow(g_hMainWindow);
    SetFocus(g_hMainWindow);
    
    // 设置焦点到编辑框
    if (g_hEdit)
    {
        SetFocus(g_hEdit);
    }
    
    LogToFile("ShowLauncherWindow: 窗口已显示并获得焦点");
}

// Add tray icon
void AddTrayIcon()
{
    LogToFile("AddTrayIcon: 添加系统托盘图标");
    
    // 初始化NOTIFYICONDATA结构
    g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    g_notifyIconData.hWnd = g_hMainWindow;
    g_notifyIconData.uID = 1;
    g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;
    g_notifyIconData.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    
    // 设置提示文本
    // 根据Windows SDK版本，szTip可能是CHAR或WCHAR数组
    // 使用strncpy_s确保兼容性
#ifdef UNICODE
    wcsncpy_s(g_notifyIconData.szTip, ARRAYSIZE(g_notifyIconData.szTip), L"快捷启动器", _TRUNCATE);
#else
    strncpy_s(g_notifyIconData.szTip, ARRAYSIZE(g_notifyIconData.szTip), "快捷启动器", _TRUNCATE);
#endif
    
    // 添加托盘图标
    Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
    g_trayIconAdded = true;
    
    LogToFile("AddTrayIcon: 系统托盘图标添加完成");
}

// Remove tray icon
void RemoveTrayIcon()
{
    LogToFile("RemoveTrayIcon: 移除系统托盘图标");
    
    if (g_trayIconAdded)
    {
        Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
        g_trayIconAdded = false;
        
        LogToFile("RemoveTrayIcon: 系统托盘图标移除完成");
    }
}

// Create tray menu
void CreateTrayMenu()
{
    LogToFile("CreateTrayMenu: 创建托盘右键菜单");
    
    // 如果菜单已存在，先销毁
    if (g_trayMenu)
    {
        DestroyMenu(g_trayMenu);
    }
    
    // 创建弹出菜单
    g_trayMenu = CreatePopupMenu();
    if (g_trayMenu)
    {
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_SHOW, BTN_SHOW_WINDOW);
        AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(g_trayMenu, MF_STRING, ID_TRAY_EXIT, BTN_EXIT);
        LogToFile("CreateTrayMenu: 成功创建托盘右键菜单");
    }
    else
    {
        LogToFile("CreateTrayMenu: 创建托盘右键菜单失败");
    }
}

// Handle tray message
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

// Edit control subclassing procedure implementation
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                // 检查是否需要忽略这个回车键（由焦点变化触发）
                if (g_ignoreNextReturn)
                {
                    LogToFile("EditSubclassProc: WM_KEYDOWN with VK_RETURN received but ignored (focus change)");
                    g_ignoreNextReturn = false; // 重置标志
                    return 0; // 消息已处理，不继续执行
                }
                
                LogToFile("EditSubclassProc: WM_KEYDOWN with VK_RETURN received");
                
                // Get current text from edit control
                WCHAR currentText[1024] = {0};
                GetWindowTextW(hwnd, currentText, sizeof(currentText)/sizeof(WCHAR));
                char currentTextLog[1024] = {0};
                WideCharToMultiByte(CP_UTF8, 0, currentText, -1, currentTextLog, sizeof(currentTextLog), NULL, NULL);
                char enterLog[1100] = {0};
                sprintf(enterLog, "  EditSubclassProc: Current edit text: '%s'", currentTextLog);
                LogToFile(enterLog);
                
                // 检查是否在dir模式下
                if (g_dirMode)
                {
                    LogToFile("  EditSubclassProc: 处于dir模式，处理目录路径输入");
                    // 检查是否输入了退出命令
                    if (wcscmp(currentText, L"q") == 0)
                    {
                        ExitDirMode();
                        return 0;
                    }
                    // 处理用户输入的目录路径
                    if (wcslen(currentText) > 0)
                    {
                        ListDirectoryContents(currentText);
                    }
                    else
                    {
                        SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
                        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"Invalid directory path");
                        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"");
                        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"输入 'q' 退出该模式");
                    }
                    return 0;
                }
                
                // 特殊处理"dir"命令 - 如果输入的是"dir"，进入dir模式
                if (wcscmp(currentText, L"dir") == 0)
                {
                    LogToFile("  EditSubclassProc: 检测到'dir'命令，进入dir模式");
                    EnterDirMode();
                    return 0; // 消息已处理，不继续执行
                }
                
                // 特殊处理"js"命令 - 如果输入的是"js"，进入计算器模式
                if (wcscmp(currentText, L"js") == 0)
                {
                    LogToFile("  EditSubclassProc: 检测到'js'命令，进入计算器模式");
                    EnterCalculatorMode();
                    // Clear the edit box
                    SetWindowTextW(hwnd, L"");
                    return 0; // 消息已处理，不继续执行
                }
                
                // 特殊处理"wz"命令 - 如果输入的是"wz"，进入网址收藏模式
                if (wcscmp(currentText, L"wz") == 0)
                {
                    LogToFile("  EditSubclassProc: 检测到'wz'命令，进入网址收藏模式");
                    EnterBookmarkMode();
                    // Clear the edit box
                    SetWindowTextW(hwnd, L"");
                    return 0; // 消息已处理，不继续执行
                }
                
                // 检查是否在计算器模式下
                if (g_calculatorMode)
                {
                    LogToFile("  EditSubclassProc: 处于计算器模式，调用EvaluateExpression");
                    // 检查是否输入了退出命令
                    if (wcscmp(currentText, L"q") == 0)
                    {
                        ExitCalculatorMode();
                        return 0;
                    }
                    // 处理用户输入的表达式，调用EvaluateExpression函数
                    if (wcslen(currentText) > 0)
                    {
                        EvaluateExpression(currentText);
                        // 清空编辑框
                        SetWindowTextW(hwnd, L"");
                    }
                    return 0;
                }
                
                // Handle return key - Only execute if user explicitly presses Enter
                {
                    // 注意：g_hListBox是ListView控件，不是ListBox控件
                    // 在非特殊模式下，处理快捷方式的执行
                    if (!g_calculatorMode && !g_bookmarkMode && !g_dirMode) {
                        int itemCount = ListView_GetItemCount(g_hListBox);
                        if (itemCount > 0) {
                            // Select and execute the first item if no item is selected
                            int selectedIndex = ListView_GetNextItem(g_hListBox, -1, LVNI_SELECTED);
                            if (selectedIndex == -1) {
                                // No item selected, select the first one
                                ListView_SetItemState(g_hListBox, 0, LVIS_SELECTED, LVIS_SELECTED);
                                selectedIndex = 0;
                            }
                            ExecuteSelectedItem(selectedIndex);
                        }
                    }
                    return 0; // Message handled, don't continue execution
                }
            }
            break;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// Search shortcuts and display results
void SearchShortcuts(const WCHAR* query)
{
    // Clear the search results
    g_searchResults.clear();
    
    // Clear the listview
    ListView_DeleteAllItems(g_hListBox);
    
    // If query is empty, show all shortcuts
    if (wcslen(query) == 0)
    {
        g_searchResults = g_shortcuts;
    }
    else
    {
        // Search for matching shortcuts
        for (size_t i = 0; i < g_shortcuts.size(); i++)
        {
            // Convert both strings to lowercase for comparison
            WCHAR lowerName[256];
            wcscpy_s(lowerName, g_shortcuts[i].name);
            
            WCHAR lowerQuery[1024];
            wcscpy_s(lowerQuery, query);
            
            // Convert to lowercase
            for (int j = 0; lowerName[j]; j++)
                lowerName[j] = towlower(lowerName[j]);
            for (int j = 0; lowerQuery[j]; j++)
                lowerQuery[j] = towlower(lowerQuery[j]);
            
            // Check if query is contained in the name
            if (wcsstr(lowerName, lowerQuery) != NULL)
            {
                g_searchResults.push_back(g_shortcuts[i]);
            }
        }
    }
    
    // Display the search results in the listview
    for (size_t i = 0; i < g_searchResults.size(); i++)
    {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = g_searchResults[i].name;
        ListView_InsertItem(g_hListBox, &lvi);
    }
    
    // Select the first item if there are any results
    if (!g_searchResults.empty())
    {
        ListView_SetItemState(g_hListBox, 0, LVIS_SELECTED, LVIS_SELECTED);
    }
}

// Window procedure function
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Declare lvc variable here to avoid initialization skip issues
    LVCOLUMNW lvc;
    
    switch (uMsg)
    {
        case WM_CREATE:
            // Initialize controls and UI elements here
            // Create the edit control
            g_hEdit = CreateWindowW(L"EDIT", L"", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 10, 380, 25, hwnd, (HMENU)IDC_EDIT, g_hInstance, NULL);
            
            // Subclass the edit control to handle special key events
            SetWindowSubclass(g_hEdit, EditSubclassProc, 0, 0);
            
            // Create the listview control
            g_hListBox = CreateWindowW(WC_LISTVIEWW, L"", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                10, 45, 380, 200, hwnd, (HMENU)IDC_LISTBOX, g_hInstance, NULL);
            
            // Set extended listview styles
            ListView_SetExtendedListViewStyle(g_hListBox, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            
            // 确保ListView支持Unicode
            SendMessageW(g_hListBox, CCM_SETUNICODEFORMAT, TRUE, 0);
            
            // Add columns to the listview
            lvc = {0};
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 200;  // Width of the first column
            lvc.pszText = L"表达式";
            lvc.iSubItem = 0;
            ListView_InsertColumn(g_hListBox, 0, &lvc);
            
            lvc = {0};  // Reinitialize for second column
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 170;  // Width of the second column
            lvc.pszText = L"注释";
            lvc.iSubItem = 1;
            ListView_InsertColumn(g_hListBox, 1, &lvc);
            
            // Apply font to the listview
            ApplyFontToControl(g_hListBox);
            
            // 确保ListView使用Unicode字体设置
            if (g_hFont) {
                SendMessageW(g_hListBox, WM_SETFONT, (WPARAM)g_hFont, MAKELPARAM(TRUE, 0));
            }
            
            // Create the settings button
            g_hSettingsButton = CreateWindowW(L"BUTTON", L"设置", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 255, 80, 30, hwnd, (HMENU)IDC_SETTINGS_BUTTON, g_hInstance, NULL);
            
            // Create the exit calc button
            g_hExitCalcButton = CreateWindowW(L"BUTTON", L"退出计算模式", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                100, 255, 100, 30, hwnd, (HMENU)IDC_EXIT_CALC_BUTTON, g_hInstance, NULL);
            
            // Create the exit bookmark button
            g_hExitBookmarkButton = CreateWindowW(L"BUTTON", L"退出网址模式", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                210, 255, 100, 30, hwnd, (HMENU)IDC_EXIT_BOOKMARK_BUTTON, g_hInstance, NULL);
            
            // Create the exit dir mode button
            g_hExitDirModeButton = CreateWindowW(L"BUTTON", L"退出目录模式", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                320, 255, 100, 30, hwnd, (HMENU)IDC_EXIT_DIR_MODE_BUTTON, g_hInstance, NULL);
            
            // Load shortcuts
            LoadShortcuts();
            
            // Show all shortcuts in the listbox
            SearchShortcuts(L"");
            
            // Register hotkeys
            RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL | MOD_ALT, 'Q');  // Ctrl+Alt+Q
            RegisterHotKey(hwnd, HOTKEY_ID_CTRL_F1, MOD_CONTROL, VK_F1);  // Ctrl+F1
            RegisterHotKey(hwnd, HOTKEY_ID_CTRL_F2, MOD_CONTROL, VK_F2);  // Ctrl+F2
            break;
        
        case WM_HOTKEY:
            switch (wParam)
            {
                case HOTKEY_ID:  // Ctrl+Alt+Q
                    {
                        // Toggle window visibility
                        if (IsWindowVisible(g_hMainWindow))
                        {
                            ShowWindow(g_hMainWindow, SW_HIDE);
                        }
                        else
                        {
                            ShowWindow(g_hMainWindow, SW_SHOW);
                            SetForegroundWindow(g_hMainWindow);
                        }
                    }
                    break;
                case HOTKEY_ID_CTRL_F1:  // Ctrl+F1
                    {
                        // Position window to visible area
                        RECT rect;
                        GetWindowRect(g_hMainWindow, &rect);
                        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                        
                        // Ensure window is within screen bounds
                        int x = max(0, min(rect.left, screenWidth - (rect.right - rect.left)));
                        int y = max(0, min(rect.top, screenHeight - (rect.bottom - rect.top)));
                        
                        SetWindowPos(g_hMainWindow, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                        ShowWindow(g_hMainWindow, SW_SHOW);
                        SetForegroundWindow(g_hMainWindow);
                    }
                    break;
                case HOTKEY_ID_CTRL_F2:  // Ctrl+F2
                    {
                        // Show window
                        ShowWindow(g_hMainWindow, SW_SHOW);
                        SetForegroundWindow(g_hMainWindow);
                    }
                    break;
            }
            break;
        
        case WM_DESTROY:
            // Unregister hotkeys
            UnregisterHotKey(hwnd, HOTKEY_ID);
            UnregisterHotKey(hwnd, HOTKEY_ID_CTRL_F1);
            UnregisterHotKey(hwnd, HOTKEY_ID_CTRL_F2);
            
            // 释放控制台
            FreeConsole();
            
            PostQuitMessage(0);
            break;
            
        case WM_COMMAND:
            // Handle control notifications and menu commands
            switch (LOWORD(wParam))
            {
                case IDC_EDIT:
                    // Handle edit control notifications
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        // Get the text from the edit control
                        WCHAR text[1024] = {0};
                        GetWindowTextW(g_hEdit, text, sizeof(text)/sizeof(WCHAR));
                        
                        // If not in a special mode, search shortcuts
                        if (!g_calculatorMode && !g_bookmarkMode && !g_dirMode)
                        {
                            SearchShortcuts(text);
                        }
                    }
                    break;
                case IDC_LISTBOX:
                    // Handle listbox notifications
                    break;
                case IDC_SETTINGS_BUTTON:
                    // Handle settings button click
                    break;
                case IDC_EXIT_CALC_BUTTON:
                    // Handle exit calc button click
                    if (g_calculatorMode)
                    {
                        ExitCalculatorMode();
                    }
                    break;
                case IDC_EXIT_BOOKMARK_BUTTON:
                    // Handle exit bookmark button click
                    if (g_bookmarkMode)
                    {
                        ExitBookmarkMode();
                    }
                    break;
                case IDC_EXIT_DIR_MODE_BUTTON:
                    // Handle exit dir mode button click
                    if (g_dirMode)
                    {
                        ExitDirMode();
                    }
                    break;
            }
            break;
            
        case WM_SIZE:
            // Handle window resizing
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// WinMain function - Entry point for Windows GUI application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Allocate console for debug output
    AllocConsole();
    
    // Redirect stdout to the console
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    
    // Set UTF-8 code page for console output
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // 设置控制台标题
    SetConsoleTitleA("Quick Launcher Debug Console");

    // Store instance handle
    g_hInstance = hInstance;

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    // Create UI font
    CreateUIFont();

    // Create main window
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wcex.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"QuickLauncherClass";
    wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));

    if (!RegisterClassExW(&wcex))
    {
        MessageBoxW(NULL, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Calculate window size to accommodate client area
    RECT rect = {0, 0, 600, 400};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create main window
    g_hMainWindow = CreateWindowExW(
        0,
        L"QuickLauncherClass",
        L"Quick Launcher",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!g_hMainWindow)
    {
        MessageBoxW(NULL, L"Failed to create main window", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Show and update window
    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    // Load calculation history and bookmarks
    LoadCalculationHistory();
    LoadBookmarks();
    LoadShortcuts();

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

