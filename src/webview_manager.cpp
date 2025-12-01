#include "webview_manager.h"
#include "common.h"
#include "logger.h"
#include <vector>
#include <set>
#include <string>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <commctrl.h>

// HTML模板读取函数声明
std::wstring ReadHtmlTemplate(const std::wstring& filePath);

// WebView2相关全局变量定义
ComPtr<ICoreWebView2Environment> g_webViewEnvironment;
ComPtr<ICoreWebView2Controller> g_webViewController;
ComPtr<ICoreWebView2> g_webView;
HWND g_hWebView2 = nullptr;
bool g_settingsMenuMode = false;
std::set<std::wstring> g_expandedPaths;
std::wstring g_currentDirPath;

/**
 * 初始化WebView2环境
 * @param hwnd 父窗口句柄
 */
void InitializeWebView2(HWND hwnd)
{
    LogToFile("InitializeWebView2: 开始初始化 WebView2");
    
    // 使用 CreateCoreWebView2Environment 创建环境（简化版本，不使用选项）
    HRESULT hr = CreateCoreWebView2Environment(
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result))
                {
                    char errorMsg[256] = {0};
                    sprintf(errorMsg, "InitializeWebView2: 创建环境失败，错误代码: 0x%08X", result);
                    LogToFile(errorMsg);
                    
                    // WebView2初始化失败，显示基本用法界面
                    LogToFile("InitializeWebView2: 显示基本用法界面（WebView2初始化失败）");
                    ShowBasicUsage();
                    return result;
                }
                
                LogToFile("InitializeWebView2: WebView2 环境创建成功");
                
                // 创建 WebView2 控制器（使用占位窗口）
                HWND webViewParent = g_hWebView2 ? g_hWebView2 : hwnd;
                env->CreateCoreWebView2Controller(webViewParent, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (FAILED(result))
                        {
                            char errorMsg[256] = {0};
                            sprintf(errorMsg, "InitializeWebView2: 创建控制器失败，错误代码: 0x%08X", result);
                            LogToFile(errorMsg);
                            
                            // 控制器创建失败，显示基本用法界面
                            LogToFile("InitializeWebView2: 显示基本用法界面（控制器创建失败）");
                            ShowBasicUsage();
                            return result;
                        }
                        
                        LogToFile("InitializeWebView2: WebView2 控制器创建成功");
                        
                        // 保存控制器引用
                        g_webViewController = controller;
                        g_webViewController->AddRef();
                        
                        // 设置 WebView2 控制器的位置和大小（匹配占位窗口）
                        if (g_hWebView2)
                        {
                            RECT bounds;
                            if (GetClientRect(g_hWebView2, &bounds))
                            {
                                g_webViewController->put_Bounds(bounds);
                                char logMsg[200] = {0};
                                sprintf(logMsg, "InitializeWebView2: 设置 WebView2 位置和大小: (%d, %d, %d, %d)", 
                                        bounds.left, bounds.top, bounds.right, bounds.bottom);
                                LogToFile(logMsg);
                            }
                        }
                        
                        // 获取 WebView2 核心对象
                        g_webViewController->get_CoreWebView2(&g_webView);
                        if (g_webView)
                        {
                            LogToFile("InitializeWebView2: WebView2 核心对象获取成功");
                            
                            // 设置消息接收处理器
                            g_webView->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                    WCHAR* message = nullptr;
                                    args->TryGetWebMessageAsString(&message);
                                    if (message)
                                    {
                                        // 解析JSON消息
                                        std::wstring msgStr = message;
                                        CoTaskMemFree(message);
                                        
                                        // 简单的JSON解析（查找type字段）
                                        if (msgStr.find(L"\"type\":\"itemClick\"") != std::wstring::npos)
                                        {
                                            // 处理搜索结果点击
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                int index = _wtoi(indexStr.c_str());
                                                if (index >= 0 && index < (int)g_searchResults.size())
                                                {
                                                    ExecuteSelectedItem(index);
                                                }
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"itemDblClick\"") != std::wstring::npos)
                                        {
                                            // 处理搜索结果双击
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                int index = _wtoi(indexStr.c_str());
                                                if (index >= 0 && index < (int)g_searchResults.size())
                                                {
                                                    ExecuteSelectedItem(index);
                                                }
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"calcAction\"") != std::wstring::npos)
                                        {
                                            // 处理计算模式操作
                                            std::wstring action;
                                            int index = -1;
                                            
                                            size_t actionPos = msgStr.find(L"\"action\":\"");
                                            if (actionPos != std::wstring::npos)
                                            {
                                                size_t start = actionPos + 10;
                                                size_t end = msgStr.find(L"\"", start);
                                                action = msgStr.substr(start, end - start);
                                            }
                                            
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 处理操作
                                            if (action == L"copy")
                                            {
                                                if (index >= 0 && index < (int)g_calculationHistory.size())
                                                {
                                                    size_t actualIndex = g_calculationHistory.size() - 1 - index;
                                                    if (actualIndex < g_calculationHistory.size())
                                                    {
                                                        std::wstring text = g_calculationHistory[actualIndex].expression + L" = " + g_calculationHistory[actualIndex].result;
                                                        if (OpenClipboard(g_hMainWindow))
                                                        {
                                                            EmptyClipboard();
                                                            size_t byteCount = (text.length() + 1) * sizeof(wchar_t);
                                                            HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, byteCount);
                                                            if (hClipboardData)
                                                            {
                                                                LPVOID lpMem = GlobalLock(hClipboardData);
                                                                memcpy(lpMem, text.c_str(), byteCount);
                                                                GlobalUnlock(hClipboardData);
                                                                SetClipboardData(CF_UNICODETEXT, hClipboardData);
                                                            }
                                                            CloseClipboard();
                                                        }
                                                    }
                                                }
                                            }
                                            else if (action == L"delete")
                                            {
                                                if (g_calculationHistory.empty() || index < 0)
                                                {
                                                    MessageBoxW(g_hMainWindow, L"请先选择要删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                                                    return S_OK;
                                                }
                                                
                                                size_t actualIndex = g_calculationHistory.size() - 1 - index;
                                                if (actualIndex >= g_calculationHistory.size())
                                                {
                                                    MessageBoxW(g_hMainWindow, L"索引转换错误，无法删除", L"错误", MB_OK | MB_ICONERROR);
                                                    return S_OK;
                                                }
                                                
                                                g_calculationHistory.erase(g_calculationHistory.begin() + actualIndex);
                                                SaveCalculationHistory();
                                                DisplayCalculationHistory();
                                                UpdateCalculatorModeWebView();
                                            }
                                            else if (action == L"clearAll")
                                            {
                                                if (MessageBoxW(g_hMainWindow, L"确定要清空所有计算历史吗？", 
                                                    L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES)
                                                {
                                                    g_calculationHistory.clear();
                                                    SaveCalculationHistory();
                                                    DisplayCalculationHistory();
                                                    UpdateCalculatorModeWebView();
                                                }
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"settingsAction\"") != std::wstring::npos)
                                        {
                                            // 处理设置菜单操作
                                            int index = -1;
                                            
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 处理设置菜单项点击（需要跳过提示行）
                                            if (index >= 0)
                                            {
                                                INT_PTR actualIndex = index + GetHintRowCount();
                                                HandleSettingsMenuItemClick(actualIndex);
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"bookmarkDblClick\"") != std::wstring::npos)
                                        {
                                            // 处理网址收藏双击
                                            int index = -1;
                                            
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 获取要打开的网址
                                            const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                                            if (index >= 0 && index < (int)displayBookmarks.size())
                                            {
                                                ShellExecuteW(NULL, L"open", displayBookmarks[index].second.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                LogToFile("WebView2消息: 打开了网址收藏");
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"addBookmark\"") != std::wstring::npos)
                                        {
                                            // 处理添加网址请求
                                            LogToFile("WebView2消息: 收到添加网址请求");
                                            
                                            // 显示HTML添加网址对话框
                                            ShowHtmlAddBookmarkDialog();
                                        }
                                        else if (msgStr.find(L"\"type\":\"addBookmarkFromDialog\"") != std::wstring::npos)
                                        {
                                            // 处理从HTML对话框添加网址
                                            LogToFile("WebView2消息: 收到从HTML对话框添加网址请求");
                                            
                                            std::wstring name, url;
                                            
                                            // 解析名称
                                            size_t namePos = msgStr.find(L"\"name\":\"");
                                            if (namePos != std::wstring::npos)
                                            {
                                                namePos += 7;  // 跳过 "name":
                                                size_t nameEnd = msgStr.find(L"\"", namePos);
                                                if (nameEnd != std::wstring::npos)
                                                {
                                                    name = msgStr.substr(namePos, nameEnd - namePos);
                                                }
                                            }
                                            
                                            // 解析URL
                                            size_t urlPos = msgStr.find(L"\"url\":\"");
                                            if (urlPos != std::wstring::npos)
                                            {
                                                urlPos += 6;  // 跳过 "url":
                                                size_t urlEnd = msgStr.find(L"\"", urlPos);
                                                if (urlEnd != std::wstring::npos)
                                                {
                                                    url = msgStr.substr(urlPos, urlEnd - urlPos);
                                                }
                                            }
                                            
                                            // 验证输入
                                            if (name.empty() || url.empty())
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框添加网址失败 - 名称或URL为空");
                                                return S_OK;
                                            }
                                            
                                            // 验证URL格式
                                            if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && 
                                                url.find(L"ftp://") != 0 && url.find(L"file://") != 0)
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框添加网址失败 - URL格式无效");
                                                return S_OK;
                                            }
                                            
                                            // 检查是否已存在相同URL
                                            for (const auto& bookmark : g_bookmarks)
                                            {
                                                if (bookmark.second == url)
                                                {
                                                    LogToFile("WebView2消息: 从HTML对话框添加网址失败 - 网址已存在");
                                                    return S_OK;
                                                }
                                            }
                                            
                                            // 添加网址收藏
                                            g_bookmarks.push_back(std::make_pair(name, url));
                                            
                                            // 保存收藏列表
                                            SaveBookmarks();
                                            
                                            // 更新WebView显示
                                            if (g_bookmarkMode)
                                            {
                                                UpdateBookmarkModeWebView();
                                            }
                                            else
                                            {
                                                // 返回到原来的界面
                                                UpdateHelpInfoWebView();
                                            }
                                            
                                            std::string logMsg = "WebView2消息: 从HTML对话框成功添加网址收藏 - " + std::string(name.begin(), name.end()) + " -> " + std::string(url.begin(), url.end());
                                            LogToFile(logMsg.c_str());
                                        }
                                        else if (msgStr.find(L"\"type\":\"closeBookmarkDialog\"") != std::wstring::npos)
                                        {
                                            // 处理关闭HTML对话框
                                            LogToFile("WebView2消息: 收到关闭HTML对话框请求");
                                            
                                            // 返回到原来的界面
                                            if (g_bookmarkMode)
                                            {
                                                UpdateBookmarkModeWebView();
                                            }
                                            else
                                            {
                                                UpdateHelpInfoWebView();
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"dirExpand\"") != std::wstring::npos)
                                        {
                                            // 处理目录展开
                                            std::wstring path;
                                            
                                            size_t pathPos = msgStr.find(L"\"path\":\"");
                                            if (pathPos != std::wstring::npos)
                                            {
                                                pathPos += 8;  // 跳过 "path":"
                                                size_t pathEnd = msgStr.find(L"\"", pathPos);
                                                if (pathEnd != std::wstring::npos)
                                                {
                                                    path = msgStr.substr(pathPos, pathEnd - pathPos);
                                                    
                                                    // 处理转义的路径
                                                    size_t pos = 0;
                                                    while ((pos = path.find(L"\\\\", pos)) != std::wstring::npos)
                                                    {
                                                        path.replace(pos, 2, L"\\");
                                                        pos += 1;
                                                    }
                                                    
                                                    // 切换展开状态
                                                    if (g_expandedPaths.find(path) != g_expandedPaths.end())
                                                    {
                                                        g_expandedPaths.erase(path);
                                                        if (g_currentDirPath == path)
                                                        {
                                                            g_currentDirPath.clear();
                                                        }
                                                    }
                                                    else
                                                    {
                                                        g_expandedPaths.insert(path);
                                                        g_currentDirPath = path;
                                                    }
                                                    
                                                    UpdateDirModeWebView();
                                                    LogToFile("WebView2消息: 展开/收起目录");
                                                }
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"dirOpen\"") != std::wstring::npos)
                                        {
                                            // 处理文件/目录打开
                                            std::wstring path;
                                            bool isDir = false;
                                            
                                            size_t pathPos = msgStr.find(L"\"path\":\"");
                                            if (pathPos != std::wstring::npos)
                                            {
                                                pathPos += 8;  // 跳过 "path":"
                                                size_t pathEnd = msgStr.find(L"\"", pathPos);
                                                if (pathEnd != std::wstring::npos)
                                                {
                                                    path = msgStr.substr(pathPos, pathEnd - pathPos);
                                                    
                                                    // 处理转义的路径
                                                    size_t pos = 0;
                                                    while ((pos = path.find(L"\\\\", pos)) != std::wstring::npos)
                                                    {
                                                        path.replace(pos, 2, L"\\");
                                                        pos += 1;
                                                    }
                                                    
                                                    // 检查是否为目录
                                                    size_t isDirPos = msgStr.find(L"\"isDir\":");
                                                    if (isDirPos != std::wstring::npos)
                                                    {
                                                        size_t isDirStart = msgStr.find(L":", isDirPos) + 1;
                                                        size_t isDirEnd = msgStr.find(L",", isDirStart);
                                                        if (isDirEnd == std::wstring::npos) isDirEnd = msgStr.find(L"}", isDirStart);
                                                        std::wstring isDirStr = msgStr.substr(isDirStart, isDirEnd - isDirStart);
                                                        isDir = (isDirStr.find(L"true") != std::wstring::npos);
                                                    }
                                                    
                                                    if (isDir)
                                                    {
                                                        // 展开目录
                                                        g_expandedPaths.insert(path);
                                                        g_currentDirPath = path;
                                                        UpdateDirModeWebView();
                                                        LogToFile("WebView2消息: 打开目录");
                                                    }
                                                    else
                                                    {
                                                        // 打开文件
                                                        ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                        LogToFile("WebView2消息: 打开文件");
                                                    }
                                                }
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"openShortcut\"") != std::wstring::npos)
                                        {
                                            // 处理快捷方式打开
                                            std::wstring command;
                                            
                                            size_t commandPos = msgStr.find(L"\"command\":\"");
                                            if (commandPos != std::wstring::npos)
                                            {
                                                commandPos += 11;  // 跳过 "command":"
                                                size_t commandEnd = msgStr.find(L"\"", commandPos);
                                                if (commandEnd != std::wstring::npos)
                                                {
                                                    command = msgStr.substr(commandPos, commandEnd - commandPos);
                                                    
                                                    // 处理转义的路径
                                                    size_t pos = 0;
                                                    while ((pos = command.find(L"\\\\", pos)) != std::wstring::npos)
                                                    {
                                                        command.replace(pos, 2, L"\\");
                                                        pos += 1;
                                                    }
                                                    
                                                    // 执行快捷方式命令
                                                    ShellExecuteW(NULL, L"open", command.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                                    
                                                    std::wstring logMsg = L"WebView2消息: 打开快捷方式 - " + command;
                                                    LogToFile(std::string(logMsg.begin(), logMsg.end()).c_str());
                                                }
                                            }
                                        }
                                    }
                                    return S_OK;
                                }).Get(), nullptr);
                            
                            // WebView2 初始化完成，根据当前状态显示内容
                            LogToFile("InitializeWebView2: WebView2 初始化完成，更新显示内容");
                            
                            // 根据当前模式显示相应内容
                            if (g_settingsMenuMode)
                            {
                                UpdateSettingsMenuWebView();
                            }
                            else if (g_calculatorMode)
                            {
                                UpdateCalculatorModeWebView();
                            }
                            else if (g_bookmarkMode)
                            {
                                // 网址收藏模式显示所有网址
                                UpdateBookmarkModeWebView();
                            }
                            else if (g_dirMode)
                            {
                                // 目录浏览模式显示目录浏览界面
                                UpdateDirModeWebView();
                            }
                            else if (g_shortcuts.empty())
                            {
                                // 如果快捷方式还未初始化，显示帮助信息
                                UpdateHelpInfoWebView();
                            }
                            else
                            {
                                // 显示搜索结果
                                SearchAndDisplayResults(g_currentSearch);
                            }
                        }
                        
                        return S_OK;
                    }).Get());
                
                return S_OK;
            }).Get());
    
    if (FAILED(hr))
    {
        char errorMsg[256] = {0};
        sprintf(errorMsg, "InitializeWebView2: 创建环境失败，错误代码: 0x%08X", hr);
        LogToFile(errorMsg);
        
        // 显示基本用法界面
        LogToFile("InitializeWebView2: 显示基本用法界面（环境创建失败）");
        ShowBasicUsage();
    }
}

