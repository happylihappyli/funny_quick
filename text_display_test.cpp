#include <iostream>
#include <string>
#include <windows.h>
#include <locale>
#include <codecvt>

// 测试UTF-8转宽字符串函数
std::wstring UTF8ToWide(const std::string& utf8Str) {
    if (utf8Str.empty()) return L"";
    
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, NULL, 0);
    if (wideSize <= 0) return L"";
    
    wchar_t* wideStr = new wchar_t[wideSize];
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, wideStr, wideSize);
    
    std::wstring result(wideStr);
    delete[] wideStr;
    return result;
}

int main() {
    // 设置控制台为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    std::cout << "=== 文本显示测试 ===" << std::endl;
    
    // 测试各种字符串
    std::string testStrings[] = {
        "1+1 = 2",
        "Hello World",
        "计算结果",
        "2*3 = 6",
        "10/2 = 5",
        "(1+2)*3 = 9"
    };
    
    for (const auto& str : testStrings) {
        std::cout << "原始字符串: " << str << std::endl;
        
        // 转换为宽字符串
        std::wstring wideStr = UTF8ToWide(str);
        std::wcout << L"宽字符串: " << wideStr << std::endl;
        
        // 检查转换是否成功
        if (!wideStr.empty()) {
            std::cout << "转换成功，长度: " << wideStr.length() << std::endl;
        } else {
            std::cout << "转换失败！" << std::endl;
        }
        
        // 尝试转回UTF-8
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
        if (utf8Size > 0) {
            char* utf8Buffer = new char[utf8Size];
            WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, utf8Buffer, utf8Size, NULL, NULL);
            std::string roundTrip(utf8Buffer);
            std::cout << "往返转换: " << roundTrip << std::endl;
            delete[] utf8Buffer;
        }
        
        std::cout << "---" << std::endl;
    }
    
    std::cout << "测试完成！" << std::endl;
    return 0;
}