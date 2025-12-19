#include "webview_manager.h"
#include "bookmark_manager.h"
#include "common.h"
#include "calculator.h"  // 计算器功能定义
#include "logger.h"
#include <vector>
#include <set>
#include <string>
#include <shlobj.h>
#include <fstream>
#include <sstream>
// #include <codecvt> // Removed deprecated header
#include <commctrl.h>
#include <imm.h> // 输入法支持

#define IDC_EMOJI_COMBO 2001

#pragma comment(lib, "imm32.lib") // 链接输入法库

// HTML模板读取函数声明
std::wstring ReadHtmlTemplate(const std::wstring& filePath);

void EnterFormulaWizardMode(const std::wstring& formulaName)
{
    if (!g_webView) return;

    // Find formula
    const CustomFormula* targetFormula = nullptr;
    for (const auto& f : g_customFormulas) {
        if (f.name == formulaName) {
            targetFormula = &f;
            break;
        }
    }

    if (!targetFormula) {
        MessageBoxW(g_hMainWindow, L"未找到该公式", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    g_currentViewMode = ViewMode::FORMULA_WIZARD;

    // Try to read custom template first
    bool isCustom = false;
    std::wstring htmlContent = ReadHtmlTemplate(L"bin\\data\\formulas\\" + formulaName + L".html");
    if (!htmlContent.empty()) {
        isCustom = true;
    } else {
        htmlContent = ReadHtmlTemplate(L"bin\\data\\formula_wizard_template.html");
        if (htmlContent.empty()) {
            htmlContent = L"<html><body><h1>Error: Could not load template</h1></body></html>";
        }
    }

    // Replace placeholders
    auto replaceAll = [](std::wstring& str, const std::wstring& from, const std::wstring& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    };

    replaceAll(htmlContent, L"{{FORMULA_NAME}}", targetFormula->name);
    replaceAll(htmlContent, L"{{FORMULA_DESC}}", targetFormula->description);

    // Generate inputs based on expression
    // Simple heuristic: extract identifiers that are not math keywords
    std::wstring inputsHtml;
    if (!isCustom) {
    std::wstring expr = targetFormula->expression;
    
    // Check if it's a JS function definition
    bool isFunction = (expr.find(L"function") == 0 || expr.find(L"=>") != std::wstring::npos);
    std::vector<std::wstring> params;

    if (isFunction) {
        // Try to parse function arguments: function(a,b) or (a,b)=>... or a=>...
        size_t openParen = expr.find(L'(');
        size_t closeParen = expr.find(L')');
        if (openParen != std::wstring::npos && closeParen != std::wstring::npos && closeParen > openParen) {
            std::wstring args = expr.substr(openParen + 1, closeParen - openParen - 1);
            std::wstringstream ss(args);
            std::wstring segment;
            while(std::getline(ss, segment, L',')) {
                // Trim whitespace
                size_t first = segment.find_first_not_of(L" \t\n\r");
                size_t last = segment.find_last_not_of(L" \t\n\r");
                if (first != std::wstring::npos && last != std::wstring::npos) {
                    params.push_back(segment.substr(first, (last - first + 1)));
                }
            }
        }
    } else {
        // Scan for variables in simple expression
        // Allow a-z, A-Z, _, then a-z, A-Z, 0-9, _
        // Exclude common Math functions and constants
        static const std::set<std::wstring> reserved = {
            L"Math", L"sin", L"cos", L"tan", L"asin", L"acos", L"atan", L"sqrt", L"pow", L"abs", 
            L"ceil", L"floor", L"round", L"max", L"min", L"random", L"PI", L"E", L"exp", L"log",
            L"function", L"return", L"var", L"let", L"const", L"if", L"else"
        };
        
        std::wstring currentToken;
        for (size_t i = 0; i < expr.length(); ++i) {
            wchar_t c = expr[i];
            if (isalnum(c) || c == L'_') {
                currentToken += c;
            } else {
                if (!currentToken.empty()) {
                    if (!isdigit(currentToken[0]) && reserved.find(currentToken) == reserved.end()) {
                        bool exists = false;
                        for(const auto& p : params) if(p == currentToken) exists = true;
                        if (!exists) params.push_back(currentToken);
                    }
                    currentToken.clear();
                }
            }
        }
        if (!currentToken.empty()) {
            if (!isdigit(currentToken[0]) && reserved.find(currentToken) == reserved.end()) {
                 bool exists = false;
                 for(const auto& p : params) if(p == currentToken) exists = true;
                 if (!exists) params.push_back(currentToken);
            }
        }
    }

    for (const auto& param : params) {
        inputsHtml += L"<div class=\"input-group\">";
        inputsHtml += L"<label>" + param + L"</label>";
        inputsHtml += L"<input type=\"text\" placeholder=\"请输入 " + param + L"\" data-param=\"" + param + L"\">";
        inputsHtml += L"</div>";
    }

    if (params.empty()) {
        inputsHtml = L"<div class='desc'>此公式无需输入参数，请直接点击计算。</div>";
    }

    replaceAll(htmlContent, L"{{FORMULA_INPUTS}}", inputsHtml);
    }
    
    // Add JS to map inputs back to expression args if needed
    // The template uses generic input collection. 
    // We need to inject the formula name so the template knows what to call.
    // NOTE: formulaName is already replaced in the template via {{FORMULA_NAME}}
    // So we don't need to inject it again, which causes variable redeclaration error.
    // std::wstring script = L"<script>var formulaName = \"" + formulaName + L"\";</script>";
    
    // Inject custom formulas definitions so window.calculate works
    std::wstring formulasJs = L"<script>window.g_formulas = {";
    for (const auto& formula : g_customFormulas)
    {
        std::wstring name = formula.name;
        std::wstring expr = formula.expression;
        std::wstring desc = formula.description;
        
        auto escape = [](std::wstring s) {
            std::wstring res;
            for (wchar_t c : s) {
                if (c == L'\\') res += L"\\\\";
                else if (c == L'"') res += L"\\\"";
                else if (c == L'\n') res += L"\\n";
                else res += c;
            }
            return res;
        };

        formulasJs += L"\"" + escape(name) + L"\": { expr: \"" + escape(expr) + L"\", desc: \"" + escape(desc) + L"\" },";
    }
    formulasJs += L"};";
    
    formulasJs += L"window.calculate = function(expr) { "
          L"  try { "
          L"    let context = {}; \n"
          L"    for (let key in window.g_formulas) { \n"
          L"       let f = window.g_formulas[key]; \n"
          L"       if (f.expr.trim().startsWith('function') || f.expr.includes('=>')) { \n"
          L"           try { context[key] = eval('(' + f.expr + ')'); } catch(e){} \n"
          L"       } \n"
          L"    } \n"
          L"    let run = function(code, ctx) { \n"
          L"       with(Math) { with(ctx) { return eval(code); } } \n"
          L"    }; \n"
          L"    return run(expr, context); "
          L"  } catch (e) { return 'Error: ' + e.message; } "
          L"};</script>";
          
    // htmlContent += script;
    htmlContent += formulasJs;

    g_webView->NavigateToString(htmlContent.c_str());
}

void ExitFormulaWizardMode()
{
    g_currentViewMode = ViewMode::CALCULATOR;
    UpdateCalculatorModeWebView();
}

void InjectCustomFormulasToWebView()
{
    if (!g_webView) return;

    std::wstring js = L"window.g_formulas = {";
    for (const auto& formula : g_customFormulas)
    {
        std::wstring name = formula.name;
        std::wstring expr = formula.expression;
        std::wstring desc = formula.description;
        
        auto escape = [](std::wstring s) {
            std::wstring res;
            for (wchar_t c : s) {
                if (c == L'\\') res += L"\\\\";
                else if (c == L'"') res += L"\\\"";
                else if (c == L'\n') res += L"\\n";
                else res += c;
            }
            return res;
        };

        js += L"\"" + escape(name) + L"\": { expr: \"" + escape(expr) + L"\", desc: \"" + escape(desc) + L"\" },";
    }
    js += L"};";
    
    js += L"window.calculate = function(expr) { "
          L"  try { "
          L"    let context = {}; \n"
          L"    for (let key in window.g_formulas) { \n"
          L"       let f = window.g_formulas[key]; \n"
          L"       if (f.expr.trim().startsWith('function') || f.expr.includes('=>')) { \n"
          L"           try { context[key] = eval('(' + f.expr + ')'); } catch(e){} \n"
          L"       } \n"
          L"    } \n"
          L"    let run = function(code, ctx) { \n"
          L"       with(Math) { with(ctx) { return eval(code); } } \n"
          L"    }; \n"
          L"    return run(expr, context); "
          L"  } catch (e) { return 'Error: ' + e.message; } "
          L"};";

    g_webView->ExecuteScript(js.c_str(), nullptr);
}

void EvaluateJSExpression(const std::wstring& expression)
{
    if (!g_webView) return;

    std::wstring escapedExpr;
    for (wchar_t c : expression) {
        if (c == L'\\') escapedExpr += L"\\\\";
        else if (c == L'"') escapedExpr += L"\\\"";
        else escapedExpr += c;
    }

    std::wstring script = L"try { "
                          L"  let res = window.calculate ? window.calculate(\"" + escapedExpr + L"\") : eval(\"" + escapedExpr + L"\"); "
                          L"  window.chrome.webview.postMessage(JSON.stringify({type: 'calcResult', expr: \"" + escapedExpr + L"\", result: '' + res})); "
                          L"} catch (e) { "
                          L"  window.chrome.webview.postMessage(JSON.stringify({type: 'calcResult', expr: \"" + escapedExpr + L"\", result: 'Error: ' + e.message})); "
                          L"}";
    
    g_webView->ExecuteScript(script.c_str(), nullptr);
}

static void ProcessWebViewMessage(const std::wstring& msgStr);
static void UpdateControllerBounds();
void UpdateInitialWebViewContent();
static void HandleSearchItemMessage(const std::wstring& msgStr);
static void HandleCalculatorMessage(const std::wstring& msgStr);
static void HandleSettingsMessage(const std::wstring& msgStr);
static void HandleBookmarkMessage(const std::wstring& msgStr);
static void HandleDirMessage(const std::wstring& msgStr);
static void HandleShortcutMessage(const std::wstring& msgStr);
static int FindShortcutOriginalIndex(const ShortcutItem& item);
static int FindBookmarkOriginalIndex(const std::pair<std::wstring, std::wstring>& b);
static void HandleFormulaManagerMessage(const std::wstring& msgStr);

// WebView2相关全局变量定义
ComPtr<ICoreWebView2Environment> g_webViewEnvironment;
ComPtr<ICoreWebView2Controller> g_webViewController;
ComPtr<ICoreWebView2> g_webView;
HWND g_hWebView2 = nullptr;
bool g_settingsMenuMode = false;
std::set<std::wstring> g_expandedPaths;
std::wstring g_currentDirPath;
ViewMode g_currentViewMode = ViewMode::NONE;
std::wstring g_lastSearchQuery;

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
                        UpdateControllerBounds();
                        
                        // 获取 WebView2 核心对象
                        g_webViewController->get_CoreWebView2(&g_webView);
                        if (g_webView)
                        {
                            // 通知主窗口 WebView2 已准备就绪
                            PostMessage(g_hMainWindow, WM_APP_WEBVIEW_READY, 0, 0);
                            
                            // 设置消息接收处理器
                            g_webView->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                [](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                    WCHAR* message = nullptr;
                                    args->TryGetWebMessageAsString(&message);
                                    if (message)
                                    {
                                        // 记录所有接收到的消息用于调试
                                        std::wstring msgStr = message;
                                        
                                        // 使用 UTF-8 转换以支持中文显示
                                        int size_needed = WideCharToMultiByte(CP_UTF8, 0, msgStr.c_str(), (int)msgStr.length(), NULL, 0, NULL, NULL);
                                        std::string utf8Msg(size_needed, 0);
                                        WideCharToMultiByte(CP_UTF8, 0, msgStr.c_str(), (int)msgStr.length(), &utf8Msg[0], size_needed, NULL, NULL);
                                        
                                        std::string logMsg = "WebView2消息收到RAW: " + utf8Msg;
                                        LogToFile(logMsg.c_str());

                                        CoTaskMemFree(message);
                                        ProcessWebViewMessage(msgStr);
                                        return S_OK;
                                    }
                                    return S_OK;
                                }).Get(), nullptr);
                            
                            // WebView2 初始化完成，根据当前状态显示内容
                            LogToFile("InitializeWebView2: WebView2 初始化完成，更新显示内容");
                            
                            UpdateInitialWebViewContent();
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

static void ProcessWebViewMessage(const std::wstring& msgStr)
{
    if (msgStr.find(L"\"type\":\"open\"") != std::wstring::npos)
    {
        // 处理首页快捷方式点击
        size_t pathPos = msgStr.find(L"\"path\":\"");
        if (pathPos != std::wstring::npos)
        {
            size_t start = pathPos + 8;
            size_t end = msgStr.find(L"\"", start);
            if (end != std::wstring::npos)
            {
                std::wstring path = msgStr.substr(start, end - start);
                // 打开快捷方式
                ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"goHome\"") != std::wstring::npos)
    {
        // 回到首页
        UpdateInitialWebViewContent();
        return;
    }
    if (msgStr.find(L"\"type\":\"itemClick\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"itemDblClick\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"editItem\"") != std::wstring::npos)
    {
        HandleSearchItemMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"calcAction\"") != std::wstring::npos)
    {
        HandleCalculatorMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"calcResult\"") != std::wstring::npos)
    {
        auto extract = [&](const std::wstring& key) -> std::wstring {
            size_t pos = msgStr.find(L"\"" + key + L"\":\"");
            if (pos != std::wstring::npos) {
                pos += key.length() + 4;
                size_t end = msgStr.find(L"\"", pos);
                if (end != std::wstring::npos) {
                    std::wstring val = msgStr.substr(pos, end - pos);
                    size_t replacePos = 0;
                    while ((replacePos = val.find(L"\\\\", replacePos)) != std::wstring::npos) {
                        val.replace(replacePos, 2, L"\\");
                        replacePos += 1;
                    }
                    return val;
                }
            }
            return L"";
        };
        std::wstring expr = extract(L"expr");
        std::wstring result = extract(L"result");
        OnCalculationResult(expr, result);
        return;
    }
    if (msgStr.find(L"\"type\":\"settingsAction\"") != std::wstring::npos)
    {
        HandleSettingsMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"bookmarkClick\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"bookmarkDblClick\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"addBookmark\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"editBookmark\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"deleteBookmark\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"addBookmarkFromDialog\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"getBookmarkData\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"editBookmarkFromDialog\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"closeBookmarkDialog\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"deleteBookmarkFromDialog\"") != std::wstring::npos)
    {
        HandleBookmarkMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"dirExpand\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"dirOpen\"") != std::wstring::npos)
    {
        HandleDirMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"addShortcut\"") != std::wstring::npos)
    {
        ShowAddShortcutDialog();
        return;
    }
    if (msgStr.find(L"\"type\":\"openShortcut\"") != std::wstring::npos)
    {
        HandleShortcutMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"formulaManagerAction\"") != std::wstring::npos)
    {
        HandleFormulaManagerMessage(msgStr);
        return;
    }
    if (msgStr.find(L"\"type\":\"calculateFormula\"") != std::wstring::npos)
    {
        // Extract expression from message
        size_t exprPos = msgStr.find(L"\"expression\":\"");
        if (exprPos != std::wstring::npos)
        {
            size_t start = exprPos + 14;
            size_t end = msgStr.find(L"\"", start);
            if (end != std::wstring::npos)
            {
                std::wstring expr = msgStr.substr(start, end - start);
                // Unescape
                size_t replacePos = 0;
                while ((replacePos = expr.find(L"\\\\", replacePos)) != std::wstring::npos) {
                    expr.replace(replacePos, 2, L"\\");
                    replacePos += 1;
                }
                
                EvaluateJSExpression(expr);
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"closeWizard\"") != std::wstring::npos)
    {
        ExitFormulaWizardMode();
        return;
    }
}

