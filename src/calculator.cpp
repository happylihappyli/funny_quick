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

// 全局变量声明（在gui_main.cpp中定义）
extern HWND g_hListView;
extern HWND g_hEdit;
extern bool g_calculatorMode;
extern std::vector<CalculationRecord> g_calculationHistory;

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

/**
 * @brief 表达式解析辅助函数 - 解析项（乘除法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的项值
 */
double parseTerm(const std::wstring& expr, size_t& pos)
{
    double value = parseNumber(expr, pos);
    
    while (pos < expr.length() && (expr[pos] == L'*' || expr[pos] == L'/'))
    {
        wchar_t op = expr[pos];
        pos++;
        double nextValue = parseNumber(expr, pos);
        
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