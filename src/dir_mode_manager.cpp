#include "dir_mode_manager.h"
#include "common.h"
#include "webview_manager.h"
#include "logger.h"
#include "ui_manager.h"
#include <fstream>
#include <sstream>
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <string>

// 前向声明
extern void UpdateWindowTitle();

/**
 * @brief 获取系统所有驱动器列表
 * 
 * @return std::vector<std::wstring> 驱动器列表（如C:\, D:\等）
 */
std::vector<std::wstring> GetDrives()
{
    std::vector<std::wstring> drives;
    WCHAR driveStrings[MAX_PATH] = {0};
    DWORD result = GetLogicalDriveStringsW(MAX_PATH, driveStrings);
    
    if (result > 0 && result < MAX_PATH)
    {
        WCHAR* drive = driveStrings;
        while (*drive)
        {
            drives.push_back(std::wstring(drive));
            drive += wcslen(drive) + 1;
        }
    }
    
    return drives;
}

/**
 * @brief 获取常用路径列表
 * 
 * @return std::vector<std::pair<std::wstring, bool>> 常用路径列表，包含显示名称和是否为目录
 */
std::vector<std::pair<std::wstring, bool>> GetCommonPaths()
{
    std::vector<std::pair<std::wstring, bool>> paths;
    WCHAR path[MAX_PATH] = {0};
    
    // 桌面
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_DESKTOP, FALSE))
    {
        paths.push_back({std::wstring(L"📁 桌面"), true});
    }
    
    // 文档
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_MYDOCUMENTS, FALSE))
    {
        paths.push_back({std::wstring(L"📁 文档"), true});
    }
    
    // 下载
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_MYDOCUMENTS, FALSE))
    {
        std::wstring downloads = std::wstring(path) + L"\\Downloads";
        DWORD attrs = GetFileAttributesW(downloads.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            paths.push_back({std::wstring(L"📁 下载"), true});
        }
    }
    
    // 图片
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_MYPICTURES, FALSE))
    {
        paths.push_back({std::wstring(L"📁 图片"), true});
    }
    
    // 视频
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_MYVIDEO, FALSE))
    {
        paths.push_back({std::wstring(L"📁 视频"), true});
    }
    
    // 音乐
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_MYMUSIC, FALSE))
    {
        paths.push_back({std::wstring(L"📁 音乐"), true});
    }
    
    // 用户目录
    if (SHGetSpecialFolderPathW(NULL, path, CSIDL_PROFILE, FALSE))
    {
        paths.push_back({std::wstring(L"📁 用户目录"), true});
    }
    
    return paths;
}

/**
 * @brief 获取指定目录的内容
 * 
 * @param path 目录路径
 * @return std::vector<std::pair<std::wstring, bool>> 目录内容列表，包含文件名和是否为目录
 */
std::vector<std::pair<std::wstring, bool>> GetDirectoryContents(const WCHAR* path)
{
    std::vector<std::pair<std::wstring, bool>> contents;
    std::wstring searchPath = std::wstring(path);
    
    // 确保路径以\结尾
    if (searchPath.back() != L'\\')
    {
        searchPath += L"\\";
    }
    searchPath += L"*";
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // 跳过 . 和 ..
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
            {
                continue;
            }
            
            bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            contents.push_back({std::wstring(findData.cFileName), isDir});
        } while (FindNextFileW(hFind, &findData));
        
        FindClose(hFind);
    }
    
    // 排序：目录在前，然后按名称排序
    std::sort(contents.begin(), contents.end(), [](const std::pair<std::wstring, bool>& a, const std::pair<std::wstring, bool>& b) {
        if (a.second != b.second)
        {
            return a.second > b.second;  // 目录在前
        }
        return a.first < b.first;  // 按名称排序
    });
    
    return contents;
}

/**
 * @brief 进入目录浏览模式
 * 
 * 此函数用于切换到目录浏览模式，显示驱动器和常用路径
 */
