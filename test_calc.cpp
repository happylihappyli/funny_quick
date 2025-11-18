#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>

// 前向声明：数学表达式解析函数
double parseNumber(const std::string& expr, size_t& pos);
double parseFactor(const std::string& expr, size_t& pos);
double parseTerm(const std::string& expr, size_t& pos);
double parseExpression(const std::string& expr, size_t& pos);

// 解析数字
double parseNumber(const std::string& expr, size_t& pos) {
    // 跳过空格
    while (pos < expr.length() && expr[pos] == ' ') {
        pos++;
    }
    
    // 解析数字
    double result = 0.0;
    bool hasDecimal = false;
    double decimalPlace = 0.1;
    bool hasNumber = false;
    
    while (pos < expr.length()) {
        char c = expr[pos];
        if (c >= '0' && c <= '9') {
            hasNumber = true;
            if (!hasDecimal) {
                result = result * 10 + (c - '0');
            } else {
                result += (c - '0') * decimalPlace;
                decimalPlace *= 0.1;
            }
        } else if (c == '.' && !hasDecimal) {
            hasDecimal = true;
        } else {
            break;
        }
        pos++;
    }
    
    if (!hasNumber) {
        throw std::runtime_error("期望数字");
    }
    
    return result;
}

// 解析项（乘除法）
double parseTerm(const std::string& expr, size_t& pos) {
    double result = parseFactor(expr, pos);
    while (pos < expr.length()) {
        if (expr[pos] == '*') {
            pos++;
            result *= parseFactor(expr, pos);
        }
        else if (expr[pos] == '/') {
            pos++;
            double divisor = parseFactor(expr, pos);
            if (divisor == 0) {
                throw std::runtime_error("除零错误");
            }
            result /= divisor;
        }
        else {
            break;
        }
    }
    return result;
}

// 解析表达式（加减法）
double parseExpression(const std::string& expr, size_t& pos) {
    double result = parseTerm(expr, pos);
    while (pos < expr.length()) {
        if (expr[pos] == '+') {
            pos++;
            result += parseTerm(expr, pos);
        }
        else if (expr[pos] == '-') {
            pos++;
            result -= parseTerm(expr, pos);
        }
        else {
            break;
        }
    }
    return result;
}

// 解析因子（数字或括号表达式）
double parseFactor(const std::string& expr, size_t& pos) {
    if (pos >= expr.length()) {
        throw std::runtime_error("意外的文件结束");
    }
    
    if (expr[pos] == '(') {
        pos++; // 跳过 '('
        double result = parseExpression(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            throw std::runtime_error("缺少右括号");
        }
        pos++; // 跳过 ')'
        return result;
    }
    else {
        return parseNumber(expr, pos);
    }
}

// JavaScript表达式计算函数
double EvaluateJavaScriptExpression(const std::string& expression) {
    // 简单的JavaScript表达式计算器
    // 支持基本数学运算：+ - * / ( )
    
    std::string expr = expression;
    std::cout << "[DEBUG] 开始计算表达式: '" << expr << "'" << std::endl;
    
    // 移除所有空格
    expr.erase(std::remove(expr.begin(), expr.end(), ' '), expr.end());
    std::cout << "[DEBUG] 移除空格后: '" << expr << "'" << std::endl;
    
    // 检查是否是help命令
    if (expr == "help" || expr == "帮助") {
        std::cout << "[DEBUG] 识别为help命令" << std::endl;
        return 0.0;  // 特殊返回值，表示显示帮助
    }
    
    // 检查是否是退出命令
    if (expr == "q" || expr == "退出") {
        std::cout << "[DEBUG] 识别为退出命令" << std::endl;
        return -1.0;  // 特殊返回值，表示退出计算模式
    }
    
    // 检查是否是js命令（进入计算模式）
    if (expr == "js" || expr == "计算") {
        std::cout << "[DEBUG] 识别为js命令" << std::endl;
        return -2.0;  // 特殊返回值，表示进入计算模式
    }
    
    try {
        // 完整的表达式解析和计算（支持括号）
        size_t pos = 0;
        std::cout << "[DEBUG] 开始解析表达式，初始位置: " << pos << std::endl;
        double result = parseExpression(expr, pos);
        std::cout << "[DEBUG] 解析结果: " << result << ", 结束位置: " << pos << ", 表达式长度: " << expr.length() << std::endl;
        
        // 检查是否还有未解析的字符
        if (pos < expr.length()) {
            std::cout << "[DEBUG] 还有未解析字符: '" << expr.substr(pos) << "'" << std::endl;
            throw std::runtime_error("表达式格式错误");
        }
        
        std::cout << "[DEBUG] 计算成功，结果: " << result << std::endl;
        return result;
    }
    catch (const std::exception& e) {
        std::cout << "[DEBUG] 计算异常: " << e.what() << std::endl;
        // 计算失败，返回NaN
        return std::numeric_limits<double>::quiet_NaN();
    }
}

int main() {
    std::cout << "=== 表达式计算测试程序 ===" << std::endl;
    std::cout << "输入'exit'退出程序" << std::endl;
    std::cout << "输入'expression'进行计算（例如: 1+1）" << std::endl;
    
    std::string input;
    bool calculatorMode = false;
    
    while (true) {
        std::cout << (calculatorMode ? "计算模式> " : "普通模式> ");
        std::getline(std::cin, input);
        
        if (input == "exit") {
            break;
        }
        
        if (input == "js" || input == "计算") {
            calculatorMode = true;
            std::cout << "进入计算模式" << std::endl;
            continue;
        }
        
        if (input == "q" || input == "退出") {
            calculatorMode = false;
            std::cout << "退出计算模式" << std::endl;
            continue;
        }
        
        if (calculatorMode) {
            double result = EvaluateJavaScriptExpression(input);
            if (!std::isnan(result)) {
                std::cout << "计算结果: " << result << std::endl;
            } else {
                std::cout << "计算失败" << std::endl;
            }
        } else {
            std::cout << "不在计算模式下" << std::endl;
        }
        
        std::cout << "---" << std::endl;
    }
    
    return 0;
}