static void HandleSearchItemMessage(const std::wstring& msgStr)
{
    if (msgStr.find(L"\"type\":\"itemClick\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"itemDblClick\"") != std::wstring::npos)
    {
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
        return;
    }
    if (msgStr.find(L"\"type\":\"editItem\"") != std::wstring::npos)
    {
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
        if (index >= 0)
        {
            if (g_searchResults.empty())
            {
                if (index < (int)g_shortcuts.size())
                {
                    ShowEditShortcutDialog(index);
                }
                else
                {
                    LogToFile("WebView2消息: 编辑快捷方式失败 - 索引超出g_shortcuts范围");
                    MessageBoxW(g_hMainWindow, L"无效的快捷方式索引", L"错误", MB_OK | MB_ICONERROR);
                }
            }
            else
            {
                if (index < (int)g_searchResults.size())
                {
                    const ShortcutItem& s = g_searchResults[index];
                    int originalIndex = FindShortcutOriginalIndex(s);
                    if (originalIndex >= 0)
                    {
                        ShowEditShortcutDialog(originalIndex);
                    }
                }
                else
                {
                    LogToFile("WebView2消息: 编辑快捷方式失败 - 索引超出g_searchResults范围");
                    MessageBoxW(g_hMainWindow, L"无效的快捷方式索引", L"错误", MB_OK | MB_ICONERROR);
                }
            }
        }
        else
        {
            LogToFile("WebView2消息: 编辑快捷方式失败 - 索引无效");
            MessageBoxW(g_hMainWindow, L"无效的快捷方式索引", L"错误", MB_OK | MB_ICONERROR);
        }
    }
}

static void HandleCalculatorMessage(const std::wstring& msgStr)
{
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
            return;
        }
        size_t actualIndex = g_calculationHistory.size() - 1 - index;
        if (actualIndex >= g_calculationHistory.size())
        {
            MessageBoxW(g_hMainWindow, L"索引转换错误，无法删除", L"错误", MB_OK | MB_ICONERROR);
            return;
        }
        g_calculationHistory.erase(g_calculationHistory.begin() + actualIndex);
        SaveCalculationHistory();
        DisplayCalculationHistory();
        UpdateCalculatorModeWebView();
    }
    else if (action == L"clearAll")
    {
        if (MessageBoxW(g_hMainWindow, L"确定要清空所有计算历史吗？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            g_calculationHistory.clear();
            SaveCalculationHistory();
            DisplayCalculationHistory();
            UpdateCalculatorModeWebView();
        }
    }
    else if (action == L"openFormulaManager")
    {
        EnterFormulaManagerMode();
    }
}

static void HandleSettingsMessage(const std::wstring& msgStr)
{
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
    if (index >= 0)
    {
        INT_PTR actualIndex = index + GetHintRowCount();
        HandleSettingsMenuItemClick(actualIndex);
    }
}

