#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>

// 复制的核心函数

// 解析数字
double parseNumber(const std::string& expr, size_t& pos) {
    while (pos < expr.length() && expr[pos] == ' ') {
        pos++;
    }
    
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
    std::cout << "=== 测试核心计算函数 ===" << std::endl;
    std::cout << "[DEBUG] 开始计算表达式: '" << expression << "'" << std::endl;
    
    // 移除所有空格
    std::string expr = expression;
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
            std::cout << "[DEBUG] 还有未解析字符: " << expr.substr(pos) << std::endl;
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
    std::cout << "=== 核心表达式计算函数测试 ===" << std::endl;
    
    // 测试用例
    std::string testExpressions[] = {
        "1+1",
        "2*3", 
        "10/2",
        "1+2*3",
        "(1+2)*3",
        "js",
        "help",
        "q"
    };
    
    for (const auto& expr : testExpressions) {
        std::cout << "\n--- 测试表达式: " << expr << " ---" << std::endl;
        double result = EvaluateJavaScriptExpression(expr);
        std::cout << "最终结果: " << result;
        if (std::isnan(result)) {
            std::cout << " (NaN - 计算失败)";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n测试完成！" << std::endl;
    return 0;
}