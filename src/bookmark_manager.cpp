#include "bookmark_manager.h"
#include "common.h"
#include "logger.h"
#include "resource.h"
#include "webview_manager.h"
#include <vector>
#include <string>
#include <algorithm>
#include <shlobj.h>
#include <fstream>
#include <codecvt>
#include <commctrl.h>

// 前向声明
extern void UpdateWindowTitle();

/**
 * @brief 网址管理模块实现文件
 * 
 * 此文件包含所有网址收藏管理相关的函数实现，
 * 包括添加、删除、保存、加载、搜索和同步Chrome书签等功能。
 */

/**
 * @brief 添加网址收藏
 * 
 * 此函数用于添加新的网址收藏到收藏列表
 * 
 * @param name 网址名称
 * @param url 网址URL
 */
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

/**
 * @brief 删除网址收藏
 * 
 * 此函数用于从收藏列表中删除指定索引的网址收藏
 * 
 * @param index 要删除的网址收藏索引
 */
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

/**
 * @brief 保存网址收藏到文件
 * 
 * 此函数将当前网址收藏列表保存到data\bookmarks.txt文件中
 */
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

/**
 * @brief 从文件加载网址收藏
 * 
 * 此函数从data\bookmarks.txt文件中加载网址收藏列表
 */
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
        LogToFile("LoadBookmarks: 发生异常");
    }
}

/**
 * @brief 搜索网址收藏
 * 
 * 此函数根据查询字符串搜索网址收藏，支持名称和URL的模糊搜索
 * 
 * @param query 搜索查询字符串
 */
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

/**
 * @brief 同步Chrome书签
 * 
 * 此函数从Chrome浏览器中同步书签到本地网址收藏列表
 */
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
            if (pos >= content.length() || content[pos] != '\"') continue;
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
            if (urlPos >= content.length() || content[urlPos] != '\"') continue;
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

/**
 * @brief 显示网址管理对话框
 * 
 * 此函数显示网址收藏管理对话框，允许用户添加、编辑和删除网址收藏
 */
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

/**
 * @brief 刷新网址列表显示
 * 
 * 此函数用于刷新列表框中的网址收藏显示
 */
void DisplayBookmarkResults()
{
    LogToFile("DisplayBookmarkResults: 刷新网址列表显示");
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    
    // 确定要显示的网址列表（搜索结果或全部网址）
    const auto& displayList = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
    
    // 添加网址到列表框
    for (const auto& bookmark : displayList)
    {
        LVITEMW lvItem = {0};
        lvItem.iItem = (int)ListView_GetItemCount(g_hListView);
        lvItem.iSubItem = 0;
        
        // 创建显示字符串：名称 - URL
        std::wstring displayStr = bookmark.first + L" - " + bookmark.second;
        lvItem.pszText = (LPWSTR)displayStr.c_str();
        
        ListView_InsertItem(g_hListView, &lvItem);
    }
    
    // 记录显示的网址数量
    char logMsg[200] = {0};
    sprintf(logMsg, "DisplayBookmarkResults: 显示了 %zu 个网址", displayList.size());
    LogToFile(logMsg);
}

/**
 * @brief 检查字符串是否为有效的URL
 * 
 * 此函数检查给定的字符串是否符合URL格式
 * 
 * @param text 要检查的字符串
 * @return true 如果是有效的URL
 * @return false 如果不是有效的URL
 */
bool IsURL(const WCHAR* text)
{
    if (!text || wcslen(text) == 0)
        return false;
    
    std::wstring url = text;
    
    // 检查是否包含常见的URL前缀
    if (url.find(L"http://") == 0 || url.find(L"https://") == 0 ||
        url.find(L"ftp://") == 0 || url.find(L"file://") == 0 ||
        url.find(L"www.") == 0)
    {
        return true;
    }
    
    // 检查是否包含域名和顶级域名
    if (url.find(L'.') != std::wstring::npos && 
        (url.find(L".com") != std::wstring::npos ||
         url.find(L".cn") != std::wstring::npos ||
         url.find(L".net") != std::wstring::npos ||
         url.find(L".org") != std::wstring::npos ||
         url.find(L".gov") != std::wstring::npos))
    {
        return true;
    }
    
    return false;
}

