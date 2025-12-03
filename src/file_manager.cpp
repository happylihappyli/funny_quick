#include "file_manager.h"
#include "common.h"
#include "logger.h"
#include "file_search_manager.h"
#include <windows.h>
#include <shlobj.h>    // 包含CSIDL_APPDATA和SHGetFolderPathW定义
#include <commctrl.h>  // 包含列表视图控件相关定义
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

// 全局变量声明（在gui_main.cpp中定义）
extern HWND g_hListView;
extern HWND g_hEdit;
extern bool g_fileMode;
extern std::vector<FileSearchResult> g_fileSearchResults;
extern FileSearchManager g_fileSearchManager;

// 前向声明
extern void UpdateWindowTitle();

/**
 * @brief 进入文件模式
 */
void EnterFileMode()
{
    g_fileMode = true;
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 清空搜索结果
    g_fileSearchResults.clear();
    
    // 添加提示信息
    AddHintRowToListView(L"💡 文件模式：输入文件名或路径进行搜索，输入'q'退出文件模式");
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新文件模式WebView显示
    UpdateFileModeWebView();
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    // 显示文件模式退出按钮，隐藏其他模式按钮
    if (g_hExitFileButton && IsWindow(g_hExitFileButton))
    {
        ShowWindow(g_hExitFileButton, SW_SHOW);
    }
    if (g_hExitCalcButton && IsWindow(g_hExitCalcButton))
    {
        ShowWindow(g_hExitCalcButton, SW_HIDE);
    }
    if (g_hExitBookmarkButton && IsWindow(g_hExitBookmarkButton))
    {
        ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    }
    
    // 进入文件模式后，自动显示文件搜索界面（搜索空字符串显示所有文件）
    SearchFiles(L"");
    
    LogToFile("EnterFileMode: 进入文件模式");
}

/**
 * @brief 退出文件模式
 */
void ExitFileMode()
{
    g_fileMode = false;
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 清空搜索结果
    g_fileSearchResults.clear();
    
    // 清空ListView
    ClearListView();
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    // 隐藏文件模式退出按钮
    if (g_hExitFileButton && IsWindow(g_hExitFileButton))
    {
        ShowWindow(g_hExitFileButton, SW_HIDE);
    }
    
    LogToFile("ExitFileMode: 退出文件模式");
}

/**
 * @brief 搜索文件
 * @param query 搜索查询字符串
 */
void SearchFiles(const WCHAR* query)
{
    // 添加详细日志记录
    char logMsg[512] = {0};
    char queryLog[256] = {0};
    if (query != NULL) {
        WideCharToMultiByte(CP_UTF8, 0, query, -1, queryLog, sizeof(queryLog), NULL, NULL);
        sprintf(logMsg, "SearchFiles: 开始搜索，查询: '%s' (长度: %zu)", queryLog, wcslen(query));
    } else {
        sprintf(logMsg, "SearchFiles: 开始搜索，查询为 NULL");
    }
    LogToFile(logMsg);
    
    if (!query || wcslen(query) == 0)
    {
        // 清空搜索结果
        g_fileSearchResults.clear();
        
        // 清空ListView
        ClearListView();
        
        // 添加提示信息
        AddHintRowToListView(L"💡 请输入文件名或路径进行搜索");
        
        // 更新WebView2显示
        UpdateFileModeWebView();
        
        LogToFile("SearchFiles: 查询为空，清空搜索结果并更新WebView2");
        return;
    }
    
    // 清空之前的搜索结果
    g_fileSearchResults.clear();
    
    // 使用文件搜索管理器进行搜索，限制最多显示30个文件
    g_fileSearchResults = g_fileSearchManager.SearchFiles(query, 30);
    
    // 显示搜索结果
    DisplayFileSearchResults();
    
    // 记录搜索结果
    if (query != NULL) {
        char resultLog[512] = {0};
        char queryLog2[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, query, -1, queryLog2, sizeof(queryLog2), NULL, NULL);
        sprintf(resultLog, "SearchFiles: 搜索 '%s' 找到 %zu 个文件", queryLog2, g_fileSearchResults.size());
        LogToFile(resultLog);
    }
}

/**
 * @brief 显示文件搜索结果
 */