// 显示HTML添加网址对话框
void ShowHtmlAddBookmarkDialog()
{
    LogToFile("ShowHtmlAddBookmarkDialog: 显示HTML添加网址对话框");
    
    // 读取HTML对话框模板
    std::wstring dialogHtml = ReadHtmlTemplate(L"data/add_bookmark_dialog.html");
    if (dialogHtml.empty())
    {
        LogToFile("ShowHtmlAddBookmarkDialog: 无法读取HTML对话框模板，使用简单对话框");
        ShowSimpleAddBookmarkDialog();
        return;
    }
    
    // 更新WebView2内容为对话框
    UpdateWebView2Content(dialogHtml.c_str());
}

// 显示简单添加网址对话框
void ShowSimpleAddBookmarkDialog()
{
    LogToFile("ShowSimpleAddBookmarkDialog: 显示添加网址对话框");
    
    // 使用简单的输入框来获取用户输入
    WCHAR name[256] = {0};
    WCHAR url[1024] = {0};
    
    // 获取名称 - 使用简单的输入框
    if (!GetSimpleInput(L"添加网址收藏", L"请输入网址名称:", L"", name, 255))
    {
        LogToFile("ShowSimpleAddBookmarkDialog: 用户取消输入名称");
        return;
    }
    
    // 检查名称是否为空
    if (wcslen(name) == 0)
    {
        MessageBoxW(g_hMainWindow, L"网址名称不能为空", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowSimpleAddBookmarkDialog: 名称为空");
        return;
    }
    
    // 获取URL - 使用简单的输入框
    if (!GetSimpleInput(L"添加网址收藏", L"请输入网址URL:", L"https://", url, 1023))
    {
        LogToFile("ShowSimpleAddBookmarkDialog: 用户取消输入URL");
        return;
    }
    
    // 检查URL是否为空
    if (wcslen(url) == 0)
    {
        MessageBoxW(g_hMainWindow, L"网址URL不能为空", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowSimpleAddBookmarkDialog: URL为空");
        return;
    }
    
    // 验证URL格式
    std::wstring urlStr(url);
    if (urlStr.find(L"http://") != 0 && urlStr.find(L"https://") != 0 && 
        urlStr.find(L"ftp://") != 0 && urlStr.find(L"file://") != 0)
    {
        MessageBoxW(g_hMainWindow, L"请输入有效的URL（以http://、https://、ftp://或file://开头）", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowSimpleAddBookmarkDialog: URL格式无效");
        return;
    }
    
    // 检查是否已存在相同URL
    for (const auto& bookmark : g_bookmarks)
    {
        if (bookmark.second == urlStr)
        {
            MessageBoxW(g_hMainWindow, L"该网址已存在于收藏中", L"添加失败", MB_OK | MB_ICONWARNING);
            LogToFile("ShowSimpleAddBookmarkDialog: 网址已存在");
            return;
        }
    }
    
    // 添加网址收藏
    g_bookmarks.push_back(std::make_pair(std::wstring(name), urlStr));
    
    // 保存收藏列表
    SaveBookmarks();
    
    // 更新WebView显示
    if (g_bookmarkMode)
    {
        UpdateBookmarkModeWebView();
    }
    
    std::string logMsg = "ShowSimpleAddBookmarkDialog: 成功添加网址收藏 - " + std::string(name, name + wcslen(name)) + " -> " + std::string(url, url + wcslen(url));
    LogToFile(logMsg.c_str());
    
    MessageBoxW(g_hMainWindow, L"网址收藏添加成功！", L"成功", MB_OK | MB_ICONINFORMATION);
}

// 简单的输入框函数
BOOL GetSimpleInput(LPCWSTR lpCaption, LPCWSTR lpPrompt, LPCWSTR lpDefault, LPWSTR lpResult, int nResultSize)
{
    // 使用简单的输入方式：直接使用编辑控件
    // 创建一个简单的编辑窗口
    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpDefault, 
                                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                10, 30, 300, 25, 
                                g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hEdit)
    {
        return FALSE;
    }
    
    // 创建一个简单的对话框窗口
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lpCaption,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                               100, 100, 350, 120,
                               g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hDlg)
    {
        DestroyWindow(hEdit);
        return FALSE;
    }
    
    // 设置父窗口
    SetParent(hEdit, hDlg);
    
    // 创建确定按钮
    HWND hOkBtn = CreateWindowExW(0, L"BUTTON", L"确定", 
                                 WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                 200, 65, 60, 25, 
                                 hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
    
    // 创建取消按钮
    HWND hCancelBtn = CreateWindowExW(0, L"BUTTON", L"取消", 
                                     WS_VISIBLE | WS_CHILD,
                                     270, 65, 60, 25, 
                                     hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
    
    // 创建提示文本
    HWND hPrompt = CreateWindowExW(0, L"STATIC", lpPrompt,
                                  WS_VISIBLE | WS_CHILD,
                                  10, 10, 300, 20,
                                  hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 显示窗口
    ShowWindow(hDlg, SW_SHOW);
    SetFocus(hEdit);
    
    // 简单的消息循环
    MSG msg;
    BOOL bResult = FALSE;
    
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_COMMAND)
        {
            if (LOWORD(msg.wParam) == IDOK)
            {
                // 获取输入文本
                GetWindowTextW(hEdit, lpResult, nResultSize);
                bResult = TRUE;
                break;
            }
            else if (LOWORD(msg.wParam) == IDCANCEL)
            {
                bResult = FALSE;
                break;
            }
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理资源
    DestroyWindow(hDlg);
    DestroyWindow(hEdit);
    
    return bResult;
}