static void HandleBookmarkMessage(const std::wstring& msgStr)
{
    if (msgStr.find(L"\"type\":\"addBookmark\"") != std::wstring::npos)
    {
        ShowHtmlAddBookmarkDialog();
        return;
    }
    if (msgStr.find(L"\"type\":\"bookmarkClick\"") != std::wstring::npos
        || msgStr.find(L"\"type\":\"bookmarkDblClick\"") != std::wstring::npos)
    {
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
        const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
        if (index >= 0 && index < (int)displayBookmarks.size())
        {
            ShellExecuteW(NULL, L"open", displayBookmarks[index].second.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"editBookmark\"") != std::wstring::npos)
    {
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
        const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
        if (index >= 0 && index < (int)displayBookmarks.size())
        {
            const auto& target = displayBookmarks[index];
            int originalIndex = -1;
            for (int i = 0; i < (int)g_bookmarks.size(); ++i)
            {
                if (g_bookmarks[i].first == target.first && g_bookmarks[i].second == target.second)
                {
                    originalIndex = i;
                    break;
                }
            }
            if (originalIndex >= 0)
            {
                ShowHtmlEditBookmarkDialog(originalIndex);
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"deleteBookmark\"") != std::wstring::npos)
    {
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
        const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
        if (index >= 0 && index < (int)displayBookmarks.size())
        {
            const auto& target = displayBookmarks[index];
            int originalIndex = -1;
            for (int i = 0; i < (int)g_bookmarks.size(); ++i)
            {
                if (g_bookmarks[i].first == target.first && g_bookmarks[i].second == target.second)
                {
                    originalIndex = i;
                    break;
                }
            }
            if (originalIndex >= 0)
            {
                std::wstring confirmMsg = L"确定要删除网址收藏 \"" + g_bookmarks[originalIndex].first + L"\" 吗？";
                if (MessageBoxW(g_hMainWindow, confirmMsg.c_str(), L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    DeleteBookmarkFromDisplayList(originalIndex);
                    UpdateBookmarkModeWebView();
                }
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"addBookmarkFromDialog\"") != std::wstring::npos)
    {
        std::wstring name, url;
        size_t namePos = msgStr.find(L"\"name\":\"");
        if (namePos != std::wstring::npos)
        {
            namePos += 8;
            size_t nameEnd = msgStr.find(L"\"", namePos);
            if (nameEnd != std::wstring::npos)
            {
                name = msgStr.substr(namePos, nameEnd - namePos);
            }
        }
        size_t urlPos = msgStr.find(L"\"url\":\"");
        if (urlPos != std::wstring::npos)
        {
            urlPos += 7;
            size_t urlEnd = msgStr.find(L"\"", urlPos);
            if (urlEnd != std::wstring::npos)
            {
                url = msgStr.substr(urlPos, urlEnd - urlPos);
            }
        }

        // Debug logging for parsed values (Add Bookmark)
        {
            int nameSize = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, NULL, 0, NULL, NULL);
            std::string nameUtf8(nameSize > 0 ? nameSize : 1, 0);
            if (nameSize > 0) WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, &nameUtf8[0], nameSize, NULL, NULL);
            
            int urlSize = WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, NULL, 0, NULL, NULL);
            std::string urlUtf8(urlSize > 0 ? urlSize : 1, 0);
            if (urlSize > 0) WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, &urlUtf8[0], urlSize, NULL, NULL);
            
            char logMsg[1024] = {0};
            sprintf(logMsg, "HandleBookmarkMessage(Add): 解析结果 name='%s', url='%s'", nameUtf8.c_str(), urlUtf8.c_str());
            LogToFile(logMsg);
        }

        if (name.empty() || url.empty())
        {
            LogToFile("HandleBookmarkMessage(Add): 名称或URL为空，无法添加");
            return;
        }
        if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && url.find(L"ftp://") != 0 && url.find(L"file://") != 0)
        {
            LogToFile("HandleBookmarkMessage(Add): URL格式无效 (必须以 http://, https://, ftp://, or file:// 开头)");
            return;
        }
        for (const auto& bookmark : g_bookmarks)
        {
            if (bookmark.second == url)
            {
                LogToFile("HandleBookmarkMessage(Add): URL已存在，跳过添加");
                return;
            }
        }
        g_bookmarks.push_back(std::make_pair(name, url));
        SaveBookmarks();
        if (g_bookmarkMode)
        {
            UpdateBookmarkModeWebView();
        }
        else
        {
            UpdateHelpInfoWebView();
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"getBookmarkData\"") != std::wstring::npos)
    {
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
        const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
        if (index >= 0 && index < (int)displayBookmarks.size())
        {
            std::wstringstream bookmarkData;
            bookmarkData << L"{\"type\":\"bookmarkData\",\"bookmark\":{"
                         << L"\"index\":" << index
                         << L",\"name\":\"" << displayBookmarks[index].first
                         << L"\",\"url\":\"" << displayBookmarks[index].second << L"\"}}";
            if (g_webViewController && g_webView)
            {
                g_webView->PostWebMessageAsJson(bookmarkData.str().c_str());
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"editBookmarkFromDialog\"") != std::wstring::npos)
    {
        int index = -1;
        std::wstring name, url;
        size_t indexPos = msgStr.find(L"\"index\":");
        if (indexPos != std::wstring::npos)
        {
            size_t start = msgStr.find(L":", indexPos) + 1;
            size_t end = msgStr.find(L",", start);
            if (end == std::wstring::npos) end = msgStr.find(L"}", start);
            std::wstring indexStr = msgStr.substr(start, end - start);
            index = _wtoi(indexStr.c_str());
        }
        size_t namePos = msgStr.find(L"\"name\":\"");
        if (namePos != std::wstring::npos)
        {
            namePos += 8;
            size_t nameEnd = msgStr.find(L"\"", namePos);
            if (nameEnd != std::wstring::npos)
            {
                name = msgStr.substr(namePos, nameEnd - namePos);
            }
        }
        size_t urlPos = msgStr.find(L"\"url\":\"");
        if (urlPos != std::wstring::npos)
        {
            urlPos += 7;
            size_t urlEnd = msgStr.find(L"\"", urlPos);
            if (urlEnd != std::wstring::npos)
            {
                url = msgStr.substr(urlPos, urlEnd - urlPos);
            }
        }

        // Debug logging for parsed values
        {
            int nameSize = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, NULL, 0, NULL, NULL);
            std::string nameUtf8(nameSize > 0 ? nameSize : 1, 0);
            if (nameSize > 0) WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, &nameUtf8[0], nameSize, NULL, NULL);
            
            int urlSize = WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, NULL, 0, NULL, NULL);
            std::string urlUtf8(urlSize > 0 ? urlSize : 1, 0);
            if (urlSize > 0) WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, &urlUtf8[0], urlSize, NULL, NULL);
            
            char logMsg[1024] = {0};
            sprintf(logMsg, "HandleBookmarkMessage: 解析结果 index=%d, name='%s', url='%s'", index, nameUtf8.c_str(), urlUtf8.c_str());
            LogToFile(logMsg);
        }

        if (index < 0 || name.empty() || url.empty())
        {
            char logMsg[200] = {0};
            sprintf(logMsg, "HandleBookmarkMessage: 编辑参数无效 index=%d", index);
            LogToFile(logMsg);
            return;
        }
        if (url.find(L"http://") != 0 && url.find(L"https://") != 0 && url.find(L"ftp://") != 0 && url.find(L"file://") != 0)
        {
            LogToFile("HandleBookmarkMessage: URL格式无效");
            return;
        }
        const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
        if (index >= 0 && index < (int)displayBookmarks.size())
        {
            int originalIndex = FindBookmarkOriginalIndex(displayBookmarks[index]);
            
            char logMsg[512] = {0};
            sprintf(logMsg, "HandleBookmarkMessage: 查找原始索引 index=%d, originalIndex=%d, displaySize=%zu", 
                    index, originalIndex, displayBookmarks.size());
            LogToFile(logMsg);

            if (originalIndex >= 0)
            {
                g_bookmarks[originalIndex] = std::make_pair(name, url);
                LogToFile("HandleBookmarkMessage: 更新 g_bookmarks 成功");
            }
            else
            {
                LogToFile("HandleBookmarkMessage: 未找到原始书签，无法更新 g_bookmarks");
            }

            if (!g_bookmarkSearchResults.empty() && index >= 0 && index < (int)g_bookmarkSearchResults.size())
            {
                g_bookmarkSearchResults[index] = std::make_pair(name, url);
                LogToFile("HandleBookmarkMessage: 更新 g_bookmarkSearchResults 成功");
            }
            SaveBookmarks();
            DisplayBookmarkResults();
            if (g_bookmarkMode)
            {
                UpdateBookmarkModeWebView();
            }
            else
            {
                UpdateHelpInfoWebView();
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"closeBookmarkDialog\"") != std::wstring::npos)
    {
        if (g_bookmarkMode)
        {
            UpdateBookmarkModeWebView();
        }
        else
        {
            UpdateHelpInfoWebView();
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"deleteBookmarkFromDialog\"") != std::wstring::npos)
    {
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
        if (index < 0)
        {
            return;
        }
        const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
        if (index >= 0 && index < (int)displayBookmarks.size())
        {
            int originalIndex = FindBookmarkOriginalIndex(displayBookmarks[index]);
            if (originalIndex >= 0)
            {
                g_bookmarks.erase(g_bookmarks.begin() + originalIndex);
            }
            if (!g_bookmarkSearchResults.empty() && index >= 0 && index < (int)g_bookmarkSearchResults.size())
            {
                g_bookmarkSearchResults.erase(g_bookmarkSearchResults.begin() + index);
            }
            SaveBookmarks();
            DisplayBookmarkResults();
            if (g_bookmarkMode)
            {
                UpdateBookmarkModeWebView();
            }
            else
            {
                UpdateHelpInfoWebView();
            }
        }
        return;
    }
}

