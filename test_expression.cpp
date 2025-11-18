#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <stdexcept>

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

// 解析因子（数字或括号表达式）
double parseFactor(const std::string& expr, size_t& pos) {
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

// JavaScript表达式计算函数
double EvaluateJavaScriptExpression(const std::string& expression) {
    // 简单的JavaScript表达式计算器
    // 支持基本数学运算：+ - * / ( )
    
    std::string expr = expression;
    
    // 移除所有空格
    expr.erase(std::remove(expr.begin(), expr.end(), ' '), expr.end());
    
    // 检查是否是help命令
    if (expr == "help" || expr == "帮助") {
        return 0.0;  // 特殊返回值，表示显示帮助
    }
    
    // 检查是否是退出命令
    if (expr == "q" || expr == "退出") {
        return -1.0;  // 特殊返回值，表示退出计算模式
    }
    
    // 检查是否是js命令（进入计算模式）
    if (expr == "js" || expr == "计算") {
        return -2.0;  // 特殊返回值，表示进入计算模式
    }
    
    try {
        // 完整的表达式解析和计算（支持括号）
        size_t pos = 0;
        double result = parseExpression(expr, pos);
        
        // 检查是否还有未解析的字符
        if (pos < expr.length()) {
            throw std::runtime_error("表达式格式错误");
        }
        
        return result;
    }
    catch (const std::exception&) {
        // 计算失败，返回NaN
        return std::numeric_limits<double>::quiet_NaN();
    }
}

int main() {
    // 测试各种表达式
    std::vector<std::string> testExpressions = {
        "1+1",
        "2*3",
        "10/2",
        "2+3*4",
        "(1+2)*3",
        "1.5+2.3",
        "10-3+1"
    };
    
    std::cout << "测试表达式解析功能:" << std::endl;
    std::cout << "==================" << std::endl;
    
    for (const auto& expr : testExpressions) {
        double result = EvaluateJavaScriptExpression(expr);
        if (std::isnan(result)) {
            std::cout << expr << " = 计算错误" << std::endl;
        } else {
            std::cout << expr << " = " << result << std::endl;
        }
    }
    
    return 0;
}