#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>
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
// Flag to ignore EN_RETURN notifications triggered by focus changes
bool g_ignoreNextReturn = false;

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
#define HOTKEY_ID 1
#define HOTKEY_ID_CTRL_F1 2
#define HOTKEY_ID_CTRL_F2 3

// 系统托盘相关常量
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_SHOW 1005
#define ID_TRAY_EXIT 1006

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
HWND g_hModeLabel = NULL;  // 模式标签控件

// 表达式解析辅助函数声明
void EnterCalculatorMode();
void ExitCalculatorMode();
void EvaluateExpression(const WCHAR* expression);
void DisplayCalculationHistory();
void SaveCalculationHistory();
void LoadCalculationHistory();

// 表达式解析辅助函数声明
double parseNumber(const std::wstring& expr, size_t& pos);
double parseTerm(const std::wstring& expr, size_t& pos);
double parseExpression(const std::wstring& expr, size_t& pos);

// Log function
void LogToFile(const char* message)
{
    // Create log directory if it doesn't exist
    CreateDirectoryW(L"log", NULL);
    
    // Generate unique log filename based on current date and time
    static WCHAR logFileName[MAX_PATH] = {0};
    static bool fileNameGenerated = false;
    static bool bomWritten = false;
    
    if (!fileNameGenerated) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        // Format: log\quick_launcher_YYYYMMDD_HHMMSS.log
        wsprintfW(logFileName, L"log\\quick_launcher_%04d%02d%02d_%02d%02d%02d.log",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
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
void ExecuteSelectedItem(int index);
void ProcessCommand(const WCHAR* command);
void InitializeCommonShortcuts();
void SearchAndDisplayResults(const WCHAR* query);
void ShowLauncherWindow();
void HideLauncherWindow();
void AddDesktopShortcuts();
void SetEnglishInputMethod();

// 系统托盘相关函数声明
void AddTrayIcon();
void RemoveTrayIcon();
void CreateTrayMenu();
void HandleTrayMessage(LPARAM lParam);

// Show launcher window
void ShowLauncherWindow()
{
    LogToFile("ShowLauncherWindow called - START");
    
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
    
    // 检查是否是"js"命令，用于进入计算模式
    if (wcscmp(command, L"js") == 0)
    {
        LogToFile("ProcessCommand: 识别为'js'命令，进入计算模式");
        EnterCalculatorMode();
        return;
    }
    
    // Clear previous results
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    
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
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
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
        sprintf(finalLog, "ProcessCommand: 打开URL '%s', ShellExecuteW返回值: %ld", urlLog, (long)result);
        LogToFile(finalLog);
        
        if ((long)result > 32)
        {
            wsprintfW(feedback, L"Success! URL opened in default browser");
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            LogToFile("ProcessCommand: URL打开成功");
        }
        else
        {
            wsprintfW(feedback, L"Failed to open URL: error code %ld", (long)result);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            sprintf(finalLog, "ProcessCommand: URL打开失败，错误代码 %ld", (long)result);
            LogToFile(finalLog);
        }
    }
    // Handle file paths
    else if (GetFileAttributesW(command) != INVALID_FILE_ATTRIBUTES)
    {
        LogToFile("ProcessCommand: 识别为文件路径");
        WCHAR feedback[1024] = {0};
        wsprintfW(feedback, L"Opening file: %s", command);
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
        
        HINSTANCE result = ShellExecuteW(NULL, L"open", command, NULL, NULL, SW_SHOWNORMAL);
        
        sprintf(logMsg, "ProcessCommand: 打开文件 '%s', ShellExecuteW返回值: %ld", commandLog, (long)result);
        LogToFile(logMsg);
        
        if ((long)result > 32)
        {
            wsprintfW(feedback, L"File opened successfully");
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            LogToFile("ProcessCommand: 文件打开成功");
        }
        else
        {
            wsprintfW(feedback, L"Failed to open file: error code %ld", (long)result);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
            sprintf(logMsg, "ProcessCommand: 文件打开失败，错误代码 %ld", (long)result);
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
                ExecuteSelectedItem((int)i);
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            sprintf(logMsg, "ProcessCommand: 未找到匹配的命令或文件 '%s'", commandLog);
            LogToFile(logMsg);
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"No matching command or file found");
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
    
    // 检查是否要进入计算模式
    if (query && _wcsicmp(query, L"js") == 0)
    {
        LogToFile("SearchAndDisplayResults: 检测到'js'命令，直接调用ProcessCommand");
        
        SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
        g_searchResults.clear();
        
        // 直接调用ProcessCommand处理"js"命令
        ProcessCommand(query);
        
        return;
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
        
        SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
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
            
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)display);
            
            // 记录添加到列表的项目
            char itemNameLog[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, sorted[i].name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
            sprintf(logMsg, "SearchAndDisplayResults: 添加项目 '%s' (类型: %d, 使用次数: %d)", 
                    itemNameLog, sorted[i].type, sorted[i].usageCount);
            LogToFile(logMsg);
        }
        return;
    }
    
    // Clear previous results
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    g_searchResults.clear();
    
    sprintf(logMsg, "SearchAndDisplayResults: 在 %zu 个快捷方式中搜索匹配项", g_shortcuts.size());
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
            sprintf(logMsg, "SearchAndDisplayResults: 找到精确匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
            
            WCHAR display[1024] = {0};
            if (g_shortcuts[i].type == 0) // Directory
                wsprintfW(display, L"DIR: %s", g_shortcuts[i].name);
            else if (g_shortcuts[i].type == 1) // URL
                wsprintfW(display, L"URL: %s", g_shortcuts[i].name);
            else // Application
                wsprintfW(display, L"APP: %s", g_shortcuts[i].name);
            
            SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)display);
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
                sprintf(logMsg, "SearchAndDisplayResults: 找到前缀匹配 '%s'", itemNameLog);
                LogToFile(logMsg);
                
                WCHAR display[1024] = {0};
                if (g_shortcuts[i].type == 0) // Directory
                    wsprintfW(display, L"DIR: %s", g_shortcuts[i].name);
                else if (g_shortcuts[i].type == 1) // URL
                    wsprintfW(display, L"URL: %s", g_shortcuts[i].name);
                else // Application
                    wsprintfW(display, L"APP: %s", g_shortcuts[i].name);
                
                SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)display);
                g_searchResults.push_back(g_shortcuts[i]);
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
                        
                        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)display);
                        g_searchResults.push_back(g_shortcuts[i]);
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
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)L"No matching items found");
    }
    else
    {
        sprintf(logMsg, "SearchAndDisplayResults: 找到 %zu 个匹配项", g_searchResults.size());
        LogToFile(logMsg);
    }
}