/**
 * @brief 网址管理对话框过程
 * 
 * 此函数处理网址管理对话框的消息处理
 * 
 * @param hwnd 对话框窗口句柄
 * @param uMsg 消息类型
 * @param wParam 消息参数
 * @param lParam 消息参数
 * @return INT_PTR 消息处理结果
 */
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
            UINT controlId = LOWORD(wParam);
            
            if (controlId == IDC_BOOKMARK_LIST)
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
            else if (controlId == IDC_BOOKMARK_ADD)
            {
                AddBookmarkFromDialog(hwnd);
                return TRUE;
            }
            else if (controlId == IDC_BOOKMARK_UPDATE)
            {
                UpdateBookmarkFromDialog(hwnd);
                return TRUE;
            }
            else if (controlId == IDC_BOOKMARK_DELETE)
            {
                DeleteBookmarkFromDialog(hwnd);
                return TRUE;
            }
            else if (controlId == IDC_BOOKMARK_CLOSE)
            {
                EndDialog(hwnd, IDOK);
                return TRUE;
            }
            
            break;
        }
        
        case WM_CLOSE:
        {
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
    }
    
    return FALSE;
}

/**
 * @brief 刷新网址列表
 * 
 * 此函数用于刷新对话框中的网址列表显示
 * 
 * @param hList 列表框句柄
 */
void RefreshBookmarkList(HWND hList)
{
    LogToFile("RefreshBookmarkList: 刷新网址列表");
    
    // 清空列表框
    ListView_DeleteAllItems(hList);
    
    // 添加网址到列表框
    for (const auto& bookmark : g_bookmarks)
    {
        LVITEMW lvItem = {0};
        lvItem.iItem = (int)ListView_GetItemCount(hList);
        lvItem.iSubItem = 0;
        
        // 创建显示字符串：名称 - URL
        std::wstring displayStr = bookmark.first + L" - " + bookmark.second;
        lvItem.pszText = (LPWSTR)displayStr.c_str();
        
        ListView_InsertItem(hList, &lvItem);
    }
    
    // 记录显示的网址数量
    char logMsg[200] = {0};
    sprintf(logMsg, "RefreshBookmarkList: 显示了 %zu 个网址", g_bookmarks.size());
    LogToFile(logMsg);
}

/**
 * @brief 从对话框添加网址
 * 
 * 此函数处理从对话框添加网址的操作
 * 
 * @param hDlg 对话框句柄
 */
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

/**
 * @brief 从对话框更新网址
 * 
 * 此函数处理从对话框更新网址的操作
 * 
 * @param hDlg 对话框句柄
 */
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
    
    // 清空编辑框
    SetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_NAME), L"");
    SetWindowTextW(GetDlgItem(hDlg, IDC_BOOKMARK_URL), L"");
    
    // 重新选择更新后的项
    ListView_SetItemState(hList, selIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    
    LogToFile("UpdateBookmarkFromDialog: 网址更新成功");
}

/**
 * @brief 从对话框删除网址
 * 
 * 此函数处理从对话框删除网址的操作
 * 
 * @param hDlg 对话框句柄
 */
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
    
    LogToFile("DeleteBookmarkFromDialog: 网址删除成功");
}

/**
 * @brief 进入网址收藏模式
 * 
 * 此函数切换到网址收藏模式，显示网址收藏列表
 */
void EnterBookmarkMode()
{
    LogToFile("EnterBookmarkMode: 进入网址收藏模式");
    
    // 设置书签模式标志
    g_bookmarkMode = true;
    
    // 清空文本框中的"wz"内容
    SetWindowTextW(g_hEdit, L"");
    
    // 加载网址收藏
    LoadBookmarks();
    
    // 清空搜索结果
    g_bookmarkSearchResults.clear();
    
    // 显示网址收藏列表到ListView
    DisplayBookmarkResults();
    
    // 更新WebView2显示
    UpdateBookmarkModeWebView();
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    LogToFile("EnterBookmarkMode: 网址收藏模式已激活，文本框已清空");
}

/**
 * @brief 退出网址收藏模式
 * 
 * 此函数退出网址收藏模式，返回到正常模式
 */
void ExitBookmarkMode()
{
    LogToFile("ExitBookmarkMode: 退出网址收藏模式");
    
    // 清除书签模式标志
    g_bookmarkMode = false;
    
    // 清空搜索结果
    g_bookmarkSearchResults.clear();
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    
    // 清空文本框中的"q"内容
    SetWindowTextW(g_hEdit, L"");
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    LogToFile("ExitBookmarkMode: 网址收藏模式已退出，文本框已清空");
}
