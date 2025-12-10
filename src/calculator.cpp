#include "calculator.h"
#include "common.h"
#include "logger.h"
#include <windows.h>
#include <shlobj.h>    // 包含CSIDL_APPDATA和SHGetFolderPathW定义
#include <commctrl.h>  // 包含列表视图控件相关定义
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>

// 全局变量声明（在gui_main.cpp中定义）
extern HWND g_hListView;
extern HWND g_hEdit;
extern bool g_calculatorMode;
extern std::vector<CalculationRecord> g_calculationHistory;

// 前向声明
extern void UpdateWindowTitle();

/**
 * @brief 表达式解析辅助函数 - 解析数字
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的数字
 */
double parseNumber(const std::wstring& expr, size_t& pos)
{
    std::wstring numStr;
    while (pos < expr.length() && (iswdigit(expr[pos]) || expr[pos] == L'.'))
    {
        numStr += expr[pos];
        pos++;
    }
    return numStr.empty() ? 0.0 : _wtof(numStr.c_str());
}

double parseFactor(const std::wstring& expr, size_t& pos)
{
    if (pos >= expr.length()) return 0.0;
    if (expr[pos] == L'+') { pos++; return parseFactor(expr, pos); }
    if (expr[pos] == L'-') { pos++; return -parseFactor(expr, pos); }
    if (expr[pos] == L'(') { pos++; double v = parseExpression(expr, pos); if (pos < expr.length() && expr[pos] == L')') pos++; return v; }
    if (iswalpha(expr[pos])) {
        std::wstring id;
        while (pos < expr.length() && (iswalpha(expr[pos]) || expr[pos] == L'_')) { id += expr[pos]; pos++; }
        if (pos < expr.length() && expr[pos] == L'(') {
            pos++;
            double arg = parseExpression(expr, pos);
            if (pos < expr.length() && expr[pos] == L')') pos++;
            if (id == L"sin") return std::sin(arg);
            if (id == L"cos") return std::cos(arg);
            if (id == L"tan") return std::tan(arg);
            if (id == L"sqrt") return std::sqrt(arg);
            if (id == L"abs") return std::fabs(arg);
            return 0.0;
        }
        if (id == L"pi") return 3.1415926;
        return 0.0;
    }
    return parseNumber(expr, pos);
}

/**
 * @brief 表达式解析辅助函数 - 解析项（乘除法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的项值
 */
double parseTerm(const std::wstring& expr, size_t& pos)
{
    double value = parseFactor(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'*' || expr[pos] == L'/'))
    {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseFactor(expr, pos);
        
        if (op == L'*')
        {
            value *= nextValue;
        }
        else if (op == L'/' && nextValue != 0)
        {
            value /= nextValue;
        }
        else
        {
            LogToFile("parseTerm: 除零错误");
            throw std::exception("除零错误");
        }
    }
    
    return value;
}

/**
 * @brief 表达式解析辅助函数 - 解析表达式（加减法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的表达式值
 */
double parseExpression(const std::wstring& expr, size_t& pos)
{
    double value = parseTerm(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'+' || expr[pos] == L'-'))
    {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseTerm(expr, pos);
        
        if (op == L'+')
        {
            value += nextValue;
        }
        else
        {
            value -= nextValue;
        }
    }
    
    return value;
}

/**
 * @brief 检查字符串是否是数学表达式
 * @param expression 要检查的字符串
 * @return 如果是数学表达式返回true，否则返回false
 */
