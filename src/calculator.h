#pragma once

#include <windows.h>
#include <string>
#include <vector>

/**
 * @brief 计算记录结构体
 */
struct CalculationRecord
{
    std::wstring expression;  // 表达式
    std::wstring result;      // 计算结果
    std::wstring timestamp;   // 时间戳
    std::wstring comment;     // 备注
};

/**
 * @brief 自定义公式结构体
 */
struct CustomFormula
{
    std::wstring name;        // 公式名称
    std::wstring expression;  // 公式表达式
    std::wstring description; // 公式描述
};

// 外部函数声明（在gui_main.cpp中定义）
void UpdateListViewColumns();
void ClearListView();
void AddHintRowToListView(const WCHAR* hintText);
void UpdateCalculatorModeWebView();

// 自定义公式全局变量
extern std::vector<CustomFormula> g_customFormulas;

/**
 * @brief 加载自定义公式
 */
void LoadCustomFormulas();

/**
 * @brief 保存自定义公式
 */
void SaveCustomFormulas();

/**
 * @brief 添加自定义公式
 * @param name 名称
 * @param expression 表达式
 * @param description 描述
 */
void AddCustomFormula(const std::wstring& name, const std::wstring& expression, const std::wstring& description);

/**
 * @brief 删除自定义公式
 * @param index 索引
 */
void DeleteCustomFormula(int index);

/**
 * @brief 删除自定义公式 (按名称)
 * @param name 公式名称
 */
void DeleteCustomFormulaByName(const std::wstring& name);

/**
 * @brief 删除计算历史记录
 * @param index 索引
 */
void DeleteCalculationHistory(int index);

/**
 * @brief 编辑自定义公式
 * @param oldName 旧名称
 * @param newName 新名称
 * @param newExpression 新表达式
 * @param newDescription 新描述
 */
void EditCustomFormula(const std::wstring& oldName, const std::wstring& newName, const std::wstring& newExpression, const std::wstring& newDescription);

/**
 * @brief 表达式解析辅助函数 - 解析数字
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的数字
 */
double parseNumber(const std::wstring& expr, size_t& pos);
double parseFactor(const std::wstring& expr, size_t& pos);

/**
 * @brief 表达式解析辅助函数 - 解析项（乘除法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的项值
 */
double parseTerm(const std::wstring& expr, size_t& pos);

/**
 * @brief 表达式解析辅助函数 - 解析表达式（加减法）
 * @param expr 表达式字符串
 * @param pos 解析位置（引用，会被修改）
 * @return 解析得到的表达式值
 */
double parseExpression(const std::wstring& expr, size_t& pos);

/**
 * @brief 检查字符串是否是数学表达式
 * @param expression 要检查的字符串
 * @return 如果是数学表达式返回true，否则返回false
 */
bool IsMathExpression(const std::wstring& expression);

/**
 * @brief 计算数学表达式
 * @param expression 要计算的表达式
 * @return 计算结果字符串
 */
std::wstring CalculateExpression(const std::wstring& expression);

/**
 * @brief 评估表达式并处理计算逻辑
 * @param expression 要评估的表达式
 */
void EvaluateExpression(const WCHAR* expression);

void OnCalculationResult(const std::wstring& expression, const std::wstring& result);

/**
 * @brief 进入计算模式
 */
void EnterCalculatorMode();

/**
 * @brief 退出计算模式
 */
void ExitCalculatorMode();

/**
 * @brief 显示计算历史记录
 */
void DisplayCalculationHistory();

/**
 * @brief 保存计算历史记录到文件
 */
void SaveCalculationHistory();

/**
 * @brief 从文件加载计算历史记录
 */
void LoadCalculationHistory();

/**
 * @brief 显示计算器帮助信息
 */
void ShowCalculatorHelpInfo();