void EnterDirMode()
{
    LogToFile("EnterDirMode: 进入目录浏览模式");
    
    g_settingsMenuMode = false;
    g_dirMode = true;
    g_currentDirPath.clear();
    g_expandedPaths.clear();
    
    // 隐藏其他按钮
    ShowWindow(g_hExitCalcButton, SW_HIDE);
    ShowWindow(g_hExitBookmarkButton, SW_HIDE);
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 显示列表框
    ShowWindow(g_hListView, SW_SHOW);
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    
    // 显示模式提示信息
    const WCHAR* hints[] = {
        L"💡 浏览常用路径和驱动器",
        L"💡 点击展开目录，双击打开文件",
        L"💡 输入 q 退出目录浏览模式"
    };
    AddMultiLineHintsToListView(hints, 3);
    
    // 更新WebView2显示
    UpdateDirModeWebView();
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

/**
 * @brief 退出目录浏览模式
 * 
 * 此函数用于退出目录浏览模式，恢复默认界面
 */
void ExitDirMode()
{
    LogToFile("ExitDirMode: 退出目录浏览模式");
    
    g_dirMode = false;
    g_currentDirPath.clear();
    g_expandedPaths.clear();
    
    // 显示设置按钮
    ShowWindow(g_hSettingsButton, SW_SHOW);
    
    // 清空列表框
    ListView_DeleteAllItems(g_hListView);
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    // 设置焦点到编辑框
    SetFocus(g_hEdit);
}

/**
 * @brief 生成驱动器HTML内容
 * 
 * 此函数生成驱动器列表的HTML内容
 * 
 * @return std::wstring 驱动器HTML内容
 */
std::wstring GenerateDrivesHtml()
{
    std::wstring drivesHtml;
    drivesHtml += L"<div style='font-weight: bold; margin-bottom: 10px; color: #333;'>💾 驱动器</div>";
    
    std::vector<std::wstring> drives = GetDrives();
    for (const auto& drive : drives)
    {
        std::wstring driveName = drive;
        if (driveName.back() == L'\\')
        {
            driveName.pop_back();
        }
        
        // 转义路径中的反斜杠
        std::wstring escapedDrive = drive;
        size_t pos = 0;
        while ((pos = escapedDrive.find(L"\\", pos)) != std::wstring::npos)
        {
            escapedDrive.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        
        drivesHtml += L"<div class='dir-item dir' onclick='toggleDir(\"" + escapedDrive + L"\")' ondblclick='openFile(\"" + escapedDrive + L"\", true)'>";
        drivesHtml += L"<span class='dir-icon'>💾</span>";
        drivesHtml += L"<span class='dir-name'>" + driveName + L"</span>";
        drivesHtml += L"<span class='dir-expand'>点击展开</span>";
        drivesHtml += L"</div>";
    }
    
    return drivesHtml;
}

/**
 * @brief 生成常用路径HTML内容
 * 
 * 此函数生成常用路径列表的HTML内容
 * 
 * @return std::wstring 常用路径HTML内容
 */
std::wstring GenerateCommonPathsHtml()
{
    std::wstring commonPathsHtml;
    commonPathsHtml += L"<div style='font-weight: bold; margin-top: 20px; margin-bottom: 10px; color: #333;'>📁 常用路径</div>";
    
    std::vector<std::pair<std::wstring, bool>> commonPaths = GetCommonPaths();
    WCHAR pathBuf[MAX_PATH] = {0};
    
    for (size_t i = 0; i < commonPaths.size(); i++)
    {
        std::wstring displayName = commonPaths[i].first;
        std::wstring actualPath;
        
        // 根据显示名称获取实际路径
        if (displayName == L"📁 桌面")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_DESKTOP, FALSE);
            actualPath = pathBuf;
        }
        else if (displayName == L"📁 文档")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_MYDOCUMENTS, FALSE);
            actualPath = pathBuf;
        }
        else if (displayName == L"📁 下载")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_MYDOCUMENTS, FALSE);
            actualPath = std::wstring(pathBuf) + L"\\Downloads";
        }
        else if (displayName == L"📁 图片")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_MYPICTURES, FALSE);
            actualPath = pathBuf;
        }
        else if (displayName == L"📁 视频")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_MYVIDEO, FALSE);
            actualPath = pathBuf;
        }
        else if (displayName == L"📁 音乐")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_MYMUSIC, FALSE);
            actualPath = pathBuf;
        }
        else if (displayName == L"📁 用户目录")
        {
            SHGetSpecialFolderPathW(NULL, pathBuf, CSIDL_PROFILE, FALSE);
            actualPath = pathBuf;
        }
        
        if (!actualPath.empty())
        {
            // 转义路径中的反斜杠
            std::wstring escapedPath = actualPath;
            size_t pos = 0;
            while ((pos = escapedPath.find(L"\\", pos)) != std::wstring::npos)
            {
                escapedPath.replace(pos, 1, L"\\\\");
                pos += 2;
            }
            
            commonPathsHtml += L"<div class='dir-item dir' onclick='toggleDir(\"" + escapedPath + L"\")' ondblclick='openFile(\"" + escapedPath + L"\", true)'>";
            commonPathsHtml += L"<span class='dir-icon'>📁</span>";
            commonPathsHtml += L"<span class='dir-name'>" + displayName + L"</span>";
            commonPathsHtml += L"<span class='dir-expand'>点击展开</span>";
            commonPathsHtml += L"</div>";
        }
    }
    
    return commonPathsHtml;
}