bool IsMathExpression(const std::wstring& expression)
{
    // 简单的数学表达式检查：包含数字和运算符
    if (expression.empty()) return false;
    
    // 检查是否包含数学运算符
    const std::wstring operators = L"+-*/^%()";
    for (wchar_t c : expression)
    {
        if (operators.find(c) != std::wstring::npos)
        {
            return true;
        }
    }
    
    // 检查是否包含数字
    for (wchar_t c : expression)
    {
        if (c >= L'0' && c <= L'9')
        {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 计算数学表达式
 * @param expression 要计算的表达式
 * @return 计算结果字符串
 */
std::wstring CalculateExpression(const std::wstring& expression)
{
    // 简单的表达式计算实现
    // 这里应该使用更复杂的数学表达式解析器
    // 目前只实现基本的四则运算
    
    try
    {
        // 将wstring转换为string
        std::string expr(expression.begin(), expression.end());
        
        // 这里应该使用数学表达式计算库
        // 暂时返回一个简单的计算结果
        return L"计算结果: " + expression;
    }
    catch (...)
    {
        return L"计算错误";
    }
}

/**
 * @brief 评估表达式并处理计算逻辑
 * @param expression 要评估的表达式
 */


/**
 * @brief 进入计算模式
 */
void EnterCalculatorMode()
{
    g_calculatorMode = true;
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 显示计算历史记录
    DisplayCalculationHistory();
    
    // 添加提示信息
    AddHintRowToListView(L"💡 计算模式：输入数学表达式进行计算，以#开头添加注释");
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新计算模式WebView显示
    UpdateCalculatorModeWebView();
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    LogToFile("EnterCalculatorMode: 进入计算模式");
}

/**
 * @brief 退出计算模式
 */
void ExitCalculatorMode()
{
    g_calculatorMode = false;
    
    // 更新ListView列标题
    UpdateListViewColumns();
    
    // 清空ListView
    ClearListView();
    
    // 清空编辑框
    SetWindowTextW(g_hEdit, L"");
    
    // 更新窗口标题
    UpdateWindowTitle();
    
    LogToFile("ExitCalculatorMode: 退出计算模式");
}

/**
 * @brief 显示计算历史记录
 */
void DisplayCalculationHistory()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return;
    }
    
    // 清空ListView
    ClearListView();
    
    // 添加历史记录到ListView
    for (const auto& record : g_calculationHistory)
    {
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = ListView_GetItemCount(g_hListView);
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(record.expression.c_str());
        ListView_InsertItem(g_hListView, &lvItem);
        
        // 设置第二列（结果）
        lvItem.iSubItem = 1;
        lvItem.pszText = const_cast<LPWSTR>(record.result.c_str());
        ListView_SetItem(g_hListView, &lvItem);
    }
    
    LogToFile("DisplayCalculationHistory: 显示计算历史记录");
}

/**
 * @brief 保存计算历史记录到文件
 */
void SaveCalculationHistory()
{
    // 创建数据目录
    WCHAR dataPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, dataPath) == S_OK)
    {
        wcscat_s(dataPath, L"\\FunnyQuick");
        CreateDirectoryW(dataPath, NULL);
        
        // 创建历史文件路径
        WCHAR historyPath[MAX_PATH];
        wcscpy_s(historyPath, dataPath);
        wcscat_s(historyPath, L"\\calculation_history.txt");
        
        // 打开文件进行写入
        std::wofstream file(historyPath);
        if (file.is_open())
        {
            for (const auto& record : g_calculationHistory)
            {
                file << record.timestamp << L"|" << record.expression << L"|" << record.result << std::endl;
            }
            file.close();
            LogToFile("SaveCalculationHistory: 成功保存计算历史记录");
        }
        else
        {
            LogToFile("SaveCalculationHistory: 无法打开历史文件进行写入");
        }
    }
    else
    {
        LogToFile("SaveCalculationHistory: 无法获取应用数据目录路径");
    }
}

/**
 * @brief 从文件加载计算历史记录
 */