void DisplayFileSearchResults()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return;
    }
    
    // 清空ListView
    ClearListView();
    
    // 添加提示信息
    if (g_fileSearchResults.empty())
    {
        AddHintRowToListView(L"💡 未找到匹配的文件，试试其他关键字");
        // 更新WebView2显示
        UpdateFileModeWebView();
        return;
    }
    
    // 添加文件搜索结果到ListView
    for (const auto& file : g_fileSearchResults)
    {
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = ListView_GetItemCount(g_hListView);
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(file.fileName.c_str());
        ListView_InsertItem(g_hListView, &lvItem);
        
        // 设置第二列（路径）
        lvItem.iSubItem = 1;
        lvItem.pszText = const_cast<LPWSTR>(file.fullPath.c_str());
        ListView_SetItem(g_hListView, &lvItem);
        
        // 设置第三列（大小）
        lvItem.iSubItem = 2;
        lvItem.pszText = const_cast<LPWSTR>(file.size.c_str());
        ListView_SetItem(g_hListView, &lvItem);
        
        // 设置第四列（修改时间）
        lvItem.iSubItem = 3;
        lvItem.pszText = const_cast<LPWSTR>(file.modified.c_str());
        ListView_SetItem(g_hListView, &lvItem);
        
        // 设置第五列（类型）
        lvItem.iSubItem = 4;
        lvItem.pszText = const_cast<LPWSTR>(file.fileType.c_str());
        ListView_SetItem(g_hListView, &lvItem);
    }
    
    // 更新WebView2显示搜索结果
    UpdateFileModeWebView();
    
    LogToFile("DisplayFileSearchResults: 显示文件搜索结果并更新WebView2");
}

/**
 * @brief 打开文件
 * @param filePath 文件路径
 */
void OpenFile(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        LogToFile("OpenFile: 文件路径为空");
        return;
    }
    
    // 使用ShellExecute打开文件
    HINSTANCE result = ShellExecuteW(NULL, L"open", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    
    if ((INT_PTR)result <= 32)
    {
        // 打开失败
        char logMsg[512] = {0};
        char pathLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, pathLog, sizeof(pathLog), NULL, NULL);
        sprintf(logMsg, "OpenFile: 无法打开文件 '%s'，错误代码: %Id", pathLog, (INT_PTR)result);
        LogToFile(logMsg);
        
        // 显示错误消息
        MessageBoxW(g_hMainWindow, L"无法打开文件，请检查文件是否存在或权限是否足够。", L"打开文件失败", MB_OK | MB_ICONERROR);
    }
    else
    {
        // 打开成功
        char logMsg[512] = {0};
        char pathLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, pathLog, sizeof(pathLog), NULL, NULL);
        sprintf(logMsg, "OpenFile: 成功打开文件 '%s'", pathLog);
        LogToFile(logMsg);
    }
}

/**
 * @brief 打开文件所在文件夹
 * @param filePath 文件路径
 */
void OpenFileFolder(const std::wstring& filePath)
{
    if (filePath.empty())
    {
        LogToFile("OpenFileFolder: 文件路径为空");
        return;
    }
    
    // 使用ShellExecute打开文件所在文件夹
    HINSTANCE result = ShellExecuteW(NULL, L"explore", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    
    if ((INT_PTR)result <= 32)
    {
        // 打开失败
        char logMsg[512] = {0};
        char pathLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, pathLog, sizeof(pathLog), NULL, NULL);
        sprintf(logMsg, "OpenFileFolder: 无法打开文件所在文件夹 '%s'，错误代码: %Id", pathLog, (INT_PTR)result);
        LogToFile(logMsg);
        
        // 显示错误消息
        MessageBoxW(g_hMainWindow, L"无法打开文件所在文件夹，请检查文件是否存在。", L"打开文件夹失败", MB_OK | MB_ICONERROR);
    }
    else
    {
        // 打开成功
        char logMsg[512] = {0};
        char pathLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, pathLog, sizeof(pathLog), NULL, NULL);
        sprintf(logMsg, "OpenFileFolder: 成功打开文件所在文件夹 '%s'", pathLog);
        LogToFile(logMsg);
    }
}

/**
 * @brief 显示文件模式帮助信息
 */
void ShowFileModeHelpInfo()
{
    // 清空ListView
    ClearListView();
    
    // 添加帮助信息
    const WCHAR* helpItems[] = {
        L"💡 文件模式使用说明",
        L"• 输入文件名或路径进行搜索",
        L"• 双击文件项打开文件",
        L"• 右键文件项可打开文件所在文件夹",
        L"• 支持基本的文件名匹配搜索",
        L"• 输入'q'退出文件模式"
    };
    
    // 添加帮助信息到ListView
    for (const auto& item : helpItems)
    {
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = ListView_GetItemCount(g_hListView);
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(item);
        ListView_InsertItem(g_hListView, &lvItem);
    }
    
    LogToFile("ShowFileModeHelpInfo: 显示文件模式帮助信息");
}