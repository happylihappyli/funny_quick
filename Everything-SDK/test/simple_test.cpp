// 简单测试程序 - 验证编译环境
#include <iostream>
#include <string>

/**
 * @brief 主函数 - 测试程序入口
 * @return int 程序退出码
 */
int main() {
    std::cout << "Hello, World! 编译环境测试成功！" << std::endl;
    std::cout << "当前系统: Windows 11" << std::endl;
    std::cout << "编译器: Microsoft Visual C++" << std::endl;
    
    // 测试UTF-8支持
    std::string chinese = "中文测试 - 你好世界！";
    std::cout << chinese << std::endl;
    
    return 0;
}