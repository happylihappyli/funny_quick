#include "webview_manager.h"
#include "common.h"
#include "calculator.h"  // 计算器功能定义
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
                    sprintf(errorMsg, "InitializeWebView2: 创建环境失败，错误代码: 0x%08lX", result);
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
                            sprintf(errorMsg, "InitializeWebView2: 创建控制器失败，错误代码: 0x%08lX", result);
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
                                        else if (msgStr.find(L"\"type\":\"bookmarkClick\"") != std::wstring::npos)
                                        {
                                            // 处理网址收藏单击（回车键应该调用这个）
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
                                                LogToFile("WebView2消息: 通过单击打开了网址收藏");
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
                                                LogToFile("WebView2消息: 通过双击打开了网址收藏");
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"addBookmark\"") != std::wstring::npos)
                                        {
                                            // 处理添加网址请求
                                            LogToFile("WebView2消息: 收到添加网址请求");
                                            
                                            // 显示HTML添加网址对话框
                                            ShowHtmlAddBookmarkDialog();
                                        }
                                        else if (msgStr.find(L"\"type\":\"editBookmark\"") != std::wstring::npos)
                                        {
                                            // 处理编辑网址请求
                                            LogToFile("WebView2消息: 收到编辑网址请求");
                                            
                                            int index = -1;
                                            
                                            // 解析索引
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 获取要显示的书签列表（搜索结果或全部书签）
                                            const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                                            
                                            if (index >= 0 && index < (int)displayBookmarks.size())
                                            {
                                                // 显示HTML编辑对话框
                                                ShowHtmlEditBookmarkDialog(index);
                                            }
                                            else
                                            {
                                                LogToFile("WebView2消息: 编辑网址失败 - 索引无效");
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"deleteBookmark\"") != std::wstring::npos)
                                        {
                                            // 处理删除网址请求
                                            LogToFile("WebView2消息: 收到删除网址请求");
                                            
                                            int index = -1;
                                            
                                            // 解析索引
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 获取要显示的书签列表（搜索结果或全部书签）
                                            const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                                            
                                            if (index >= 0 && index < (int)displayBookmarks.size())
                                            {
                                                // 显示确认对话框
                                                std::wstring confirmMsg = L"确定要删除网址收藏 \"" + displayBookmarks[index].first + L"\" 吗？";
                                                if (MessageBoxW(g_hMainWindow, confirmMsg.c_str(), L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES)
                                                {
                                                    // 删除网址收藏
                                                    DeleteBookmarkFromDisplayList(index);
                                                    
                                                    // 更新WebView显示
                                                    UpdateBookmarkModeWebView();
                                                    
                                                    LogToFile("WebView2消息: 成功删除网址收藏");
                                                }
                                            }
                                            else
                                            {
                                                LogToFile("WebView2消息: 删除网址失败 - 索引无效");
                                            }
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
                                        else if (msgStr.find(L"\"type\":\"getBookmarkData\"") != std::wstring::npos)
                                        {
                                            // 处理获取书签数据请求（用于HTML编辑对话框）
                                            LogToFile("WebView2消息: 收到获取书签数据请求");
                                            
                                            int index = -1;
                                            
                                            // 解析索引
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 获取要显示的书签列表（搜索结果或全部书签）
                                            const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                                            
                                            if (index >= 0 && index < (int)displayBookmarks.size())
                                            {
                                                // 构建书签数据JSON（符合JavaScript期望的格式）
                                                std::wstringstream bookmarkData;
                                                bookmarkData << L"{\"type\":\"bookmarkData\",\"bookmark\":{" 
                                                             << L"\"index\":" << index 
                                                             << L",\"name\":\"" << displayBookmarks[index].first 
                                                             << L"\",\"url\":\"" << displayBookmarks[index].second << L"\"}}";
                                                
                                                // 发送书签数据到WebView
                                                if (g_webViewController && g_webView)
                                                {
                                                    g_webView->PostWebMessageAsJson(bookmarkData.str().c_str());
                                                    LogToFile("WebView2消息: 成功发送书签数据");
                                                }
                                            }
                                            else
                                            {
                                                LogToFile("WebView2消息: 获取书签数据失败 - 索引无效");
                                            }
                                        }
                                        else if (msgStr.find(L"\"type\":\"editBookmarkFromDialog\"") != std::wstring::npos)
                                        {
                                            // 处理从HTML对话框编辑网址
                                            LogToFile("WebView2消息: 收到从HTML对话框编辑网址请求");
                                            
                                            int index = -1;
                                            std::wstring name, url;
                                            
                                            // 解析索引
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
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
                                            if (index < 0 || name.empty() || url.empty())
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框编辑网址失败 - 索引、名称或URL无效");
                                                return S_OK;
                                            }
                                            
                                            // 验证URL格式
                                            if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && 
                                                url.find(L"ftp://") != 0 && url.find(L"file://") != 0)
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框编辑网址失败 - URL格式无效");
                                                return S_OK;
                                            }
                                            
                                            // 获取要显示的书签列表（搜索结果或全部书签）
                                            const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                                            
                                            if (index >= 0 && index < (int)displayBookmarks.size())
                                            {
                                                // 更新网址收藏
                                                g_bookmarks[index] = std::make_pair(name, url);
                                                
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
                                                
                                                std::string logMsg = "WebView2消息: 从HTML对话框成功编辑网址收藏 - " + std::string(name.begin(), name.end()) + " -> " + std::string(url.begin(), url.end());
                                                LogToFile(logMsg.c_str());
                                            }
                                            else
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框编辑网址失败 - 索引无效");
                                            }
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
                                        else if (msgStr.find(L"\"type\":\"deleteBookmarkFromDialog\"") != std::wstring::npos)
                                        {
                                            // 处理从HTML对话框删除网址
                                            LogToFile("WebView2消息: 收到从HTML对话框删除网址请求");
                                            
                                            int index = -1;
                                            
                                            // 解析索引
                                            size_t indexPos = msgStr.find(L"\"index\":");
                                            if (indexPos != std::wstring::npos)
                                            {
                                                size_t start = msgStr.find(L":", indexPos) + 1;
                                                size_t end = msgStr.find(L",", start);
                                                if (end == std::wstring::npos) end = msgStr.find(L"}", start);
                                                std::wstring indexStr = msgStr.substr(start, end - start);
                                                index = _wtoi(indexStr.c_str());
                                            }
                                            
                                            // 验证索引
                                            if (index < 0)
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框删除网址失败 - 索引无效");
                                                return S_OK;
                                            }
                                            
                                            // 获取要显示的书签列表（搜索结果或全部书签）
                                            const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
                                            
                                            if (index >= 0 && index < (int)displayBookmarks.size())
                                            {
                                                // 删除网址收藏
                                                g_bookmarks.erase(g_bookmarks.begin() + index);
                                                
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
                                                
                                                std::string logMsg = "WebView2消息: 从HTML对话框成功删除网址收藏 - 索引: " + std::to_string(index);
                                                LogToFile(logMsg.c_str());
                                            }
                                            else
                                            {
                                                LogToFile("WebView2消息: 从HTML对话框删除网址失败 - 索引无效");
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
        sprintf(errorMsg, "InitializeWebView2: 创建环境失败，错误代码: 0x%08lX", hr);
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
    
    // 更新WebView2显示
    UpdateBookmarkModeWebView();
    
    LogToFile("ShowSimpleAddBookmarkDialog: 网址添加成功，WebView2已更新");
}

