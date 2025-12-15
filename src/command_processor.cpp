#include "command_processor.h"
#include "logger.h"
#include "ui_helpers.h"
#include "calculator.h"
#include "webview_manager.h"
#include "dir_mode_manager.h"
#include "window_size_handler.h"
#include "bookmark_manager.h"
#include "file_search_manager.h"
#include "file_manager.h"
#include "tray_icon_manager.h"
#include <shlobj.h>
#include <strsafe.h>
#include <commctrl.h>
#include <algorithm>
#include <shellapi.h>

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
    
    // 检查是否在文件模式下输入"q"退出
    if (g_fileMode && wcscmp(command, L"q") == 0)
    {
        LogToFile("ProcessCommand: 在文件模式下输入'q'，退出文件模式");
        ExitFileMode();
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
    
    // 检查是否是"file"命令，用于进入文件模式
    if (wcscmp(command, L"file") == 0)
    {
        LogToFile("ProcessCommand: 识别为'file'命令，进入文件模式");
        EnterFileMode();
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

// Import desktop shortcuts with duplicate checking
int ImportDesktopShortcuts()
{
    int addedCount = 0;
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
                    WCHAR fullPath[MAX_PATH] = {0};
                    wsprintfW(fullPath, L"%s\\%s", desktopPath, findData.cFileName);
                    
                    // Check for duplicates
                    bool exists = false;
                    for (const auto& existing : g_shortcuts)
                    {
                        if (_wcsicmp(existing.path, fullPath) == 0)
                        {
                            exists = true;
                            break;
                        }
                    }
                    
                    if (!exists)
                    {
                        ShortcutItem shortcut = {0};
                        
                        // Copy filename for manipulation
                        WCHAR fileName[MAX_PATH];
                        wcscpy(fileName, findData.cFileName);
                        
                        // Extract name without extension
                        WCHAR* dotPos = wcsrchr(fileName, L'.');
                        if (dotPos) *dotPos = L'\0';
                        
                        wcscpy(shortcut.name, fileName);
                        wcscpy(shortcut.path, fullPath);
                        
                        // Set default comment and icon
                        WCHAR defaultComment[512] = {0};
                        wsprintfW(defaultComment, L"桌面快捷方式: %s", findData.cFileName);
                        wcscpy(shortcut.comment, defaultComment);
                        wcscpy(shortcut.iconPath, shortcut.path);
                        
                        shortcut.type = 2; // Application
                        shortcut.usageCount = 0;
                        
                        g_shortcuts.push_back(shortcut);
                        addedCount++;
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            
            FindClose(hFind);
        }
    }
    
    if (addedCount > 0)
    {
        SaveShortcuts();
    }
    
    return addedCount;
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
                    
                    // 设置默认备注和图标路径
                    WCHAR defaultComment[512] = {0};
                    wsprintfW(defaultComment, L"桌面快捷方式: %s", findData.cFileName);
                    wcscpy(shortcut.comment, defaultComment);
                    wcscpy(shortcut.iconPath, shortcut.path); // 使用快捷方式本身作为图标源
                    
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
    // 优先从文件加载用户保存的快捷方式
    if (LoadShortcuts())
    {
        LogToFile("InitializeCommonShortcuts: 已从文件加载快捷方式，跳过默认初始化");
        return;
    }
    
    g_shortcuts.clear();
    
    // Add desktop shortcuts first
    AddDesktopShortcuts();
    
    // Desktop folder
    ShortcutItem desktop = {0};
    wcscpy(desktop.name, L"Desktop");
    wcscpy(desktop.comment, L"桌面文件夹，包含所有桌面快捷方式");
    wcscpy(desktop.iconPath, L"shell32.dll,-34"); // 文件夹图标
    desktop.type = 0;
    desktop.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, desktop.path, CSIDL_DESKTOP, FALSE);
    g_shortcuts.push_back(desktop);
    
    // Show Desktop
    ShortcutItem showDesktop = {0};
    wcscpy(showDesktop.name, L"Show Desktop");
    wcscpy(showDesktop.comment, L"快速显示桌面，隐藏所有窗口");
    wcscpy(showDesktop.iconPath, L"shell32.dll,-35"); // 桌面图标
    wcscpy(showDesktop.path, L"explorer.exe shell:::{3080F90D-D7AD-11D9-BD98-0000947B0257}");
    showDesktop.type = 2;
    showDesktop.usageCount = 0;
    g_shortcuts.push_back(showDesktop);
    
    // Start Menu Programs
    ShortcutItem startMenu = {0};
    wcscpy(startMenu.name, L"Start Menu");
    wcscpy(startMenu.comment, L"开始菜单程序文件夹");
    wcscpy(startMenu.iconPath, L"shell32.dll,-155"); // 程序文件夹图标
    startMenu.type = 0;
    startMenu.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, startMenu.path, CSIDL_PROGRAMS, FALSE);
    g_shortcuts.push_back(startMenu);
    
    // Downloads folder
    ShortcutItem downloads = {0};
    wcscpy(downloads.name, L"Downloads");
    wcscpy(downloads.comment, L"下载文件夹，包含所有下载的文件");
    wcscpy(downloads.iconPath, L"shell32.dll,-176"); // 下载文件夹图标
    downloads.type = 0;
    downloads.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, downloads.path, CSIDL_MYDOCUMENTS, FALSE);
    wcscat(downloads.path, L"\\Downloads");
    g_shortcuts.push_back(downloads);
    
    // Documents folder
    ShortcutItem documents = {0};
    wcscpy(documents.name, L"Documents");
    wcscpy(documents.comment, L"文档文件夹，包含个人文档");
    wcscpy(documents.iconPath, L"shell32.dll,-235"); // 文档文件夹图标
    documents.type = 0;
    documents.usageCount = 0;
    SHGetSpecialFolderPathW(NULL, documents.path, CSIDL_MYDOCUMENTS, FALSE);
    g_shortcuts.push_back(documents);
    
    // Google URL
    ShortcutItem google = {0};
    wcscpy(google.name, L"Google");
    wcscpy(google.comment, L"谷歌搜索引擎，全球最大的搜索引擎");
    wcscpy(google.iconPath, L"https://www.google.com/favicon.ico");
    wcscpy(google.path, L"https://www.google.com");
    google.type = 1;
    google.usageCount = 0;
    g_shortcuts.push_back(google);
    
    // Baidu URL
    ShortcutItem baidu = {0};
    wcscpy(baidu.name, L"Baidu");
    wcscpy(baidu.comment, L"百度搜索引擎，中文搜索引擎");
    wcscpy(baidu.iconPath, L"https://www.baidu.com/favicon.ico");
    wcscpy(baidu.path, L"https://www.baidu.com");
    baidu.type = 1;
    baidu.usageCount = 0;
    g_shortcuts.push_back(baidu);
    
    // File Explorer
    ShortcutItem explorer = {0};
    wcscpy(explorer.name, L"Explorer");
    wcscpy(explorer.comment, L"文件资源管理器，浏览和管理文件");
    wcscpy(explorer.iconPath, L"explorer.exe");
    wcscpy(explorer.path, L"explorer.exe");
    explorer.type = 2;
    explorer.usageCount = 0;
    g_shortcuts.push_back(explorer);
    
    // Notepad
    ShortcutItem notepad = {0};
    wcscpy(notepad.name, L"Notepad");
    wcscpy(notepad.comment, L"记事本程序，简单的文本编辑器");
    wcscpy(notepad.iconPath, L"notepad.exe");
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
    
    // Control Panel
    ShortcutItem controlPanel = {0};
    wcscpy(controlPanel.name, L"Control Panel");
    wcscpy(controlPanel.comment, L"控制面板，管理系统设置");
    wcscpy(controlPanel.iconPath, L"shell32.dll,-137"); // Control Panel icon
    wcscpy(controlPanel.path, L"control.exe");
    controlPanel.type = 2;
    controlPanel.usageCount = 0;
    g_shortcuts.push_back(controlPanel);
    
    // Uninstall Programs
    ShortcutItem uninstall = {0};
    wcscpy(uninstall.name, L"Uninstall Programs");
    wcscpy(uninstall.comment, L"卸载程序，管理已安装的软件");
    wcscpy(uninstall.iconPath, L"shell32.dll,-16718"); // Uninstall icon (approximate)
    wcscpy(uninstall.path, L"appwiz.cpl");
    uninstall.type = 2;
    uninstall.usageCount = 0;
    g_shortcuts.push_back(uninstall);
    
    // Advanced System Settings
    ShortcutItem sysSettings = {0};
    wcscpy(sysSettings.name, L"System Settings");
    wcscpy(sysSettings.comment, L"高级系统设置，环境变量等");
    wcscpy(sysSettings.iconPath, L"sysdm.cpl"); // Use sysdm.cpl as icon source
    wcscpy(sysSettings.path, L"SystemPropertiesAdvanced.exe");
    sysSettings.type = 2;
    sysSettings.usageCount = 0;
    g_shortcuts.push_back(sysSettings);
    
    // 保存初始快捷方式，便于后续编辑持久化
    SaveShortcuts();
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
            L"💡 输入 set 进入设置模式，输入 dir 进入目录管理模式",
            L"💡 输入 file 进入文件模式，输入 help 显示帮助"
        };
        AddMultiLineHintsToListView(hints, 4);
        for (int i = 0; i < 5; ++i)
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
        
        bool match = false;
        size_t queryLen = wcslen(query);

        // Helper to check if str contains query (case-insensitive)
        auto containsQuery = [&](const WCHAR* str) -> bool {
            if (!str) return false;
            size_t strLen = wcslen(str);
            if (queryLen > strLen) return false;
            
            for (size_t j = 0; j <= strLen - queryLen; j++)
            {
                if (_wcsnicmp(&str[j], query, queryLen) == 0)
                    return true;
            }
            return false;
        };
        
        // Check for exact match (already case-insensitive)
        if (_wcsicmp(g_shortcuts[i].name, query) == 0)
        {
            match = true;
            sprintf(logMsg, "HandleShortcutSearch: 找到精确匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
        }
        // Check name substring
        else if (containsQuery(g_shortcuts[i].name))
        {
            match = true;
            sprintf(logMsg, "HandleShortcutSearch: 找到名称匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
        }
        // Check path/URL substring
        else if (containsQuery(g_shortcuts[i].path))
        {
            match = true;
            sprintf(logMsg, "HandleShortcutSearch: 找到路径/URL匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
        }
        // Check comment substring
        else if (containsQuery(g_shortcuts[i].comment))
        {
            match = true;
            sprintf(logMsg, "HandleShortcutSearch: 找到备注匹配 '%s'", itemNameLog);
            LogToFile(logMsg);
        }
        
        if (match)
        {
            g_searchResults.push_back(g_shortcuts[i]);
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
    g_currentViewMode = ViewMode::SEARCH;
    // 无论是否有结果，WebView2 都显示提示信息和最新列表
    std::wstring html;
    CreateWebView2HTML(g_searchResults, webViewHints, html);
    UpdateWebView2Content(html.c_str());
}

void SearchAndDisplayResults(const WCHAR* query)
{
    // Save search query for view restoration
    if (query) g_lastSearchQuery = query;
    else g_lastSearchQuery = L"";

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
        
        // WZ模式搜索时，显示专门的网址收藏页面，而不是普通搜索页面
        UpdateBookmarkModeWebView();
    }
    else if (g_fileMode)
    {
        // 文件模式：只进行文件搜索，不进行快捷方式搜索
        // 文件搜索由SearchFiles函数处理，这里只清空快捷方式搜索结果
        g_searchResults.clear();
        
        // 文件模式搜索时，显示文件搜索页面
        UpdateFileModeWebView();
        
        LogToFile("SearchAndDisplayResults: 文件模式下，跳过快捷方式搜索，只进行文件搜索");
    }
    else
    {
        // 普通模式：同时搜索快捷方式和网址收藏
        SearchBookmarks(query);
        HandleShortcutSearch(query);
        
        // 默认模式搜索时，显示普通搜索页面
        UpdateWebViewForSearch(webViewHints);
    }
    
    // 显示搜索结果到ListView
    DisplaySearchResults();
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
    // 检查是否是文件模式
    if (g_fileMode)
    {
        LogToFile("ExecuteSelectedItem: 文件模式下调用，转发到ExecuteFileModeItem");
        ExecuteFileModeItem(index);
        return;
    }
    
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

// 执行文件模式下的选中项
void ExecuteFileModeItem(INT_PTR index)
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
    
    // 修复：添加g_fileSearchResults.empty()检查
    if (adjustedIndex < 0 || (size_t)adjustedIndex >= g_fileSearchResults.size() || g_fileSearchResults.empty())
    {
        char logMsg[200] = {0};
        sprintf(logMsg, "ExecuteFileModeItem: 无效索引 %Id（调整后 %Id），文件搜索结果大小为 %zu，是否为空: %s", 
                index, adjustedIndex, g_fileSearchResults.size(), g_fileSearchResults.empty() ? "是" : "否");
        LogToFile(logMsg);
        return;
    }
    
    if (hintRowCount > 0)
    {
        char adjustLog[200] = {0};
        sprintf(adjustLog, "ExecuteFileModeItem: 检测到 %d 行提示行，实际执行索引调整为 %Id", hintRowCount, adjustedIndex);
        LogToFile(adjustLog);
    }
    
    FileSearchResult& file = g_fileSearchResults[(size_t)adjustedIndex];
    
    // 记录要执行的文件信息
    char fileNameLog[1024] = {0};
    char filePathLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, file.fileName.c_str(), -1, fileNameLog, sizeof(fileNameLog), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, file.fullPath.c_str(), -1, filePathLog, sizeof(filePathLog), NULL, NULL);
    
    char logMsg[1100] = {0};
    sprintf(logMsg, "ExecuteFileModeItem: 执行文件[%Id] '%s' (路径: '%s', 类型: %s, 文件夹: %s)", 
            adjustedIndex, fileNameLog, filePathLog, file.isFile ? "文件" : "非文件", file.isFolder ? "是" : "否");
    LogToFile(logMsg);
    
    // 执行文件或打开文件夹
    HINSTANCE result;
    if (file.isFolder)
    {
        LogToFile("ExecuteFileModeItem: 打开文件夹");
        result = ShellExecuteW(NULL, L"open", file.fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else
    {
        LogToFile("ExecuteFileModeItem: 打开文件");
        result = ShellExecuteW(NULL, L"open", file.fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    
    // 记录执行结果
    sprintf(logMsg, "ExecuteFileModeItem: ShellExecuteW 返回值: %Id", (INT_PTR)result);
    LogToFile(logMsg);
    
    // Only show error message if execution failed
    if ((INT_PTR)result <= 32)
    {
        sprintf(logMsg, "ExecuteFileModeItem: 执行失败，错误代码 %Id", (INT_PTR)result);
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
        LogToFile("ExecuteFileModeItem: 执行成功");
    }
}