static void HandleDirMessage(const std::wstring& msgStr)
{
    if (msgStr.find(L"\"type\":\"dirExpand\"") != std::wstring::npos)
    {
        std::wstring path;
        size_t pathPos = msgStr.find(L"\"path\":\"");
        if (pathPos != std::wstring::npos)
        {
            pathPos += 8;
            size_t pathEnd = msgStr.find(L"\"", pathPos);
            if (pathEnd != std::wstring::npos)
            {
                path = msgStr.substr(pathPos, pathEnd - pathPos);
                size_t pos = 0;
                while ((pos = path.find(L"\\\\", pos)) != std::wstring::npos)
                {
                    path.replace(pos, 2, L"\\");
                    pos += 1;
                }
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
            }
        }
        return;
    }
    if (msgStr.find(L"\"type\":\"dirOpen\"") != std::wstring::npos)
    {
        std::wstring path;
        bool isDir = false;
        size_t pathPos = msgStr.find(L"\"path\":\"");
        if (pathPos != std::wstring::npos)
        {
            pathPos += 8;
            size_t pathEnd = msgStr.find(L"\"", pathPos);
            if (pathEnd != std::wstring::npos)
            {
                path = msgStr.substr(pathPos, pathEnd - pathPos);
                size_t pos = 0;
                while ((pos = path.find(L"\\\\", pos)) != std::wstring::npos)
                {
                    path.replace(pos, 2, L"\\");
                    pos += 1;
                }
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
                    g_expandedPaths.insert(path);
                    g_currentDirPath = path;
                    UpdateDirModeWebView();
                }
                else
                {
                    ShellExecuteW(NULL, L"open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                }
            }
        }
    }
}

static void HandleShortcutMessage(const std::wstring& msgStr)
{
    std::wstring command;
    size_t commandPos = msgStr.find(L"\"command\":\"");
    if (commandPos != std::wstring::npos)
    {
        commandPos += 11;
        size_t commandEnd = msgStr.find(L"\"", commandPos);
        if (commandEnd != std::wstring::npos)
        {
            command = msgStr.substr(commandPos, commandEnd - commandPos);
            size_t pos = 0;
            while ((pos = command.find(L"\\\\", pos)) != std::wstring::npos)
            {
                command.replace(pos, 2, L"\\");
                pos += 1;
            }
            ShellExecuteW(NULL, L"open", command.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
    }
}

static void UpdateInitialWebViewContent()
{
    if (g_settingsMenuMode)
    {
        UpdateSettingsMenuWebView();
    }
    else if (g_currentViewMode == ViewMode::FORMULA_MANAGER)
    {
        UpdateFormulaManagerWebView();
    }
    else if (g_calculatorMode)
    {
        UpdateCalculatorModeWebView();
    }
    else if (g_bookmarkMode)
    {
        UpdateBookmarkModeWebView();
    }
    else if (g_dirMode)
    {
        UpdateDirModeWebView();
    }
    else if (g_shortcuts.empty())
    {
        UpdateHelpInfoWebView();
    }
    else
    {
        // 检查是否有快捷方式标记为显示在首页
        bool hasHomeShortcuts = false;
        for (const auto& shortcut : g_shortcuts) {
            if (shortcut.showOnHome) {
                hasHomeShortcuts = true;
                break;
            }
        }
        
        if (hasHomeShortcuts) {
            // 显示首页快捷方式
            std::wstring html = L"<html><head><meta charset='utf-8'><title>首页快捷方式</title>";
            html += L"<style>";
            html += L"body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f5f5f5; margin: 0; padding: 20px; }";
            html += L".shortcut-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap: 20px; }";
            html += L".shortcut-item { background-color: white; border-radius: 8px; padding: 15px; text-align: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1); cursor: pointer; transition: all 0.2s; }";
            html += L".shortcut-item:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.15); }";
            html += L".shortcut-icon { width: 64px; height: 64px; margin: 0 auto 10px; background-size: contain; background-repeat: no-repeat; background-position: center; }";
            html += L".shortcut-name { font-size: 14px; color: #333; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }";
            html += L"</style></head><body>";
            html += L"<h1>常用快捷方式</h1>";
            html += L"<div class='shortcut-grid'>";
            
            for (const auto& shortcut : g_shortcuts) {
                if (shortcut.showOnHome) {
                    html += L"<div class='shortcut-item' data-path='" + std::wstring(shortcut.path) + L"'>";
                    html += L"<div class='shortcut-icon' style='background-image: url(" + std::wstring(shortcut.iconPath) + L");'></div>";
                    html += L"<div class='shortcut-name'>" + std::wstring(shortcut.name) + L"</div>";
                    html += L"</div>";
                }
            }
            
            html += L"</div>";
            html += L"<script>";
            html += L"document.querySelectorAll('.shortcut-item').forEach(item => {";
            html += L"  item.addEventListener('click', () => {";
            html += L"    const path = item.getAttribute('data-path');";
            html += L"    window.chrome.webview.postMessage({ type: 'open', path: path });";
            html += L"  });";
            html += L"});";
            html += L"</script>";
            html += L"</body></html>";
            
            UpdateWebView2Content(html.c_str());
        } else {
            // 没有首页快捷方式，显示搜索结果
            SearchAndDisplayResults(g_currentSearch);
        }
    }
}

static void UpdateControllerBounds()
{
    if (g_hWebView2)
    {
        RECT bounds;
        if (GetClientRect(g_hWebView2, &bounds))
        {
            g_webViewController->put_Bounds(bounds);
            char logMsg[200] = {0};
            sprintf(logMsg, "InitializeWebView2: 设置 WebView2 位置和大小: (%ld, %ld, %ld, %ld)",
                    bounds.left, bounds.top, bounds.right, bounds.bottom);
            LogToFile(logMsg);
        }
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
        // Escape single quotes and backslashes for JavaScript
        std::wstring escapedName = bookmark.first;
        size_t pos = 0;
        while ((pos = escapedName.find(L"\\", pos)) != std::wstring::npos) {
            escapedName.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = escapedName.find(L"'", pos)) != std::wstring::npos) {
            escapedName.replace(pos, 1, L"\\'");
            pos += 2;
        }
        
        std::wstring escapedUrl = bookmark.second;
        pos = 0;
        while ((pos = escapedUrl.find(L"\\", pos)) != std::wstring::npos) {
            escapedUrl.replace(pos, 1, L"\\\\");
            pos += 2;
        }
        pos = 0;
        while ((pos = escapedUrl.find(L"'", pos)) != std::wstring::npos) {
            escapedUrl.replace(pos, 1, L"\\'");
            pos += 2;
        }

        // 在<body>标签后添加脚本以初始化对话框数据
        std::wstring script = L"<script>\n";
        script += L"window.onload = function() {\n";
        script += L"    initializeDialog(" + std::to_wstring(index) + L", '" + escapedName + L"', '" + escapedUrl + L"');\n";
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
    LogToFile("GetSimpleInput: called");
    // 使用简单的输入方式：直接使用编辑控件
    // 创建一个简单的编辑窗口
    HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpDefault, 
                                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                10, 30, 300, 25, 
                                g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hEdit)
    {
        LogToFile("GetSimpleInput: Failed to create edit control");
        return FALSE;
    }
    
    // 计算对话框在屏幕中心的位置
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dialogWidth = 350;
    int dialogHeight = 120;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    // 创建一个简单的对话框窗口，显示在屏幕中心
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lpCaption,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                               dialogX, dialogY, dialogWidth, dialogHeight,
                               g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hDlg)
    {
        LogToFile("GetSimpleInput: Failed to create dialog window");
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
    LogToFile("GetSimpleInput: Dialog shown");
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
                LogToFile("GetSimpleInput: OK clicked");
                // 获取输入文本
                GetWindowTextW(hEdit, lpResult, nResultSize);
                bResult = TRUE;
                break;
            }
            else if (LOWORD(msg.wParam) == IDCANCEL)
            {
                LogToFile("GetSimpleInput: Cancel clicked");
                bResult = FALSE;
                break;
            }
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理资源
    DestroyWindow(hDlg);
    DestroyWindow(hEdit); // Parent destroyed, child usually destroyed too but safe to be sure if not child
    
    return bResult;
}

/**
 * @brief 多行输入对话框函数
 * 
 * 此函数创建一个包含名称、路径和备注三个编辑框的对话框，允许用户同时编辑所有信息
 * 
 * @param lpCaption 对话框标题
 * @param lpName 名称编辑框的初始值和返回结果
 * @param lpPath 路径编辑框的初始值和返回结果
 * @param lpComment 备注编辑框的初始值和返回结果
 * @param nNameSize 名称缓冲区大小
 * @param nPathSize 路径缓冲区大小
 * @param nCommentSize 备注缓冲区大小
 * @return BOOL 如果用户点击确定返回TRUE，点击取消返回FALSE
 */
BOOL GetMultiLineInput(LPCWSTR lpCaption, LPWSTR lpName, LPWSTR lpPath, LPWSTR lpComment, int nNameSize, int nPathSize, int nCommentSize)
{
    // 计算对话框在屏幕中心的位置
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dialogWidth = 500;
    int dialogHeight = 300;
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    // 创建一个包含多个编辑框的对话框窗口，显示在屏幕中心
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lpCaption,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
                               dialogX, dialogY, dialogWidth, dialogHeight,
                               g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hDlg)
    {
        return FALSE;
    }
    
    // 创建名称标签
    HWND hNameLabel = CreateWindowExW(0, L"STATIC", L"名称:",
                                     WS_VISIBLE | WS_CHILD,
                                     10, 10, 80, 20,
                                     hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建名称编辑框
    HWND hNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpName, 
                                    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                    100, 10, 380, 25, 
                                    hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建路径标签
    HWND hPathLabel = CreateWindowExW(0, L"STATIC", L"路径:",
                                     WS_VISIBLE | WS_CHILD,
                                     10, 45, 80, 20,
                                     hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建路径编辑框
    HWND hPathEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpPath, 
                                    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                    100, 45, 380, 25, 
                                    hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建备注标签
    HWND hCommentLabel = CreateWindowExW(0, L"STATIC", L"备注:",
                                         WS_VISIBLE | WS_CHILD,
                                         10, 80, 80, 20,
                                         hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建备注编辑框（多行）
    HWND hCommentEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpComment, 
                                       WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                                       100, 80, 380, 100, 
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建确定按钮
    HWND hOkBtn = CreateWindowExW(0, L"BUTTON", L"确定", 
                                 WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                 300, 200, 80, 30, 
                                 hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
    
    // 创建取消按钮
    HWND hCancelBtn = CreateWindowExW(0, L"BUTTON", L"取消", 
                                     WS_VISIBLE | WS_CHILD,
                                     390, 200, 80, 30, 
                                     hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
    
    // 显示窗口
    ShowWindow(hDlg, SW_SHOW);
    SetFocus(hNameEdit);
    
    // 设置对话框为模态窗口
    EnableWindow(g_hMainWindow, FALSE);
    
    // 简单的消息循环
    MSG msg;
    BOOL bResult = FALSE;
    BOOL bRunning = TRUE;
    
    while (bRunning && GetMessage(&msg, NULL, 0, 0))
    {
        // 检查是否是对话框的消息，如果是则直接处理并继续下一个消息
        if (IsDialogMessage(hDlg, &msg))
        {
            continue;
        }
        
        // 检查消息是否属于对话框的子控件
        if (msg.hwnd == hDlg || IsChild(hDlg, msg.hwnd))
        {
            if (msg.message == WM_COMMAND)
            {
                if (LOWORD(msg.wParam) == IDOK)
                {
                    // 获取所有输入文本
                    GetWindowTextW(hNameEdit, lpName, nNameSize);
                    GetWindowTextW(hPathEdit, lpPath, nPathSize);
                    GetWindowTextW(hCommentEdit, lpComment, nCommentSize);
                    bResult = TRUE;
                    bRunning = FALSE;
                }
                else if (LOWORD(msg.wParam) == IDCANCEL)
                {
                    bResult = FALSE;
                    bRunning = FALSE;
                }
            }
            else if (msg.message == WM_CLOSE)
            {
                // 处理对话框关闭消息
                bResult = FALSE;
                bRunning = FALSE;
            }
            
            // 如果是对话框相关消息，处理后继续下一个消息
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;  // 重要：处理完对话框消息后继续下一个消息
        }
        
        // 非对话框消息正常处理
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 恢复主窗口
    EnableWindow(g_hMainWindow, TRUE);
    SetFocus(g_hMainWindow);
    
    // 清理资源
    DestroyWindow(hDlg);
    
    return bResult;
}





// 属性对话框状态结构体
struct PropertiesDialogState {
    BOOL* pRunning;
    BOOL* pResult;
    HWND hNameEdit;
    HWND hTargetEdit;
    HWND hIconEdit; // 新增：图标路径编辑框
    HWND hEmojiCombo; // 新增：表情选择下拉框
    HWND hCommentEdit;
    HWND hShowOnHomeCheck; // 新增：显示在首页复选框
    LPWSTR lpName;
    LPWSTR lpPath;
    LPWSTR lpIconPath; // 新增：图标路径指针
    LPWSTR lpComment;
    bool* pShowOnHome; // 新增：显示在首页标志
    WNDPROC oldProc;
};

// 属性对话框子类化过程
LRESULT CALLBACK PropertiesDlgSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PropertiesDialogState* pState = (PropertiesDialogState*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    
    if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == IDC_EMOJI_COMBO && code == CBN_SELCHANGE && pState) {
             int idx = (int)SendMessageW(pState->hEmojiCombo, CB_GETCURSEL, 0, 0);
             if (idx != CB_ERR) {
                 WCHAR buffer[32] = {0};
                 SendMessageW(pState->hEmojiCombo, CB_GETLBTEXT, idx, (LPARAM)buffer);
                 if (wcscmp(buffer, L"自定义图标") != 0) {
                     // 提取纯表情符号（去掉可能的说明文字，这里假设只有表情）
                     std::wstring emojiPath = L"emoji:";
                     emojiPath += buffer;
                     SetWindowTextW(pState->hIconEdit, emojiPath.c_str());
                 }
             }
             return 0;
        }

        if (id == IDOK && pState) {
            GetWindowTextW(pState->hNameEdit, pState->lpName, 256);
            GetWindowTextW(pState->hTargetEdit, pState->lpPath, 1024);
            GetWindowTextW(pState->hIconEdit, pState->lpIconPath, 512); // 获取图标路径
            GetWindowTextW(pState->hCommentEdit, pState->lpComment, 512);
            
            // 获取显示在首页复选框状态
            if (pState->pShowOnHome && pState->hShowOnHomeCheck) {
                *pState->pShowOnHome = (SendMessageW(pState->hShowOnHomeCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            }
            
            *(pState->pResult) = TRUE;
            *(pState->pRunning) = FALSE;
            return 0;
        } else if (id == IDCANCEL && pState) {
            *(pState->pResult) = FALSE;
            *(pState->pRunning) = FALSE;
            return 0;
        }
    } else if (msg == WM_CLOSE && pState) {
        *(pState->pResult) = FALSE;
        *(pState->pRunning) = FALSE;
        return 0;
    }
    
    if (pState && pState->oldProc) {
        return CallWindowProcW(pState->oldProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/**
 * @brief 显示类似Windows属性对话框的编辑界面
 * 
 * 此函数创建一个仿Windows属性对话框样式的界面，用于编辑快捷方式
 * 具有Windows属性对话框的经典外观和布局
 * 
 * @param lpCaption 对话框标题
 * @param lpName 名称编辑框的初始值和返回结果
 * @param lpPath 路径编辑框的初始值和返回结果
 * @param lpComment 备注编辑框的初始值和返回结果
 * @param lpIconPath 图标路径编辑框的初始值和返回结果
 * @param shortcutType 快捷方式类型
 * @return BOOL 如果用户点击确定返回TRUE，点击取消返回FALSE
 */
BOOL GetPropertiesStyleInput(LPCWSTR lpCaption, LPWSTR lpName, LPWSTR lpPath, LPWSTR lpComment, LPWSTR lpIconPath, int shortcutType, bool* pShowOnHome = nullptr)
{
    // 计算对话框在屏幕中心的位置
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dialogWidth = 450;
    int dialogHeight = 440; // 增加高度以容纳
    int dialogX = (screenWidth - dialogWidth) / 2;
    int dialogY = (screenHeight - dialogHeight) / 2;
    
    // 创建Windows属性对话框样式的窗口
    HWND hDlg = CreateWindowExW(0, L"#32770", lpCaption,
                               WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                               dialogX, dialogY, dialogWidth, dialogHeight,
                               g_hMainWindow, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hDlg)
    {
        return FALSE;
    }
    
    // 创建图标显示区域（类似属性对话框的图标显示）
    HWND hIcon = CreateWindowExW(0, L"STATIC", NULL,
                                 WS_VISIBLE | WS_CHILD | SS_ICON,
                                 15, 15, 32, 32,
                                 hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 根据快捷方式类型设置图标
    HICON hIconToUse;
    if (shortcutType == 0) // 文件夹类型
        hIconToUse = (HICON)LoadImageW(GetModuleHandle(NULL), L"shell32.dll", IMAGE_ICON, 32, 32, LR_SHARED);
    else if (shortcutType == 1) // URL类型
        hIconToUse = (HICON)LoadImageW(GetModuleHandle(NULL), L"imageres.dll", IMAGE_ICON, 32, 32, LR_SHARED);
    else // 应用程序类型
        hIconToUse = (HICON)LoadImageW(GetModuleHandle(NULL), L"shell32.dll", IMAGE_ICON, 32, 32, LR_SHARED);
    
    if (hIconToUse)
    {
        SendMessageW(hIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconToUse);
    }
    
    // 创建类型标签
    HWND hTypeLabel = CreateWindowExW(0, L"STATIC", L"类型:",
                                     WS_VISIBLE | WS_CHILD,
                                     60, 15, 60, 20,
                                     hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 显示快捷方式类型
    WCHAR typeText[100] = {0};
    if (shortcutType == 0)
        wcscpy_s(typeText, L"文件夹");
    else if (shortcutType == 1)
        wcscpy_s(typeText, L"URL");
    else
        wcscpy_s(typeText, L"应用程序");
    
    HWND hTypeValue = CreateWindowExW(0, L"STATIC", typeText,
                                     WS_VISIBLE | WS_CHILD,
                                     120, 15, 150, 20,
                                     hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建位置标签
    HWND hLocationLabel = CreateWindowExW(0, L"STATIC", L"位置:",
                                         WS_VISIBLE | WS_CHILD,
                                         60, 35, 60, 20,
                                         hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    HWND hLocationValue = CreateWindowExW(0, L"STATIC", L"快速启动器",
                                         WS_VISIBLE | WS_CHILD,
                                         120, 35, 150, 20,
                                         hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建名称标签
    HWND hNameLabel = CreateWindowExW(0, L"STATIC", L"名称:",
                                     WS_VISIBLE | WS_CHILD,
                                     60, 60, 60, 20,
                                     hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建名称编辑框
    HWND hNameEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpName, 
                                    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                    120, 58, 300, 25, 
                                    hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建目标标签
    HWND hTargetLabel = CreateWindowExW(0, L"STATIC", L"目标:",
                                       WS_VISIBLE | WS_CHILD,
                                       60, 85, 60, 20,
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建目标编辑框
    HWND hTargetEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpPath, 
                                      WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                      120, 83, 300, 25, 
                                      hDlg, NULL, GetModuleHandle(NULL), NULL);

    // 创建图标路径标签 (新增)
    HWND hIconLabel = CreateWindowExW(0, L"STATIC", L"图标:",
                                       WS_VISIBLE | WS_CHILD,
                                       60, 110, 60, 20,
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建图标路径编辑框 (新增)
    HWND hIconEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpIconPath, 
                                      WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                      120, 108, 300, 25, 
                                      hDlg, NULL, GetModuleHandle(NULL), NULL);

    // 创建表情选择标签
    HWND hEmojiLabel = CreateWindowExW(0, L"STATIC", L"表情:",
                                       WS_VISIBLE | WS_CHILD,
                                       60, 137, 60, 20,
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);

    // 创建表情选择下拉框
    HWND hEmojiCombo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
                                       WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | WS_VSCROLL,
                                       120, 135, 300, 200,
                                       hDlg, (HMENU)IDC_EMOJI_COMBO, GetModuleHandle(NULL), NULL);

    // 添加表情选项
    SendMessageW(hEmojiCombo, CB_ADDSTRING, 0, (LPARAM)L"自定义图标");
    const WCHAR* emojis[] = {L"📁", L"🌐", L"📱", L"📝", L"🎮", L"🎵", L"🎬", L"🖼️", L"⚙️", L"❤️", L"⭐", L"🔥", L"⚠️", L"✅", L"❌", L"❓"};
    for (const auto& emoji : emojis)
    {
        SendMessageW(hEmojiCombo, CB_ADDSTRING, 0, (LPARAM)emoji);
    }

    // 初始化选中项
    if (wcsncmp(lpIconPath, L"emoji:", 6) == 0)
    {
        const WCHAR* emoji = lpIconPath + 6;
        int idx = (int)SendMessageW(hEmojiCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)emoji);
        if (idx != CB_ERR)
        {
            SendMessageW(hEmojiCombo, CB_SETCURSEL, idx, 0);
        }
    }
    else
    {
        SendMessageW(hEmojiCombo, CB_SETCURSEL, 0, 0); // 选中"自定义图标"
    }
    
    // 创建备注标签 (位置下移)
    HWND hCommentLabel = CreateWindowExW(0, L"STATIC", L"备注:",
                                        WS_VISIBLE | WS_CHILD,
                                        60, 165, 60, 20,
                                        hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建备注编辑框（多行，位置下移）
    HWND hCommentEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", lpComment, 
                                       WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                                       120, 165, 300, 80, 
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建起始位置标签 (位置下移)
    HWND hStartInLabel = CreateWindowExW(0, L"STATIC", L"起始位置:",
                                        WS_VISIBLE | WS_CHILD,
                                        60, 260, 60, 20,
                                        hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    HWND hStartInValue = CreateWindowExW(0, L"STATIC", L".",
                                        WS_VISIBLE | WS_CHILD,
                                        120, 260, 150, 20,
                                        hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建快捷键标签 (位置下移)
    HWND hShortcutKeyLabel = CreateWindowExW(0, L"STATIC", L"快捷键:",
                                            WS_VISIBLE | WS_CHILD,
                                            60, 285, 60, 20,
                                            hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    HWND hShortcutKeyValue = CreateWindowExW(0, L"STATIC", L"无",
                                            WS_VISIBLE | WS_CHILD,
                                            120, 285, 60, 20,
                                            hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建显示在首页复选框
    HWND hShowOnHomeLabel = CreateWindowExW(0, L"STATIC", L"显示在首页:",
                                           WS_VISIBLE | WS_CHILD,
                                           60, 290, 80, 20,
                                           hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    HWND hShowOnHomeCheck = CreateWindowExW(0, L"BUTTON", L"",
                                           WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                           140, 290, 20, 20,
                                           hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 初始化复选框状态
    if (pShowOnHome && *pShowOnHome)
    {
        SendMessageW(hShowOnHomeCheck, BM_SETCHECK, BST_CHECKED, 0);
    }
    
    // 创建按钮组框架 (位置下移)
    HWND hButtonGroup = CreateWindowExW(0, L"BUTTON", NULL,
                                       WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                                       15, 320, 415, 60,
                                       hDlg, NULL, GetModuleHandle(NULL), NULL);
    
    // 创建确定按钮 (位置下移)
    HWND hOkBtn = CreateWindowExW(0, L"BUTTON", L"确定", 
                                 WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                 250, 340, 80, 30, 
                                 hDlg, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
    
    // 创建取消按钮 (位置下移)
    HWND hCancelBtn = CreateWindowExW(0, L"BUTTON", L"取消", 
                                     WS_VISIBLE | WS_CHILD,
                                     340, 340, 80, 30, 
                                     hDlg, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
    
    // 显示窗口
    ShowWindow(hDlg, SW_SHOW);
    SetFocus(hNameEdit);
    
    // 显式关联输入法上下文，确保中文输入可用
    HIMC hIMC = ImmGetContext(hDlg);
    if (hIMC)
    {
        ImmAssociateContext(hDlg, hIMC);
        ImmReleaseContext(hDlg, hIMC);
    }
    
    // 设置对话框为模态窗口
    EnableWindow(g_hMainWindow, FALSE);
    
    // 消息循环
    MSG msg;
    BOOL bResult = FALSE;
    BOOL bRunning = TRUE;
    
    // 设置子类化以拦截消息
    PropertiesDialogState state = {0};
    state.pRunning = &bRunning;
    state.pResult = &bResult;
    state.hNameEdit = hNameEdit;
    state.hTargetEdit = hTargetEdit;
    state.hIconEdit = hIconEdit; // 新增
    state.hEmojiCombo = hEmojiCombo; // 新增
    state.hCommentEdit = hCommentEdit;
    state.hShowOnHomeCheck = hShowOnHomeCheck; // 新增
    state.lpName = lpName;
    state.lpPath = lpPath;
    state.lpIconPath = lpIconPath; // 新增
    state.lpComment = lpComment;
    state.pShowOnHome = pShowOnHome; // 新增
    
    SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)&state);
    state.oldProc = (WNDPROC)SetWindowLongPtrW(hDlg, GWLP_WNDPROC, (LONG_PTR)PropertiesDlgSubclassProc);
    
    while (bRunning && GetMessage(&msg, NULL, 0, 0))
    {
        // 优先处理输入法相关消息，防止IsDialogMessage吞掉它们
        if (msg.message == WM_IME_COMPOSITION || 
            msg.message == WM_IME_STARTCOMPOSITION || 
            msg.message == WM_IME_ENDCOMPOSITION ||
            msg.message == WM_IME_NOTIFY ||
            msg.message == WM_IME_SETCONTEXT ||
            msg.message == WM_IME_CONTROL ||
            msg.message == WM_IME_COMPOSITIONFULL ||
            msg.message == WM_IME_SELECT ||
            msg.message == WM_IME_CHAR)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        // 检查是否是对话框的消息
        if (!IsDialogMessage(hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    // 恢复主窗口
    EnableWindow(g_hMainWindow, TRUE);
    SetFocus(g_hMainWindow);
    
    // 清理资源
    DestroyWindow(hDlg);
    
    return bResult;
}

/**
 * @brief 显示编辑快捷方式对话框
 * 
 * 此函数显示编辑快捷方式的对话框，允许用户修改快捷方式的名称、路径和备注
 * 类似Windows标准属性对话框的样式和操作方式
 * 
 * @param index 要编辑的快捷方式索引
 */
void ShowEditShortcutDialog(int index)
{
    LogToFile("ShowEditShortcutDialog: 显示编辑快捷方式对话框");
    
    // 检查索引是否有效
    if (index < 0 || index >= (int)g_shortcuts.size())
    {
        MessageBoxW(g_hMainWindow, L"无效的快捷方式索引", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditShortcutDialog: 无效索引");
        return;
    }
    
    // 获取当前快捷方式信息
    ShortcutItem& shortcut = g_shortcuts[index];
    WCHAR name[256] = {0};
    WCHAR path[1024] = {0};
    WCHAR comment[512] = {0};
    WCHAR iconPath[512] = {0};
    
    // 复制当前值到缓冲区
    wcscpy_s(name, 255, shortcut.name);
    wcscpy_s(path, 1023, shortcut.path);
    wcscpy_s(comment, 511, shortcut.comment);
    wcscpy_s(iconPath, 511, shortcut.iconPath);
    
    // 显示类似Windows属性对话框的编辑界面
    bool showOnHome = shortcut.showOnHome;
    if (!GetPropertiesStyleInput(L"属性", name, path, comment, iconPath, shortcut.type, &showOnHome))
    {
        LogToFile("ShowEditShortcutDialog: 用户取消编辑");
        return;
    }
    
    // 检查名称是否为空
    if (wcslen(name) == 0)
    {
        MessageBoxW(g_hMainWindow, L"快捷方式名称不能为空", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditShortcutDialog: 名称为空");
        return;
    }
    
    // 检查路径是否为空
    if (wcslen(path) == 0)
    {
        MessageBoxW(g_hMainWindow, L"快捷方式路径不能为空", L"错误", MB_OK | MB_ICONERROR);
        LogToFile("ShowEditShortcutDialog: 路径为空");
        return;
    }
    
    // 更新快捷方式信息
    wcscpy_s(shortcut.name, 255, name);
    wcscpy_s(shortcut.path, 1023, path);
    wcscpy_s(shortcut.comment, 511, comment);
    wcscpy_s(shortcut.iconPath, 511, iconPath);
    shortcut.showOnHome = showOnHome;
    
    // 更新使用次数
    shortcut.usageCount++;
    
    // 保存快捷方式列表
    SaveShortcuts();
    
    // 刷新显示
    SearchAndDisplayResults(g_currentSearch);
    
    LogToFile("ShowEditShortcutDialog: 快捷方式编辑成功，界面已更新");
}

/**
 * @brief 显示添加快捷方式对话框
 * 
 * 此函数显示添加快捷方式的对话框
 */
void ShowAddShortcutDialog()
{
    LogToFile("ShowAddShortcutDialog: 显示添加快捷方式对话框");
    
    WCHAR name[256] = {0};
    WCHAR path[1024] = {0};
    WCHAR comment[512] = {0};
    WCHAR iconPath[512] = {0};
    
    // 显示类似Windows属性对话框的编辑界面
    // 默认为URL类型(1)
    bool showOnHome = false; // 默认不显示在首页
    if (!GetPropertiesStyleInput(L"添加快捷方式", name, path, comment, iconPath, 1, &showOnHome))
    {
        LogToFile("ShowAddShortcutDialog: 用户取消添加");
        return;
    }
    
    // 检查名称是否为空
    if (wcslen(name) == 0)
    {
        MessageBoxW(g_hMainWindow, L"快捷方式名称不能为空", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    // 检查路径是否为空
    if (wcslen(path) == 0)
    {
        MessageBoxW(g_hMainWindow, L"快捷方式路径不能为空", L"错误", MB_OK | MB_ICONERROR);
        return;
    }
    
    ShortcutItem shortcut = {0};
    wcscpy_s(shortcut.name, 255, name);
    wcscpy_s(shortcut.path, 1023, path);
    wcscpy_s(shortcut.comment, 511, comment);
    wcscpy_s(shortcut.iconPath, 511, iconPath);
    
    // 简单的类型推断
    if (wcsstr(path, L"http://") || wcsstr(path, L"https://"))
        shortcut.type = 1; // URL
    else
    {
        // 检查是否是目录
        DWORD attrs = GetFileAttributesW(path);
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
            shortcut.type = 0; // Directory
        else
            shortcut.type = 2; // Application
    }
        
    shortcut.usageCount = 0;
    shortcut.showOnHome = showOnHome;
    
    g_shortcuts.push_back(shortcut);
    
    // 保存快捷方式列表
    SaveShortcuts();
    
    // 刷新显示
    SearchAndDisplayResults(g_currentSearch);
    
    LogToFile("ShowAddShortcutDialog: 快捷方式添加成功");
}

/**
 * @brief 保存快捷方式列表到文件
 * 
 * 此函数将当前快捷方式列表保存到data\shortcuts.txt文件中
 */
void SaveShortcuts()
{
    LogToFile("SaveShortcuts: 开始保存快捷方式列表");

    WCHAR modulePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, modulePath, MAX_PATH);
    std::wstring moduleDir = modulePath;
    size_t lastBackslash = moduleDir.find_last_of(L"\\");
    if (lastBackslash != std::wstring::npos) moduleDir = moduleDir.substr(0, lastBackslash);
    std::wstring parentDir = moduleDir;
    size_t parentBackslash = parentDir.find_last_of(L"\\");
    if (parentBackslash != std::wstring::npos) parentDir = parentDir.substr(0, parentBackslash);

    std::wstring rootDataDir = parentDir + L"\\data";
    std::wstring rootFile = rootDataDir + L"\\shortcuts.txt";
    std::wstring binDataDir = moduleDir + L"\\data";
    std::wstring binFile = binDataDir + L"\\shortcuts.txt";

    FILE* file = _wfopen(rootFile.c_str(), L"w, ccs=UTF-8");
    if (!file)
    {
        CreateDirectoryW(rootDataDir.c_str(), NULL);
        file = _wfopen(rootFile.c_str(), L"w, ccs=UTF-8");
    }
    if (!file)
    {
        CreateDirectoryW(binDataDir.c_str(), NULL);
        file = _wfopen(binFile.c_str(), L"w, ccs=UTF-8");
    }
    if (!file)
    {
        LogToFile("SaveShortcuts: 无法打开快捷方式文件进行写入");
        return;
    }
    
    // 写入快捷方式
    for (const auto& shortcut : g_shortcuts)
    {
        // 格式：名称|路径|类型|备注|图标路径|使用次数|显示在首页
        fwprintf(file, L"%s|%s|%d|%s|%s|%d|%d\n", 
                 shortcut.name, 
                 shortcut.path, 
                 shortcut.type, 
                 shortcut.comment, 
                 shortcut.iconPath, 
                 shortcut.usageCount,
                 shortcut.showOnHome ? 1 : 0);
    }
    
    fclose(file);
    
    // 记录保存的快捷方式数量
    char logMsg[200] = {0};
    sprintf(logMsg, "SaveShortcuts: 保存了 %zu 条快捷方式", g_shortcuts.size());
    LogToFile(logMsg);
    LogToFile("SaveShortcuts: 函数结束");
}

static int FindShortcutOriginalIndex(const ShortcutItem& item)
{
    for (int i = 0; i < (int)g_shortcuts.size(); ++i)
    {
        if (_wcsicmp(g_shortcuts[i].name, item.name) == 0 && _wcsicmp(g_shortcuts[i].path, item.path) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int FindBookmarkOriginalIndex(const std::pair<std::wstring, std::wstring>& b)
{
    for (int i = 0; i < (int)g_bookmarks.size(); ++i)
    {
        if (g_bookmarks[i].first == b.first && g_bookmarks[i].second == b.second)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 从文件加载快捷方式列表
 *
 * 优先从上层目录的 data\shortcuts.txt 读取；若不存在则尝试从当前模块目录的 data\shortcuts.txt 读取
 * 文件格式：名称|路径|类型|备注|图标路径|使用次数
 *
 * @return true 如果成功加载（文件存在且解析成功，可能为空列表也算成功）
 * @return false 如果文件不存在或打开失败
 */
bool LoadShortcuts()
{
    LogToFile("LoadShortcuts: 开始加载快捷方式列表");

    WCHAR modulePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, modulePath, MAX_PATH);
    std::wstring moduleDir = modulePath;
    size_t lastBackslash = moduleDir.find_last_of(L"\\");
    if (lastBackslash != std::wstring::npos) moduleDir = moduleDir.substr(0, lastBackslash);
    std::wstring parentDir = moduleDir;
    size_t parentBackslash = parentDir.find_last_of(L"\\");
    if (parentBackslash != std::wstring::npos) parentDir = parentDir.substr(0, parentBackslash);

    std::wstring rootFile = parentDir + L"\\data\\shortcuts.txt";
    std::wstring binFile = moduleDir + L"\\data\\shortcuts.txt";

    std::wstring chosenFile;
    if (GetFileAttributesW(rootFile.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        chosenFile = rootFile;
    }
    else if (GetFileAttributesW(binFile.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        chosenFile = binFile;
    }
    else
    {
        LogToFile("LoadShortcuts: 快捷方式文件不存在");
        return false;
    }

    FILE* file = _wfopen(chosenFile.c_str(), L"r, ccs=UTF-8");
    if (!file)
    {
        LogToFile("LoadShortcuts: 无法打开快捷方式文件进行读取");
        return false;
    }

    // 清空现有列表
    g_shortcuts.clear();

    WCHAR line[2048];
    while (fgetws(line, sizeof(line)/sizeof(WCHAR), file))
    {
        size_t len = wcslen(line);
        if (len > 0 && line[len - 1] == L'\n') line[len - 1] = L'\0';
        if (line[0] == L'\0') continue;

        // 分割字段
        std::wstring s = line;
        std::vector<std::wstring> parts;
        size_t start = 0;
        while (true)
        {
            size_t p = s.find(L'|', start);
            if (p == std::wstring::npos) { parts.push_back(s.substr(start)); break; }
            parts.push_back(s.substr(start, p - start));
            start = p + 1;
        }

        if (parts.size() < 2) continue; // 至少需要名称和路径

        ShortcutItem item = {0};
        wcscpy_s(item.name, 255, parts[0].c_str());
        wcscpy_s(item.path, 255, parts[1].c_str());
        item.type = (parts.size() >= 3) ? _wtoi(parts[2].c_str()) : 2;
        wcscpy_s(item.comment, 511, (parts.size() >= 4) ? parts[3].c_str() : L"");
        wcscpy_s(item.iconPath, 511, (parts.size() >= 5) ? parts[4].c_str() : L"" );
        item.usageCount = (parts.size() >= 6) ? _wtoi(parts[5].c_str()) : 0;
        item.showOnHome = (parts.size() >= 7) ? (_wtoi(parts[6].c_str()) != 0) : false;

        g_shortcuts.push_back(item);
    }

    fclose(file);

    char logMsg[200] = {0};
    sprintf(logMsg, "LoadShortcuts: 加载了 %zu 条快捷方式", g_shortcuts.size());
    LogToFile(logMsg);
    return true;
}

/**
 * @brief 从快捷方式文件或可执行文件中提取图标路径
 * 
 * 此函数根据快捷方式类型和路径提取对应的图标路径
 * 对于文件夹类型，使用系统文件夹图标
 * 对于URL类型，使用网站favicon图标
 * 对于应用程序类型，从可执行文件提取图标
 * 
 * @param shortcut 快捷方式项
 * @param iconPath 输出参数，存储提取的图标路径
 * @return BOOL 成功返回TRUE，失败返回FALSE
 */
BOOL ExtractShortcutIcon(const ShortcutItem& shortcut, WCHAR* iconPath, int iconPathSize)
{
    if (iconPath == NULL || iconPathSize <= 0)
    {
        LogToFile("ExtractShortcutIcon: 输出参数无效");
        return FALSE;
    }
    
    // 如果快捷方式已经有图标路径，直接使用
    if (wcslen(shortcut.iconPath) > 0)
    {
        wcscpy_s(iconPath, iconPathSize, shortcut.iconPath);
        return TRUE;
    }
    
    // 根据快捷方式类型处理图标
    switch (shortcut.type)
    {
    case 0: // 文件夹类型
        {
            // 使用系统文件夹图标
            wcscpy_s(iconPath, iconPathSize, L"shell32.dll,-34");
            return TRUE;
        }
        
    case 1: // URL类型
        {
            // 对于URL，尝试使用网站favicon
            // 这里可以扩展为从网站获取favicon，目前使用默认链接图标
            wcscpy_s(iconPath, iconPathSize, L"imageres.dll,-1002"); // 默认链接图标
            return TRUE;
        }
        
    case 2: // 应用程序类型
        {
            // 对于应用程序，尝试从可执行文件提取图标
            WCHAR exePath[MAX_PATH] = {0};
            
            // 检查路径是否包含空格，如果是则可能需要引号
            if (wcsstr(shortcut.path, L" ") != NULL)
            {
                // 路径包含空格，尝试解析可执行文件路径
                const WCHAR* exeStart = wcsstr(shortcut.path, L"\"");
                if (exeStart != NULL)
                {
                    // 找到引号，提取可执行文件路径
                    const WCHAR* exeEnd = wcsstr(exeStart + 1, L"\"");
                    if (exeEnd != NULL)
                    {
                        size_t exeLength = exeEnd - (exeStart + 1);
                        if (exeLength < MAX_PATH)
                        {
                            wcsncpy_s(exePath, MAX_PATH, exeStart + 1, exeLength);
                        }
                    }
                }
            }
            
            // 如果没有提取到带引号的路径，直接使用原路径
            if (wcslen(exePath) == 0)
            {
                wcscpy_s(exePath, MAX_PATH, shortcut.path);
            }
            
            // 检查文件是否存在
            if (GetFileAttributesW(exePath) != INVALID_FILE_ATTRIBUTES)
            {
                // 文件存在，直接使用可执行文件路径作为图标路径
                wcscpy_s(iconPath, iconPathSize, exePath);
                return TRUE;
            }
            else
            {
                // 文件不存在，使用默认应用程序图标
                wcscpy_s(iconPath, iconPathSize, L"shell32.dll,-1"); // 默认应用程序图标
                return TRUE;
            }
        }
        
    default:
        {
            // 未知类型，使用默认图标
            wcscpy_s(iconPath, iconPathSize, L"shell32.dll,-1");
            return TRUE;
        }
    }
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
    const auto& displayBookmarks = g_bookmarkSearchResults.empty() ? g_bookmarks : g_bookmarkSearchResults;
    bool isSearchResult = !g_bookmarkSearchResults.empty();
    std::wstring tpl = ReadHtmlTemplate(L"data/bookmark_template.html");
    if (!tpl.empty())
    {
        std::wstring table;
        if (displayBookmarks.empty())
        {
            table += L"<div class='empty'>";
            table += isSearchResult ? L"未找到匹配的网址收藏" : L"暂无网址收藏";
            table += L"</div>";
        }
        else
        {
            table += L"<table><thead><tr><th>名称</th><th>URL</th><th>操作</th></tr></thead><tbody>";
            for (size_t i = 0; i < displayBookmarks.size(); ++i)
            {
                const auto& b = displayBookmarks[i];
                table += L"<tr class='bookmark-row' onclick='onBookmarkRowClick(" + std::to_wstring(i) + L")' ondblclick='onBookmarkRowDblClick(" + std::to_wstring(i) + L")'>";
                table += L"<td>" + b.first + L"</td>";
                table += L"<td class='url-cell'>" + b.second + L"</td>";
                table += L"<td>";
                table += L"<button onclick=\"window.chrome.webview.postMessage(JSON.stringify({type:'editBookmark',index:" + std::to_wstring(i) + L"}))\">编辑</button>";
                table += L"<button onclick=\"window.chrome.webview.postMessage(JSON.stringify({type:'deleteBookmark',index:" + std::to_wstring(i) + L"}))\">删除</button>";
                table += L"</td>";
                table += L"</tr>";
            }
            table += L"</tbody></table>";
        }
        const std::wstring placeholder = L"<!-- BOOKMARKS_TABLE_PLACEHOLDER -->";
        size_t p = tpl.find(placeholder);
        if (p != std::wstring::npos)
        {
            tpl.replace(p, placeholder.size(), table);
        }
        else
        {
            tpl += table;
        }
        UpdateWebView2Content(tpl.c_str());
        char logMsg[200] = {0};
        sprintf(logMsg, "UpdateBookmarkModeWebView: 更新完成，显示 %zu 条书签", displayBookmarks.size());
        LogToFile(logMsg);
        return;
    }
    std::wstring htmlContent = L"<!DOCTYPE html><html><body><div>bookmark_template.html 未找到</div></body></html>";
    UpdateWebView2Content(htmlContent.c_str());
    char logMsg[200] = {0};
    sprintf(logMsg, "UpdateBookmarkModeWebView: 模板缺失，显示 %zu 条书签", displayBookmarks.size());
    LogToFile(logMsg);
}

// HTML转义函数
static std::wstring EscapeHtmlAttribute(const std::wstring& str)
{
    std::wstring result;
    for (wchar_t c : str)
    {
        switch (c)
        {
        case L'&': result += L"&amp;"; break;
        case L'<': result += L"&lt;"; break;
        case L'>': result += L"&gt;"; break;
        case L'"': result += L"&quot;"; break;
        case L'\'': result += L"&#39;"; break;
        default: result += c;
        }
    }
    return result;
}

// 显示HTML快捷方式编辑对话框
void ShowHtmlShortcutDialog(int index)
{
    LogToFile("ShowHtmlShortcutDialog: 显示HTML快捷方式对话框");
    
    // 如果index为-1，表示添加模式；否则为编辑模式
    bool isEdit = (index >= 0);
    
    // 读取HTML对话框模板
    // 注意：这里我们复用edit_shortcut_dialog.html，如果需要区分可以创建add_shortcut_dialog.html
    // 或者在同一个HTML中根据传入的数据动态调整标题
    std::wstring dialogHtml = ReadHtmlTemplate(L"data/edit_shortcut_dialog.html");
    if (dialogHtml.empty())
    {
        LogToFile("ShowHtmlShortcutDialog: 无法读取HTML对话框模板，使用原生对话框");
        if (isEdit) ShowEditShortcutDialog(index);
        else ShowAddShortcutDialog();
        return;
    }
    
    // 准备初始化数据
    std::wstring name, path, comment, iconPath;
    int type = 2; // 默认应用程序
    int usageCount = 0;
    
    if (isEdit && index < (int)g_shortcuts.size())
    {
        const auto& item = g_shortcuts[index];
        name = item.name;
        path = item.path;
        comment = item.comment;
        iconPath = item.iconPath;
        type = item.type;
        usageCount = item.usageCount;
    }
    
    // 注入数据到HTML
    // 这里我们使用简单的字符串替换或JS注入
    // 为了更稳健，我们使用JS注入
    
    size_t bodyPos = dialogHtml.find(L"<body>");
    if (bodyPos != std::wstring::npos)
    {
        // 转义特殊字符
        auto escapeJS = [](const std::wstring& s) {
            std::wstring res = s;
            size_t pos = 0;
            while ((pos = res.find(L"\\", pos)) != std::wstring::npos) { res.replace(pos, 1, L"\\\\"); pos += 2; }
            pos = 0;
            while ((pos = res.find(L"'", pos)) != std::wstring::npos) { res.replace(pos, 1, L"\\'"); pos += 2; }
            pos = 0;
            while ((pos = res.find(L"\"", pos)) != std::wstring::npos) { res.replace(pos, 1, L"\\\""); pos += 2; }
            return res;
        };
        
        std::wstring script = L"<script>\n";
        script += L"window.onload = function() {\n";
        script += L"    if (window.initializeDialog) {\n";
        script += L"        window.initializeDialog(" + 
                  std::to_wstring(index) + L", '" + 
                  escapeJS(name) + L"', '" + 
                  escapeJS(path) + L"', '" + 
                  escapeJS(comment) + L"', '" + 
                  escapeJS(iconPath) + L"', " + 
                  std::to_wstring(type) + L");\n";
        script += L"    }\n";
        script += L"};\n";
        script += L"</script>\n";
        
        dialogHtml.insert(bodyPos + 6, script);
    }
    
    UpdateWebView2Content(dialogHtml.c_str());
}

// 更新公式管理WebView
void UpdateFormulaManagerWebView()
{
    LogToFile("UpdateFormulaManagerWebView: 更新公式管理界面");
    
    std::wstring tpl = ReadHtmlTemplate(L"data/formula_manager_template.html");
    if (tpl.empty())
    {
        LogToFile("UpdateFormulaManagerWebView: 模板缺失");
        UpdateWebView2Content(L"<!DOCTYPE html><html><body><h1>模板缺失: formula_manager_template.html</h1></body></html>");
        return;
    }
    
    // 生成 JSON 数据
    auto escapeJson = [](const std::wstring& s) {
        std::wstring res;
        for (wchar_t c : s) {
            if (c == L'\\') res += L"\\\\";
            else if (c == L'"') res += L"\\\"";
            else if (c == L'\n') res += L"\\n";
            else if (c == L'\r') res += L"\\r";
            else if (c == L'\t') res += L"\\t";
            else res += c;
        }
        return res;
    };

    std::wstring jsonData = L"<script>window.g_formulaData = [";
    for (const auto& f : g_customFormulas)
    {
        jsonData += L"{name: \"" + escapeJson(f.name) + L"\", expression: \"" + escapeJson(f.expression) + L"\", description: \"" + escapeJson(f.description) + L"\"},";
    }
    jsonData += L"];</script>";

    // 生成公式表格
    std::wstring table = L"<table id='formulaTable'><thead><tr><th>名称</th><th>表达式</th><th>描述</th><th>操作</th></tr></thead><tbody>";
    
    for (int i = 0; i < (int)g_customFormulas.size(); ++i)
    {
        const auto& f = g_customFormulas[i];
        table += L"<tr>";
        table += L"<td>" + EscapeHtmlAttribute(f.name) + L"</td>";
        table += L"<td>" + EscapeHtmlAttribute(f.expression) + L"</td>";
        table += L"<td>" + EscapeHtmlAttribute(f.description) + L"</td>";
        table += L"<td>";
        table += L"<button class='edit-btn' onclick=\"editFormula(" + std::to_wstring(i) + L")\">编辑</button>";
        table += L"<button class='delete-btn' onclick=\"deleteFormula(" + std::to_wstring(i) + L")\">删除</button>";
        table += L"</td>";
        table += L"</tr>";
    }
    table += L"</tbody></table>";
    
    // 追加 JSON 数据到表格 HTML 后面
    table += jsonData;
    
    // 替换占位符
    const std::wstring placeholder = L"<!-- FORMULA_TABLE_PLACEHOLDER -->";
    size_t p = tpl.find(placeholder);
    if (p != std::wstring::npos)
    {
        tpl.replace(p, placeholder.size(), table);
    }
    else
    {
        // 如果找不到占位符，尝试插入到body中
        size_t bodyEnd = tpl.find(L"</body>");
        if (bodyEnd != std::wstring::npos)
        {
            tpl.insert(bodyEnd, table);
        }
    }
    
    UpdateWebView2Content(tpl.c_str());
}

// 进入公式管理模式
void EnterFormulaManagerMode()
{
    LogToFile("EnterFormulaManagerMode: 进入公式管理模式");
    g_currentViewMode = ViewMode::FORMULA_MANAGER;
    UpdateFormulaManagerWebView();
}

// 退出公式管理模式
void ExitFormulaManagerMode()
{
    LogToFile("ExitFormulaManagerMode: 退出公式管理模式");
    // 返回到计算器模式
    g_currentViewMode = ViewMode::CALCULATOR;
    g_calculatorMode = true; // 确保布尔标志也更新
    UpdateCalculatorModeWebView();
}

// 处理公式管理消息
static void HandleFormulaManagerMessage(const std::wstring& msgStr)
{
    // 简单的JSON字段提取辅助lambda
    auto extractField = [&](const std::wstring& json, const std::wstring& field) -> std::wstring {
        size_t pos = json.find(L"\"" + field + L"\":\"");
        if (pos == std::wstring::npos) return L"";
        pos += field.length() + 4;
        
        size_t end = pos;
        while (true) {
            end = json.find(L"\"", end);
            if (end == std::wstring::npos) return L"";
            
            // 检查是否转义
            size_t backslashCount = 0;
            size_t p = end;
            while (p > pos && json[p - 1] == L'\\') {
                backslashCount++;
                p--;
            }
            
            if (backslashCount % 2 == 0) break; // 偶数个反斜杠，说明引号未被转义
            end++;
        }
        
        std::wstring value = json.substr(pos, end - pos);
        
        // 处理转义字符
        // 先处理 \" -> "
        size_t replacePos = 0;
        while ((replacePos = value.find(L"\\\"", replacePos)) != std::wstring::npos) {
            value.replace(replacePos, 2, L"\"");
            replacePos += 1;
        }
        // 再处理 \\ -> \ (注意顺序，其实应该一起处理或者用状态机，这里简化处理)
        // 实际上标准的JSON反转义更复杂，这里暂且满足基本需求
        replacePos = 0;
        while ((replacePos = value.find(L"\\\\", replacePos)) != std::wstring::npos) {
            value.replace(replacePos, 2, L"\\");
            replacePos += 1;
        }
        
        return value;
    };
    
    auto extractInt = [&](const std::wstring& json, const std::wstring& field) -> int {
        size_t pos = json.find(L"\"" + field + L"\":");
        if (pos == std::wstring::npos) return -1;
        pos += field.length() + 3;
        size_t end = json.find(L",", pos);
        if (end == std::wstring::npos) end = json.find(L"}", pos);
        return _wtoi(json.substr(pos, end - pos).c_str());
    };
    
    std::wstring action = extractField(msgStr, L"action");
    
    if (action == L"addFormula")
    {
        std::wstring name = extractField(msgStr, L"name");
        std::wstring expr = extractField(msgStr, L"expression");
        std::wstring desc = extractField(msgStr, L"description");
        
        if (!name.empty() && !expr.empty())
        {
            AddCustomFormula(name, expr, desc);
            SaveCustomFormulas();
            UpdateFormulaManagerWebView();
        }
    }
    else if (action == L"editFormula")
    {
        int index = extractInt(msgStr, L"index");
        std::wstring newName = extractField(msgStr, L"name");
        std::wstring newExpr = extractField(msgStr, L"expression");
        std::wstring newDesc = extractField(msgStr, L"description");
        
        if (index >= 0 && index < (int)g_customFormulas.size() && !newName.empty() && !newExpr.empty())
        {
            const auto& oldFormula = g_customFormulas[index];
            EditCustomFormula(oldFormula.name, newName, newExpr, newDesc);
            SaveCustomFormulas();
            UpdateFormulaManagerWebView();
        }
    }
    else if (action == L"deleteFormula")
    {
        int index = extractInt(msgStr, L"index");
        if (index >= 0 && index < (int)g_customFormulas.size())
        {
            if (MessageBoxW(g_hMainWindow, L"确定要删除这个公式吗？", L"确认删除", MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                DeleteCustomFormula(index);
                SaveCustomFormulas();
                UpdateFormulaManagerWebView();
            }
        }
    }
    else if (action == L"closeFormulaManager")
    {
        ExitFormulaManagerMode();
    }
}