/**
 * @brief 生成已展开目录HTML内容
 * 
 * 此函数生成已展开目录列表的HTML内容
 * 
 * @return std::wstring 已展开目录HTML内容
 */
std::wstring GenerateExpandedDirsHtml()
{
    std::wstring expandedDirsHtml;
    
    if (!g_expandedPaths.empty())
    {
        expandedDirsHtml += L"<div style='font-weight: bold; margin-top: 20px; margin-bottom: 10px; color: #333;'>📂 已展开的目录</div>";
        
        // 显示所有已展开的目录内容
        for (const auto& expandedPath : g_expandedPaths)
        {
            expandedDirsHtml += L"<div style='font-weight: bold; margin-top: 15px; margin-bottom: 8px; color: #555; font-size: 14px;'>📂 " + expandedPath + L"</div>";
            std::vector<std::pair<std::wstring, bool>> contents = GetDirectoryContents(expandedPath.c_str());
            
            for (const auto& item : contents)
            {
                std::wstring fullPath = expandedPath;
                if (fullPath.back() != L'\\')
                {
                    fullPath += L"\\";
                }
                fullPath += item.first;
                
                // 转义路径中的反斜杠
                std::wstring escapedPath = fullPath;
                size_t pos = 0;
                while ((pos = escapedPath.find(L"\\", pos)) != std::wstring::npos)
                {
                    escapedPath.replace(pos, 1, L"\\\\");
                    pos += 2;
                }
                
                std::wstring icon = item.second ? L"📁" : L"📄";
                std::wstring className = item.second ? L"dir-item dir" : L"dir-item file";
                
                expandedDirsHtml += L"<div class='" + className + L"' style='margin-left: 20px;'";
                if (item.second)
                {
                    expandedDirsHtml += L" onclick='toggleDir(\"" + escapedPath + L"\")'";
                }
                expandedDirsHtml += L" ondblclick='openFile(\"" + escapedPath + L"\", " + (item.second ? L"true" : L"false") + L")'>";
                expandedDirsHtml += L"<span class='dir-icon'>" + icon + L"</span>";
                expandedDirsHtml += L"<span class='dir-name'>" + item.first + L"</span>";
                if (item.second)
                {
                    expandedDirsHtml += L"<span class='dir-expand'>点击展开</span>";
                }
                expandedDirsHtml += L"</div>";
            }
        }
    }
    
    return expandedDirsHtml;
}

/**
 * @brief 生成快捷方式HTML内容
 * 
 * 此函数生成快捷方式列表的HTML内容
 * 
 * @return std::wstring 快捷方式HTML内容
 */
