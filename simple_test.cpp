#include <iostream>
#include <string>
#include <algorithm>

// 简化的数字解析函数
double parseNumber(const std::string& expr, size_t& pos) {
    while (pos < expr.length() && expr[pos] == ' ') pos++;
    
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

// 简化的项解析（乘除法）
double parseTerm(const std::string& expr, size_t& pos) {
    double result = parseNumber(expr, pos);
    while (pos < expr.length()) {
        if (expr[pos] == '*') {
            pos++;
            result *= parseNumber(expr, pos);
        }
        else if (expr[pos] == '/') {
            pos++;
            double divisor = parseNumber(expr, pos);
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

// 简化的表达式解析（加减法）
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

// 简化的表达式计算函数
double simpleCalc(const std::string& expression) {
    std::string expr = expression;
    std::cout << "[简单测试] 原始表达式: '" << expr << "'" << std::endl;
    
    // 移除空格
    expr.erase(std::remove(expr.begin(), expr.end(), ' '), expr.end());
    std::cout << "[简单测试] 移除空格后: '" << expr << "'" << std::endl;
    
    try {
        size_t pos = 0;
        std::cout << "[简单测试] 开始解析，位置: " << pos << std::endl;
        double result = parseExpression(expr, pos);
        std::cout << "[简单测试] 解析结果: " << result << std::endl;
        std::cout << "[简单测试] 结束位置: " << pos << ", 总长度: " << expr.length() << std::endl;
        
        if (pos < expr.length()) {
            std::cout << "[简单测试] 未解析部分: '" << expr.substr(pos) << "'" << std::endl;
            throw std::runtime_error("表达式格式错误");
        }
        
        return result;
    }
    catch (const std::exception& e) {
        std::cout << "[简单测试] 异常: " << e.what() << std::endl;
        return 0.0;
    }
}

int main() {
    std::cout << "=== 简化表达式计算测试 ===" << std::endl;
    
    // 测试案例
    std::cout << "\n--- 测试 1+1 ---" << std::endl;
    double result1 = simpleCalc("1+1");
    std::cout << "结果: " << result1 << std::endl;
    
    std::cout << "\n--- 测试 2*3 ---" << std::endl;
    double result2 = simpleCalc("2*3");
    std::cout << "结果: " << result2 << std::endl;
    
    std::cout << "\n--- 测试 10/2 ---" << std::endl;
    double result3 = simpleCalc("10/2");
    std::cout << "结果: " << result3 << std::endl;
    
    std::cout << "\n--- 测试 1+2*3 ---" << std::endl;
    double result4 = simpleCalc("1+2*3");
    std::cout << "结果: " << result4 << std::endl;
    
    std::cout << "\n测试完成！" << std::endl;
    return 0;
}