// Execute selected item from list
void ExecuteSelectedItem(int index)
{
    if (index < 0 || (size_t)index >= g_searchResults.size())
    {
        char logMsg[200] = {0};
        sprintf(logMsg, "ExecuteSelectedItem: 无效索引 %d，搜索结果大小为 %zu", index, g_searchResults.size());
        LogToFile(logMsg);
        return;
    }
        
    ShortcutItem& item = g_searchResults[(size_t)index]; // Use reference to update usage count
    
    // 记录要执行的项目信息
    char itemNameLog[1024] = {0};
    char itemPathLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, item.name, -1, itemNameLog, sizeof(itemNameLog), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, item.path, -1, itemPathLog, sizeof(itemPathLog), NULL, NULL);
    
    char logMsg[1100] = {0};
    sprintf(logMsg, "ExecuteSelectedItem: 执行项目 '%s' (路径: '%s', 类型: %d, 使用次数: %d)", 
            itemNameLog, itemPathLog, item.type, item.usageCount);
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
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    else // Application
    {
        LogToFile("ExecuteSelectedItem: 执行应用程序");
        result = ShellExecuteW(NULL, L"open", item.path, NULL, NULL, SW_SHOWNORMAL);
    }
    
    // 记录执行结果
    sprintf(logMsg, "ExecuteSelectedItem: ShellExecuteW 返回值: %ld", (long)result);
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
    if ((long)result <= 32)
    {
        sprintf(logMsg, "ExecuteSelectedItem: 执行失败，错误代码 %ld", (long)result);
        LogToFile(logMsg);
        WCHAR feedback[1024] = {0};
        wsprintfW(feedback, L"Failed to execute: error code %ld", (long)result);
        SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)feedback);
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
            // 创建模式标签控件
            g_hModeLabel = CreateWindowExW(
                  0,
                  L"STATIC",
                  L"搜索:",
                  WS_CHILD | WS_VISIBLE | SS_LEFT,
                  10, 10, 50, 25,
                  hwnd, NULL,
                  g_hInstance, NULL);
            
            // Create edit control without ES_WANTRETURN to receive WM_KEYDOWN messages
            g_hEdit = CreateWindowExW(
                  0,
                  WC_EDITW,
                  L"",
                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                  65, 10, 230, 25,  // 减小宽度，为退出按钮腾出空间
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
            
            // Create listbox with LBS_NOTIFY but ensure only double click triggers execution
            g_hListBox = CreateWindowExW(
                  0,
                  WC_LISTBOXW,
                  L"",
                  WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | LBS_DISABLENOSCROLL,
                  10, 45, 370, 200,
                  hwnd, (HMENU)IDC_LISTBOX,
                  g_hInstance, NULL);
            
            // Create exit calculator mode button (initially hidden)
            g_hExitCalcButton = CreateWindowExW(
                  0,
                  L"BUTTON",
                  L"退出计算",
                  WS_CHILD | BS_PUSHBUTTON,
                  300, 10, 80, 25,
                  hwnd, (HMENU)IDC_EXIT_CALC_BUTTON,
                  g_hInstance, NULL);
            
            // Create settings button (initially visible in non-calculator mode)
            g_hSettingsButton = CreateWindowExW(
                  0,
                  L"BUTTON",
                  L"设置",
                  WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
                  300, 10, 80, 25,
                  hwnd, (HMENU)IDC_SETTINGS_BUTTON,
                  g_hInstance, NULL);
            
            // Initially hide the exit calculator button
            ShowWindow(g_hExitCalcButton, SW_HIDE);
            
            return 0;
            
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
                // 处理设置按钮点击
                else if (LOWORD(wParam) == IDC_SETTINGS_BUTTON)
                {
                    // 处理设置按钮点击
                    LogToFile("WM_COMMAND: 用户点击设置按钮");
                    // TODO: 实现设置功能
                    MessageBoxW(hwnd, L"设置功能正在开发中...", L"提示", MB_OK | MB_ICONINFORMATION);
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
                                // 在搜索模式下，进行正常搜索
                                SearchAndDisplayResults(searchText);
                            }
                            break;
                            
                        case EN_RETURN:
                            // Log edit control EN_RETURN notification
                            LogToFile("  Edit control EN_RETURN notification - processing");
                            
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
                                        int itemCount = SendMessageW(g_hListBox, LB_GETCOUNT, 0, 0);
                                        char logMsg[200] = {0};
                                        sprintf(logMsg, "  EN_RETURN: 列表框项目数量: %d", itemCount);
                                        LogToFile(logMsg);
                                    
                                        // If list has items, check if input is "js" command first
                        if (itemCount > 0)
                        {
                            // 检查输入内容是否是"js"命令
                            if (wcscmp(currentText, L"js") == 0)
                            {
                                LogToFile("  EN_RETURN: 识别为'js'命令，调用ProcessCommand");
                                ProcessCommand(currentText);
                            }
                            else
                            {
                                // Force select the first item to ensure it's highlighted
                                INT_PTR firstSelIndex = 0;
                                SendMessageW(g_hListBox, LB_SETCURSEL, firstSelIndex, 0);
                                LogToFile("  EN_RETURN: 强制选择第一个项目");
                                
                                // 获取第一个项目的文本
                                WCHAR firstItemText[1024] = {0};
                                SendMessageW(g_hListBox, LB_GETTEXT, firstSelIndex, (LPARAM)firstItemText);
                                char firstItemLog[1024] = {0};
                                WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                                sprintf(logMsg, "  EN_RETURN: 第一个项目文本: '%s'", firstItemLog);
                                LogToFile(logMsg);
                                
                                // Verify g_searchResults has items before executing
                                // Also check if the first item is not the "No matching items found" message
                                if (!g_searchResults.empty() && g_searchResults.size() > 0)
                                {
                                    LogToFile("  EN_RETURN: 搜索结果不为空，执行第一个项目");
                                    ExecuteSelectedItem((int)firstSelIndex);
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
                else if (LOWORD(wParam) == IDC_LISTBOX)
                {
                    // Only handle double click - explicitly ignore all other listbox notifications
                    // This prevents auto-opening on selection change or focus change
                    if (HIWORD(wParam) == LBN_DBLCLK)
                    {
                        INT_PTR selIndex = SendMessageW(g_hListBox, LB_GETCURSEL, 0, 0);
                        if (selIndex != LB_ERR)
                        {
                            ExecuteSelectedItem((int)selIndex);
                        }
                    }
                    // Explicitly ignore LBN_SELCHANGE to prevent auto-open on selection
                    // Also ignore LBN_SETFOCUS and other notifications
                }
                return 0;
            }
            
        case WM_KEYDOWN:
            // Handle Enter key press directly in edit control
            if (wParam == VK_RETURN && GetFocus() == g_hEdit)
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
                        int itemCount = SendMessageW(g_hListBox, LB_GETCOUNT, 0, 0);
                        char logMsg[200] = {0};
                        sprintf(logMsg, "  WM_KEYDOWN: 列表框项目数量: %d", itemCount);
                        LogToFile(logMsg);
                    
                        // If list has items, check if input is "js" command first
                        if (itemCount > 0)
                        {
                            // 检查输入内容是否是"js"命令
                            if (wcscmp(currentText, L"js") == 0)
                            {
                                LogToFile("  WM_KEYDOWN: 识别为'js'命令，调用ProcessCommand");
                                ProcessCommand(currentText);
                            }
                            else
                            {
                                // Force select the first item to ensure it's highlighted
                                INT_PTR firstSelIndex = 0;
                                SendMessageW(g_hListBox, LB_SETCURSEL, firstSelIndex, 0);
                                LogToFile("  WM_KEYDOWN: 强制选择第一个项目");
                                
                                // 获取第一个项目的文本
                                WCHAR firstItemText[1024] = {0};
                                SendMessageW(g_hListBox, LB_GETTEXT, firstSelIndex, (LPARAM)firstItemText);
                                char firstItemLog[1024] = {0};
                                WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                                sprintf(logMsg, "  WM_KEYDOWN: 第一个项目文本: '%s'", firstItemLog);
                                LogToFile(logMsg);
                                
                                // Verify g_searchResults has items before executing
                                // Also check if the first item is not the "No matching items found" message
                                if (!g_searchResults.empty() && g_searchResults.size() > 0)
                                {
                                    LogToFile("  WM_KEYDOWN: 搜索结果不为空，执行第一个项目");
                                    ExecuteSelectedItem((int)firstSelIndex);
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
            // For other key presses, fall through to default handler
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
    
    // Initialize common shortcuts
    InitializeCommonShortcuts();
    
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
    
    // Calculate window position to center it on the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 400;
    int windowHeight = 300;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    
    // Create window with proper style
    g_hMainWindow = CreateWindowExW(
        0,
        L"QuickLauncherClass",
        L"快速启动",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
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
    
    // Now set focus to edit box and display default shortcuts
    if (g_hEdit)
    {
        SetFocus(g_hEdit);
        LogToFile("Set focus to edit control after window creation");
    }
    
    if (g_hListBox)
    {
        SearchAndDisplayResults(L"");
        LogToFile("Displayed default search results after window creation");
    }
    
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
                
                // Handle return key - Ensure it executes the first item
                {
                    int itemCount = SendMessageW(g_hListBox, LB_GETCOUNT, 0, 0);
                    char logMsg[200] = {0};
                    sprintf(logMsg, "  EditSubclassProc: Listbox item count: %d", itemCount);
                    LogToFile(logMsg);
                
                    // If list has items, explicitly select and open the first one
                    if (itemCount > 0)
                    {
                        // 检查是否在计算模式
                        if (g_calculatorMode)
                        {
                            LogToFile("  EditSubclassProc: 计算模式下，忽略列表项，调用EvaluateExpression");
                            EvaluateExpression(currentText);
                        }
                        else
                        {
                            // Force select the first item to ensure it's highlighted
                            INT_PTR firstSelIndex = 0;
                            SendMessageW(g_hListBox, LB_SETCURSEL, firstSelIndex, 0);
                            LogToFile("  EditSubclassProc: Force selecting first item");
                            
                            // Get first item text
                            WCHAR firstItemText[1024] = {0};
                            SendMessageW(g_hListBox, LB_GETTEXT, firstSelIndex, (LPARAM)firstItemText);
                            char firstItemLog[1024] = {0};
                            WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                            sprintf(logMsg, "  EditSubclassProc: First item text: '%s'", firstItemLog);
                            LogToFile(logMsg);
                            
                            // Verify g_searchResults has items before executing
                            // Also check if the first item is not the "No matching items found" message
                            if (!g_searchResults.empty() && g_searchResults.size() > 0)
                            {
                                LogToFile("  EditSubclassProc: Search results not empty, executing first item");
                                ExecuteSelectedItem((int)firstSelIndex);
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
                                    LogToFile("  EditSubclassProc: Error: search results empty but listbox has actual items");
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
                            
                            // 检查是否在计算模式
                            if (g_calculatorMode)
                            {
                                LogToFile("  EditSubclassProc: 计算模式下，调用EvaluateExpression");
                                EvaluateExpression(currentText);
                            }
                            else
                            {
                                LogToFile("  EditSubclassProc: 非计算模式，调用ProcessCommand");
                                ProcessCommand(currentText);
                            }
                        }
                        else
                        {
                            LogToFile("  EditSubclassProc: List empty and input text empty, no action taken");
                        }
                    }
                }
                return 0; // Message handled, no further processing needed
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
    
    // 设置计算模式标志
    g_calculatorMode = true;
    
    // 更新模式标签文本
    SetWindowTextW(g_hModeLabel, L"计算:");
    
    // 显示退出计算模式按钮
    ShowWindow(g_hExitCalcButton, SW_SHOW);
    
    // 隐藏设置按钮
    ShowWindow(g_hSettingsButton, SW_HIDE);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 显示计算历史记录
    DisplayCalculationHistory();
    
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
    SetWindowTextW(g_hModeLabel, L"搜索:");
    
    // 隐藏退出计算模式按钮
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    
    // 显示设置按钮
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 清空列表框
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
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
            swprintf(resultStr, 256, L"%.6g", result);
            LogToFile("EvaluateExpression: 创建了结果字符串");
            
            // 创建历史记录条目（使用原始表达式，包括等号部分）
            std::wstring historyEntry = expression;
            historyEntry += L" = ";
            historyEntry += resultStr;
            LogToFile("EvaluateExpression: 创建了历史记录条目");
            
            // 添加到计算历史
            g_calculationHistory.push_back(historyEntry);
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
    LogToFile("DisplayCalculationHistory: 显示计算历史");
    
    // 暂停列表框重绘以提高性能
    SendMessageW(g_hListBox, WM_SETREDRAW, FALSE, 0);
    
    // 清空列表框
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    
    // 添加历史记录到列表框（最新的在顶部）
    for (auto it = g_calculationHistory.rbegin(); it != g_calculationHistory.rend(); ++it)
    {
        // 创建临时字符串以避免反向迭代器的指针问题
        std::wstring entry = *it;
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)entry.c_str());
    }
    
    // 恢复列表框重绘
    SendMessageW(g_hListBox, WM_SETREDRAW, TRUE, 0);
    
    // 强制重绘列表框
    InvalidateRect(g_hListBox, NULL, TRUE);
    
    // 记录显示的历史记录数量
    char logMsg[200] = {0};
    sprintf(logMsg, "DisplayCalculationHistory: 显示了 %zu 条历史记录", g_calculationHistory.size());
    LogToFile(logMsg);
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
    
    // 写入历史记录
    for (const auto& entry : g_calculationHistory)
    {
        // 直接写入宽字符字符串
        fwprintf(file, L"%s\n", entry.c_str());
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
            
            // 添加到历史记录
            g_calculationHistory.push_back(std::wstring(buffer));
            
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