std::wstring GenerateQuickShortcutsHtml()
{
    std::wstring shortcutsHtml;
    shortcutsHtml += L"<div style='font-weight: bold; margin-top: 20px; margin-bottom: 10px; color: #333;'>⚡ 快捷方式</div>";
    
    // 添加常用快捷方式
    std::vector<std::pair<std::wstring, std::wstring>> shortcuts = {
        {L"📊 计算器", L"js"},
        {L"📁 目录浏览", L"dir"},
        {L"⚙️ 设置", L"set"},
        {L"❓ 帮助", L"help"}
    };
    
    for (const auto& shortcut : shortcuts)
    {
        shortcutsHtml += L"<div class='dir-item shortcut' onclick='executeShortcut(\"" + shortcut.second + L"\")'>";
        shortcutsHtml += L"<span class='dir-icon'>⚡</span>";
        shortcutsHtml += L"<span class='dir-name'>" + shortcut.first + L"</span>";
        shortcutsHtml += L"</div>";
    }
    
    return shortcutsHtml;
}

/**
 * @brief 更新目录浏览模式的WebView2显示
 * 
 * 此函数从外部HTML文件读取模板，并动态生成目录浏览模式的HTML界面
 */
void UpdateDirModeWebView()
{
    g_currentViewMode = ViewMode::DIR_MODE;
    if (!g_webView)
    {
        LogToFile("UpdateDirModeWebView: WebView2 未初始化，无法显示目录浏览");
        return;
    }
    
    // 使用缓存
    if (!g_dirModeHtmlCached)
    {
        // 读取HTML模板文件
        g_cachedDirModeHtml = ReadHtmlTemplate(L"data/dir_mode_template.html");
        
        if (g_cachedDirModeHtml.empty())
        {
            LogToFile("UpdateDirModeWebView: 模板文件读取失败 (data/dir_mode_template.html)");
            std::wstring errorHtml = L"<html><body><h3 style='color:red;'>错误：无法加载目录模式模板文件 (data/dir_mode_template.html)</h3><p>请检查 data 目录下的模板文件是否存在。</p></body></html>";
            UpdateWebView2Content(errorHtml.c_str());
            return;
        }
        
        g_dirModeHtmlCached = true;
        LogToFile("UpdateDirModeWebView: 模板文件已缓存");
    }
    
    // 使用缓存的模板
    std::wstring htmlTemplate = g_cachedDirModeHtml;
    
    // 生成动态内容
    std::wstring drivesHtml = GenerateDrivesHtml();
    std::wstring commonPathsHtml = GenerateCommonPathsHtml();
    std::wstring expandedDirsHtml = GenerateExpandedDirsHtml();
    std::wstring quickShortcutsHtml = GenerateQuickShortcutsHtml();
    
    // 替换模板中的占位符
    size_t drivesPos = htmlTemplate.find(L"<!-- DRIVES_PLACEHOLDER -->");
    if (drivesPos != std::wstring::npos)
    {
        htmlTemplate.replace(drivesPos, wcslen(L"<!-- DRIVES_PLACEHOLDER -->"), drivesHtml);
    }
    
    size_t commonPathsPos = htmlTemplate.find(L"<!-- COMMON_PATHS_PLACEHOLDER -->");
    if (commonPathsPos != std::wstring::npos)
    {
        htmlTemplate.replace(commonPathsPos, wcslen(L"<!-- COMMON_PATHS_PLACEHOLDER -->"), commonPathsHtml);
    }
    
    size_t expandedDirsPos = htmlTemplate.find(L"<!-- EXPANDED_DIRS_PLACEHOLDER -->");
    if (expandedDirsPos != std::wstring::npos)
    {
        htmlTemplate.replace(expandedDirsPos, wcslen(L"<!-- EXPANDED_DIRS_PLACEHOLDER -->"), expandedDirsHtml);
    }
    
    size_t quickShortcutsPos = htmlTemplate.find(L"<!-- QUICK_SHORTCUTS_PLACEHOLDER -->");
    if (quickShortcutsPos != std::wstring::npos)
    {
        htmlTemplate.replace(quickShortcutsPos, wcslen(L"<!-- QUICK_SHORTCUTS_PLACEHOLDER -->"), quickShortcutsHtml);
    }
    
    // 更新WebView2内容
    UpdateWebView2Content(htmlTemplate.c_str());
}