void LoadCalculationHistory()
{
    // 清空当前历史记录
    g_calculationHistory.clear();
    
    // 获取数据目录
    WCHAR dataPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, dataPath) == S_OK)
    {
        wcscat_s(dataPath, L"\\FunnyQuick");
        
        // 创建历史文件路径
        WCHAR historyPath[MAX_PATH];
        wcscpy_s(historyPath, dataPath);
        wcscat_s(historyPath, L"\\calculation_history.txt");
        
        // 打开文件进行读取
        std::wifstream file(historyPath);
        if (file.is_open())
        {
            std::wstring line;
            while (std::getline(file, line))
            {
                // 解析每行数据
                size_t pos1 = line.find(L'|');
                size_t pos2 = line.find(L'|', pos1 + 1);
                
                if (pos1 != std::wstring::npos && pos2 != std::wstring::npos)
                {
                    CalculationRecord record;
                    record.timestamp = line.substr(0, pos1);
                    record.expression = line.substr(pos1 + 1, pos2 - pos1 - 1);
                    record.result = line.substr(pos2 + 1);
                    
                    g_calculationHistory.push_back(record);
                }
            }
            file.close();
            LogToFile("LoadCalculationHistory: 成功加载计算历史记录");
        }
        else
        {
            LogToFile("LoadCalculationHistory: 无法打开历史文件进行读取");
        }
    }
    else
    {
        LogToFile("LoadCalculationHistory: 无法获取应用数据目录路径");
    }
}

/**
 * @brief 显示计算器帮助信息
 */
void ShowCalculatorHelpInfo()
{
    // 清空ListView
    ClearListView();
    
    // 添加帮助信息
    const WCHAR* helpItems[] = {
        L"💡 计算模式使用说明",
        L"• 输入数学表达式进行计算（如：2+3*4）",
        L"• 以#开头添加注释（如：#这是注释）",
        L"• 支持基本的四则运算：+ - * /",
        L"• 计算结果会自动保存到历史记录",
        L"• 点击'退出计算'返回普通模式"
    };
    
    for (int i = 0; i < sizeof(helpItems) / sizeof(helpItems[0]); i++)
    {
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = const_cast<LPWSTR>(helpItems[i]);
        ListView_InsertItem(g_hListView, &lvItem);
    }
    
    LogToFile("ShowCalculatorHelpInfo: 显示计算器帮助信息");
}

/**
 * @brief 评估数学表达式并计算结果
 * @param expression 要计算的表达式字符串
 */
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
        
        // 检查并提取注释内容（#后面的内容）
        std::wstring comment;
        size_t hashPos = expr.find(L'#');
        if (hashPos != std::wstring::npos) {
            comment = expr.substr(hashPos + 1);
            expr = expr.substr(0, hashPos);
            // 移除注释前后的空格
            size_t start = comment.find_first_not_of(L' ');
            size_t end = comment.find_last_not_of(L' ');
            if (start != std::wstring::npos && end != std::wstring::npos) {
                comment = comment.substr(start, end - start + 1);
            } else {
                comment = L"";
            }
            
            char commentLog[1024] = {0};
            WideCharToMultiByte(CP_UTF8, 0, comment.c_str(), -1, commentLog, sizeof(commentLog), NULL, NULL);
            sprintf(logMsg, "EvaluateExpression: 提取到注释 '%s'", commentLog);
            LogToFile(logMsg);
        }
        
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
            swprintf(resultStr, sizeof(resultStr)/sizeof(WCHAR), L"%.6g", result);
            LogToFile("EvaluateExpression: 创建了结果字符串");
            
            // 创建历史记录条目（只使用去除注释的表达式）
            std::wstring displayExpr = expr;  // 使用去除注释的表达式
            displayExpr += L" = ";
            displayExpr += resultStr;
            LogToFile("EvaluateExpression: 创建了历史记录条目");
            
            // 创建历史记录结构体，包含完整表达式（表达式+结果）和注释
            CalculationRecord record;
            record.expression = displayExpr;  // 使用包含结果的完整表达式（不包含注释）
            record.result = resultStr;
            record.comment = comment;
            LogToFile("EvaluateExpression: 创建了计算记录结构体");
            
            // 添加到计算历史
            g_calculationHistory.push_back(record);
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
            
            // 更新计算模式WebView显示
            LogToFile("EvaluateExpression: 准备更新WebView显示");
            UpdateCalculatorModeWebView();
            LogToFile("EvaluateExpression: 更新了WebView显示");
            
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