/**
 * @brief 显示编辑网址对话框
 * 
 * 此函数显示编辑网址的对话框，允许用户修改选中的网址
 * 
 * @param index 要编辑的网址索引
 */
void ShowEditBookmarkDialog(int index)
{
    LogToFile("ShowEditBookmarkDialog: 显示编辑网址对话框");
    
    // 检查索引是否有效
    if (index < 0 || index >= (int)g_bookmarks.size())
    {
        MessageBoxW(g_hMainWindow, L"无效的网址索引", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditBookmarkDialog: 无效索引");
        return;
    }
    
    // 获取当前网址信息
    const auto& bookmark = g_bookmarks[index];
    std::wstring currentName = bookmark.first;
    std::wstring currentUrl = bookmark.second;
    
    // 使用简单的输入框来获取用户输入
    WCHAR name[256] = {0};
    WCHAR url[1024] = {0};
    
    // 复制当前值到缓冲区
    wcscpy_s(name, 255, currentName.c_str());
    wcscpy_s(url, 1023, currentUrl.c_str());
    
    // 获取名称 - 使用简单的输入框
    if (!GetSimpleInput(L"编辑网址收藏", L"请输入新的网址名称:", name, name, 255))
    {
        LogToFile("ShowEditBookmarkDialog: 用户取消输入名称");
        return;
    }
    
    // 检查名称是否为空
    if (wcslen(name) == 0)
    {
        MessageBoxW(g_hMainWindow, L"网址名称不能为空", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditBookmarkDialog: 名称为空");
        return;
    }
    
    // 获取URL - 使用简单的输入框
    if (!GetSimpleInput(L"编辑网址收藏", L"请输入新的网址URL:", url, url, 1023))
    {
        LogToFile("ShowEditBookmarkDialog: 用户取消输入URL");
        return;
    }
    
    // 检查URL是否为空
    if (wcslen(url) == 0)
    {
        MessageBoxW(g_hMainWindow, L"网址URL不能为空", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditBookmarkDialog: URL为空");
        return;
    }
    
    // 验证URL格式
    std::wstring urlStr(url);
    if (urlStr.find(L"http://") != 0 && urlStr.find(L"https://") != 0 && 
        urlStr.find(L"ftp://") != 0 && urlStr.find(L"file://") != 0)
    {
        MessageBoxW(g_hMainWindow, L"请输入有效的URL（以http://、https://、ftp://或file://开头）", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditBookmarkDialog: URL格式无效");
        return;
    }
    
    // 检查是否已存在相同URL（排除当前编辑的网址）
    for (int i = 0; i < (int)g_bookmarks.size(); i++)
    {
        if (i != index && g_bookmarks[i].second == urlStr)
        {
            MessageBoxW(g_hMainWindow, L"该网址已存在于收藏中", L"编辑失败", MB_OK | MB_ICONWARNING);
            LogToFile("ShowEditBookmarkDialog: 网址已存在");
            return;
        }
    }
    
    // 更新网址收藏
    g_bookmarks[index].first = std::wstring(name);
    g_bookmarks[index].second = urlStr;
    
    // 保存收藏列表
    SaveBookmarks();
    
    // 更新WebView2显示
    UpdateBookmarkModeWebView();
    
    LogToFile("ShowEditBookmarkDialog: 网址编辑成功，WebView2已更新");
}

/**
 * @brief 从显示列表中删除网址
 * 
 * 此函数从显示列表中删除指定索引的网址
 * 
 * @param index 要删除的网址索引
 */
void DeleteBookmarkFromDisplayList(int index)
{
    LogToFile("DeleteBookmarkFromDisplayList: 删除网址");
    
    // 检查索引是否有效
    if (index < 0 || index >= (int)g_bookmarks.size())
    {
        MessageBoxW(g_hMainWindow, L"无效的网址索引", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("DeleteBookmarkFromDisplayList: 无效索引");
        return;
    }
    
    // 获取要删除的网址信息用于确认
    const auto& bookmark = g_bookmarks[index];
    std::wstring name = bookmark.first;
    std::wstring url = bookmark.second;
    
    // 确认删除
    std::wstring confirmMsg = L"确定要删除网址 \"" + name + L"\" (" + url + L") 吗？";
    if (MessageBoxW(g_hMainWindow, confirmMsg.c_str(), L"确认删除", MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        LogToFile("DeleteBookmarkFromDisplayList: 用户取消删除");
        return;
    }
    
    // 删除网址
    g_bookmarks.erase(g_bookmarks.begin() + index);
    
    // 保存收藏列表
    SaveBookmarks();
    
    // 更新WebView2显示
    UpdateBookmarkModeWebView();
    
    LogToFile("DeleteBookmarkFromDisplayList: 网址删除成功，WebView2已更新");
}

/**
 * @brief 显示HTML编辑网址对话框
 * 
 * 此函数显示HTML编辑网址的对话框，提供更好的用户体验
 * 
 * @param index 要编辑的网址索引
 */
void ShowHtmlEditBookmarkDialog(int index)
{
    LogToFile("ShowHtmlEditBookmarkDialog: 显示HTML编辑网址对话框");
    
    // 检查索引是否有效
    if (index < 0 || index >= (int)g_bookmarks.size())
    {
        MessageBoxW(g_hMainWindow, L"无效的网址索引", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowHtmlEditBookmarkDialog: 无效索引");
        return;
    }
    
    // 读取HTML对话框模板
    std::wstring dialogHtml = ReadHtmlTemplate(L"data/edit_bookmark_dialog.html");
    if (dialogHtml.empty())
    {
        LogToFile("ShowHtmlEditBookmarkDialog: 无法读取HTML对话框模板，使用简单对话框");
        ShowEditBookmarkDialog(index);
        return;
    }
    
    // 获取书签数据
    const auto& bookmark = g_bookmarks[index];
    
    // 在HTML中添加初始化脚本
    size_t bodyPos = dialogHtml.find(L"<body>");
    if (bodyPos != std::wstring::npos)
    {
        // 在<body>标签后添加脚本以初始化对话框数据
        std::wstring script = L"<script>\n";
        script += L"window.onload = function() {\n";
        script += L"    initializeDialog(" + std::to_wstring(index) + L", '" + bookmark.first + L"', '" + bookmark.second + L"');\n";
        script += L"};\n";
        script += L"</script>\n";
        dialogHtml.insert(bodyPos + 6, script);
    }
    
    // 更新WebView2内容为对话框
    UpdateWebView2Content(dialogHtml.c_str());
    
    LogToFile("ShowHtmlEditBookmarkDialog: HTML编辑对话框已显示");
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

/**
 * @brief 更新书签模式的WebView2显示
 * 
 * 此函数用于在书签模式下更新WebView2的内容，
 * 显示当前的书签列表和搜索结果，并提供添加、编辑、删除操作
 */
void UpdateBookmarkModeWebView()
{
    LogToFile("UpdateBookmarkModeWebView: 开始更新书签模式WebView显示");
    
    // 检查WebView2是否已初始化
    if (!g_webView)
    {
        LogToFile("UpdateBookmarkModeWebView: WebView2未初始化，无法更新显示");
        return;
    }
    
    // 创建HTML内容
    std::wstring htmlContent;
    
    // HTML头部
    htmlContent += L"<!DOCTYPE html>\n";
    htmlContent += L"<html>\n";
    htmlContent += L"<head>\n";
    htmlContent += L"<meta charset=\"UTF-8\">\n";
    htmlContent += L"<title>网址收藏管理</title>\n";
    htmlContent += L"<style>\n";
    htmlContent += L"body { font-family: 'Microsoft YaHei UI', sans-serif; margin: 0; padding: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #f5f8ff; }\n";
    htmlContent += L".header { background: linear-gradient(90deg, #4a90e2, #357abd); color: white; padding: 15px; position: relative; border-radius: 0 0 10px 10px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); }\n";
    htmlContent += L".header h2 { margin: 0; text-align: center; }\n";
    htmlContent += L".action-buttons { position: absolute; right: 15px; top: 15px; display: flex; gap: 10px; }\n";
    htmlContent += L".action-btn { background: linear-gradient(135deg, #4CAF50, #45a049); color: white; border: none; padding: 8px 16px; border-radius: 4px; cursor: pointer; font-size: 0.9em; box-shadow: 0 2px 4px rgba(0,0,0,0.2); }\n";
    htmlContent += L".action-btn:hover { background: linear-gradient(135deg, #45a049, #3d8b40); }\n";
    htmlContent += L".action-btn.delete { background: linear-gradient(135deg, #e74c3c, #c0392b); }\n";
    htmlContent += L".action-btn.delete:hover { background: linear-gradient(135deg, #c0392b, #a93226); }\n";
    htmlContent += L".bookmark-list { padding: 20px; }\n";
    htmlContent += L".bookmark-item { background: rgba(255,255,255,0.95); margin: 10px 0; padding: 15px; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); position: relative; border-left: 4px solid #4a90e2; }\n";
    htmlContent += L".bookmark-item:hover { background: rgba(255,255,255,0.98); transform: translateY(-1px); box-shadow: 0 4px 8px rgba(0,0,0,0.15); }\n";
    htmlContent += L".bookmark-item.search-result { border-left: 4px solid #4a90e2; background: rgba(255,255,255,0.95); }\n";
    htmlContent += L".bookmark-item.search-result:hover { background: rgba(255,255,255,0.98); }\n";
    htmlContent += L".bookmark-icon { display: inline-block; width: 20px; height: 20px; margin-right: 8px; vertical-align: middle; background: linear-gradient(135deg, #4a90e2, #357abd); border-radius: 50%; position: relative; }\n";
    htmlContent += L".bookmark-icon::before { content: '🔗'; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); font-size: 12px; }\n";
    htmlContent += L".bookmark-icon.search-result { background: linear-gradient(135deg, #4a90e2, #357abd); }\n";
    htmlContent += L".bookmark-icon.search-result::before { content: '🔗'; }\n";
    htmlContent += L".bookmark-name { font-weight: bold; color: #2c3e50; margin-bottom: 5px; display: flex; align-items: center; }\n";
    htmlContent += L".bookmark-url { color: #666; font-size: 0.9em; word-break: break-all; margin-left: 28px; }\n";
    htmlContent += L".bookmark-actions { position: absolute; right: 15px; top: 15px; display: flex; gap: 5px; }\n";
    htmlContent += L".bookmark-btn { background: linear-gradient(135deg, #95a5a6, #7f8c8d); color: white; border: none; padding: 4px 8px; border-radius: 3px; cursor: pointer; font-size: 0.8em; }\n";
    htmlContent += L".bookmark-btn:hover { background: linear-gradient(135deg, #7f8c8d, #6c7b7d); }\n";
    htmlContent += L".bookmark-btn.edit { background: linear-gradient(135deg, #f39c12, #d35400); }\n";
    htmlContent += L".bookmark-btn.edit:hover { background: linear-gradient(135deg, #d35400, #b34700); }\n";
    htmlContent += L".bookmark-btn.delete { background: linear-gradient(135deg, #e74c3c, #c0392b); }\n";
    htmlContent += L".bookmark-btn.delete:hover { background: linear-gradient(135deg, #c0392b, #a93226); }\n";
    htmlContent += L".empty-state { text-align: center; padding: 40px; color: rgba(255,255,255,0.8); }\n";
    htmlContent += L".search-info { background: rgba(255,255,255,0.15); border-left: 4px solid #4a90e2; padding: 10px 15px; margin: 10px 0; border-radius: 5px; font-size: 0.9em; color: rgba(255,255,255,0.9); }\n";
    htmlContent += L"</style>\n";
    htmlContent += L"</head>\n";
    htmlContent += L"<body>\n";
    
    // 头部
    htmlContent += L"<div class=\"header\">\n";
    htmlContent += L"<h2>网址收藏管理</h2>\n";
    htmlContent += L"<div class=\"action-buttons\">\n";
    htmlContent += L"<button class=\"action-btn\" onclick=\"window.chrome.webview.postMessage('{\\\"type\\\":\\\"addBookmark\\\"}');\">添加网址</button>\n";
    htmlContent += L"</div>\n";
    htmlContent += L"</div>\n";
    
    // 书签列表
    htmlContent += L"<div class=\"bookmark-list\">\n";
    
    // 获取要显示的书签列表（搜索结果或全部书签）
    const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
    bool isSearchResult = !g_bookmarkSearchResults.empty();
    
    if (displayBookmarks.empty())
    {
        // 空状态
        htmlContent += L"<div class=\"empty-state\">\n";
        if (isSearchResult)
        {
            htmlContent += L"<h3>未找到匹配的网址收藏</h3>\n";
            htmlContent += L"<p>请尝试其他搜索关键词</p>\n";
        }
        else
        {
            htmlContent += L"<h3>暂无网址收藏</h3>\n";
            htmlContent += L"<p>点击右上角的添加按钮来添加第一个网址收藏</p>\n";
        }
        htmlContent += L"</div>\n";
    }
    else
    {
        // 如果是搜索结果，显示搜索信息
        if (isSearchResult)
        {
            htmlContent += L"<div class=\"search-info\">\n";
            htmlContent += L"<strong>🔍 搜索结果</strong> - 找到 ";
            htmlContent += std::to_wstring(displayBookmarks.size());
            htmlContent += L" 个匹配的网址收藏\n";
            htmlContent += L"</div>\n";
        }
        
        // 显示书签列表
        for (size_t i = 0; i < displayBookmarks.size(); i++)
        {
            const auto& bookmark = displayBookmarks[i];
            
            // 根据是否为搜索结果设置不同的样式类
            std::wstring itemClass = isSearchResult ? L"bookmark-item search-result" : L"bookmark-item";
            std::wstring iconClass = isSearchResult ? L"bookmark-icon search-result" : L"bookmark-icon";
            
            htmlContent += L"<div class=\"";
            htmlContent += itemClass;
            htmlContent += L"\">\n";
            htmlContent += L"<div class=\"bookmark-actions\">\n";
            htmlContent += L"<button class=\"bookmark-btn edit\" onclick=\"window.chrome.webview.postMessage('{\\\"type\\\":\\\"editBookmark\\\",\\\"index\\\":";
            htmlContent += std::to_wstring(i);
            htmlContent += L"}');\">编辑</button>\n";
            htmlContent += L"<button class=\"bookmark-btn delete\" onclick=\"window.chrome.webview.postMessage('{\\\"type\\\":\\\"deleteBookmark\\\",\\\"index\\\":";
            htmlContent += std::to_wstring(i);
            htmlContent += L"}');\">删除</button>\n";
            htmlContent += L"</div>\n";
            htmlContent += L"<div class=\"bookmark-name\" onclick=\"window.chrome.webview.postMessage('{\\\"type\\\":\\\"bookmarkClick\\\",\\\"index\\\":";
            htmlContent += std::to_wstring(i);
            htmlContent += L"}');\" style=\"cursor: pointer;\">\n";
            htmlContent += L"<span class=\"";
            htmlContent += iconClass;
            htmlContent += L"\"></span>";
            htmlContent += bookmark.first;
            htmlContent += L"</div>\n";
            htmlContent += L"<div class=\"bookmark-url\" onclick=\"window.chrome.webview.postMessage('{\\\"type\\\":\\\"bookmarkClick\\\",\\\"index\\\":";
            htmlContent += std::to_wstring(i);
            htmlContent += L"}');\" style=\"cursor: pointer;\">";
            htmlContent += bookmark.second;
            htmlContent += L"</div>\n";
            htmlContent += L"</div>\n";
        }
    }
    
    htmlContent += L"</div>\n";
    htmlContent += L"</body>\n";
    htmlContent += L"</html>\n";
    
    // 更新WebView2内容
    UpdateWebView2Content(htmlContent.c_str());
    
    // 记录更新状态
    char logMsg[200] = {0};
    sprintf(logMsg, "UpdateBookmarkModeWebView: 更新完成，显示 %zu 条书签", displayBookmarks.size());
    LogToFile(logMsg);
}