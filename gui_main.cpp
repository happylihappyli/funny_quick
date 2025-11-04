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

// Define notification codes if not defined
#ifndef EN_RETURN
#define EN_RETURN 0x0100
#endif

// Global variables
HINSTANCE g_hInstance = NULL;
HWND g_hMainWindow = NULL;
HWND g_hEdit = NULL;
HWND g_hListBox = NULL;
// Flag to ignore EN_RETURN notifications triggered by focus changes
bool g_ignoreNextReturn = false;

// Subclassing procedure pointer for edit control
WNDPROC g_originalEditProc = NULL;

// System tray icon data
NOTIFYICONDATA g_trayIconData = {0};
bool g_trayIconAdded = false;

// Edit control subclassing procedure declaration
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// Constants
#define IDC_EDIT 1001
#define IDC_LISTBOX 1002
#define HOTKEY_ID 1
#define HOTKEY_ID_CTRL_F1 2
#define HOTKEY_ID_CTRL_F2 3
#define WM_TRAY_MESSAGE (WM_USER + 1)
#define ID_TRAY_EXIT 1003
#define ID_TRAY_SHOW 1004

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

// Forward declarations
void ExecuteSelectedItem(int index);
void ProcessCommand(const WCHAR* command);
void InitializeCommonShortcuts();
void SearchAndDisplayResults(const WCHAR* query);
void ShowLauncherWindow();
void HideLauncherWindow();
void AddDesktopShortcuts();
void SetEnglishInputMethod();
void AddTrayIcon();
void RemoveTrayIcon();
void ShowTrayMenu();

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

// Process command
void ProcessCommand(const WCHAR* command)
{
    // 记录要处理的命令
    char commandLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, command, -1, commandLog, sizeof(commandLog), NULL, NULL);
    char logMsg[1100] = {0};
    sprintf(logMsg, "ProcessCommand: 处理命令 '%s'", commandLog);
    LogToFile(logMsg);
    
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
            // Create edit control without ES_WANTRETURN to receive WM_KEYDOWN messages
            g_hEdit = CreateWindowExW(
                  0,
                  WC_EDITW,
                  L"",
                  WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                  10, 10, 370, 25,
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
            // 当窗口最小化时自动隐藏窗口
            if (wParam == SIZE_MINIMIZED)
            {
                LogToFile("WM_SIZE: 窗口最小化，自动隐藏窗口");
                ShowWindow(hwnd, SW_HIDE);
            }
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
                            SearchAndDisplayResults(searchText);
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
                                
                                // Handle return key - Ensure it executes the first item
                                {
                                    int itemCount = SendMessageW(g_hListBox, LB_GETCOUNT, 0, 0);
                                    char logMsg[200] = {0};
                                    sprintf(logMsg, "  EN_RETURN: 列表框项目数量: %d", itemCount);
                                    LogToFile(logMsg);
                                
                                    // If list has items, explicitly select and open the first one
                                    if (itemCount > 0)
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
                
                // Handle return key - Ensure it executes the first item
                {
                    int itemCount = SendMessageW(g_hListBox, LB_GETCOUNT, 0, 0);
                    char logMsg[200] = {0};
                    sprintf(logMsg, "  WM_KEYDOWN: 列表框项目数量: %d", itemCount);
                    LogToFile(logMsg);
                
                    // If list has items, explicitly select and open the first one
                    if (itemCount > 0)
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
                    else
                    {
                        // If no items, process as command
                        if (wcslen(currentText) > 0)
                        {
                            sprintf(logMsg, "  EditSubclassProc: List empty, processing input as command: '%s'", currentTextLog);
                            LogToFile(logMsg);
                            ProcessCommand(currentText);
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