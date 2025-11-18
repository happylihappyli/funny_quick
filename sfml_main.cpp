// SFML UI版本快速启动器
// 使用SFML库创建跨平台UI界面

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobj.h>  // 添加CSIDL常量定义

// 全局变量定义
sf::RenderWindow* g_window = nullptr;
sf::Font g_font;
sf::Text g_searchText;
sf::RectangleShape g_searchBox;
sf::RectangleShape g_listBox;
std::vector<sf::Text> g_listItems;
std::vector<std::string> g_searchResults;

// 选择项背景矩形
sf::RectangleShape g_selectionBackground;

// 光标相关变量
sf::RectangleShape g_cursor;          // 光标矩形
sf::Clock g_cursorClock;              // 光标闪烁时钟
bool g_cursorVisible = true;          // 光标是否可见
float g_cursorBlinkInterval = 0.5f;   // 光标闪烁间隔（秒）
std::string currentSearch;

// 快捷方式结构体
struct ShortcutItem {
    std::string name;
    std::string path;
    int type; // 0: 目录, 1: URL, 2: 应用程序
    int usageCount;
};

std::vector<ShortcutItem> g_shortcuts;

// 选择索引变量
int g_selectedIndex = -1;  // -1表示没有选择

// 计算模式相关变量
bool g_calculatorMode = false;  // 是否处于计算模式
sf::Text g_calculationResult;  // 计算结果文本
std::vector<std::string> g_calculationHistory;  // 计算历史记录

// 模式背景色
sf::Color g_normalBackgroundColor = sf::Color::White;      // 普通模式背景色
sf::Color g_calculatorBackgroundColor = sf::Color(240, 248, 255);  // 计算模式背景色（浅蓝色）

// 滚动相关变量
int g_scrollOffset = 0;  // 当前滚动偏移量
int g_maxVisibleItems = 10;  // 列表框最多显示的项目数

// 前向声明：数学表达式解析函数
double parseNumber(const std::string& expr, size_t& pos);
double parseFactor(const std::string& expr, size_t& pos);
double parseTerm(const std::string& expr, size_t& pos);
double parseExpression(const std::string& expr, size_t& pos);

// 将UTF-8字符串转换为宽字符串（用于SFML显示）
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

// 日志函数
void LogToFile(const std::string& message) {
    // 创建日志目录
    CreateDirectoryA("log", NULL);
    
    // 生成带时间戳的日志文件名
    SYSTEMTIME st;
    GetLocalTime(&st);
    char filename[256];
    sprintf_s(filename, sizeof(filename), "log\\quick_launcher_%04d%02d%02d_%02d%02d%02d.log", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    // 写入日志
    FILE* file = nullptr;
    errno_t err = fopen_s(&file, filename, "a");
    if (err == 0 && file) {
        char timestamp[64];
        sprintf_s(timestamp, sizeof(timestamp), "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        
        fprintf(file, "%s%s\n", timestamp, message.c_str());
        fclose(file);
    }
    // 同时输出到控制台
    std::cout << message << std::endl;
}

// 初始化字体
bool InitializeFont() {
    // 尝试加载系统字体（按优先级排序）
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",      // 微软雅黑
        "C:\\Windows\\Fonts\\simhei.ttf",     // 黑体
        "C:\\Windows\\Fonts\\simsun.ttc",     // 宋体
        "C:\\Windows\\Fonts\\simkai.ttf",     // 楷体
        "C:\\Windows\\Fonts\\microsoft yahei ui.ttf", // 微软雅黑UI
        "C:\\Windows\\Fonts\\msyhbd.ttc",    // 微软雅黑粗体
        "C:\\Windows\\Fonts\\msyhl.ttc",     // 微软雅黑细体
        "C:\\Windows\\Fonts\\arial.ttf",      // Arial（英文）
        "C:\\Windows\\Fonts\\tahoma.ttf",     // Tahoma
        "C:\\Windows\\Fonts\\segoeui.ttf",   // Segoe UI
        nullptr
    };
    
    for (int i = 0; fontPaths[i] != nullptr; i++) {
        if (g_font.loadFromFile(fontPaths[i])) {
            std::cout << "成功加载字体: " << fontPaths[i] << std::endl;
            return true;
        }
    }
    
    // 如果系统字体加载失败，尝试使用SFML内置字体
    std::cout << "警告：无法加载系统字体，尝试使用默认字体\n";
    
    // 创建默认字体（如果系统字体都不可用）
    if (!g_font.loadFromFile("")) {
        std::cout << "错误：无法创建默认字体\n";
        return false;
    }
    
    return true;
}

// 进入计算模式
void EnterCalculatorMode() {
    std::cout << "进入计算模式" << std::endl;
    LogToFile("进入计算模式");
    g_calculatorMode = true;
    g_calculationHistory.clear();
    
    // 添加欢迎信息到历史记录
    g_calculationHistory.push_back("=== 欢迎使用计算模式 ===");
    g_calculationHistory.push_back("输入数学表达式进行计算");
    g_calculationHistory.push_back("输入'help'查看帮助信息");
    g_calculationHistory.push_back("输入'q'退出计算模式");
    
    // 切换到计算模式背景色
    g_listBox.setFillColor(sf::Color(245, 250, 255)); // 计算模式列表框背景色
    
    // 清空搜索结果以便显示计算界面
    g_listItems.clear();
    g_searchResults.clear();
    
    // 在计算模式下显示提示信息，不调用ShowCalculatorHelp避免覆盖计算结果
    sf::Text promptItem;
    promptItem.setFont(g_font);
    promptItem.setCharacterSize(14);
    promptItem.setFillColor(sf::Color::Blue);
    promptItem.setString(L"提示: 输入表达式后按回车进行计算");
    promptItem.setPosition(25, 65);
    g_listItems.push_back(promptItem);
    g_searchResults.push_back("提示: 输入表达式后按回车进行计算");
}

// 退出计算模式
void ExitCalculatorMode() {
    g_calculatorMode = false;
    LogToFile("ExitCalculatorMode: 退出计算模式");
}

// 显示帮助信息
void ShowHelp() {
    // 清空当前显示
    g_listItems.clear();
    g_searchResults.clear();
    
    // 添加帮助信息
    std::vector<std::string> helpItems = {
        "=== 快速启动器帮助 ===",
        "",
        "基本操作:",
        "• 输入关键词搜索快捷方式",
        "• 按上下键选择项目",
        "• 按Enter键或双击执行项目",
        "• 按ESC键退出程序",
        "",
        "计算模式:",
        "• 输入'js'进入计算模式",
        "• 输入数学表达式进行计算",
        "• 支持: + - * / ( ) 和小数",
        "• 输入'help'查看详细帮助",
        "• 输入'q'退出计算模式",
        "",
        "常用快捷方式:",
        "• 记事本、画图、计算器",
        "• 命令提示符、PowerShell",
        "• 桌面快捷方式",
        "• 系统文件夹"
    };
    
    // 创建帮助文本项
    for (size_t i = 0; i < helpItems.size(); i++) {
        sf::Text item;
        item.setFont(g_font);
        item.setCharacterSize(14);
        
        // 设置不同颜色区分标题和内容
            if (helpItems[i].find("===") != std::string::npos) {
                item.setFillColor(sf::Color::Blue);  // 标题蓝色
            } else if (helpItems[i].empty()) {
                item.setFillColor(sf::Color::Black);  // 空行黑色
            } else if (helpItems[i].find("•") == 0) {
                item.setFillColor(sf::Color(0, 100, 0));  // 列表项深绿色
            } else {
                item.setFillColor(sf::Color::Black);  // 普通文本黑色
            }
        
        item.setString(UTF8ToWide(helpItems[i]));
        item.setPosition(25, 65 + static_cast<float>(i * 20));
        g_listItems.push_back(item);
        g_searchResults.push_back(helpItems[i]);
    }
    
    // 重置滚动偏移
    g_scrollOffset = 0;
    
    LogToFile("ShowHelp: 显示帮助信息");
}

// 显示计算模式帮助
void ShowCalculatorHelp() {
    // 清空当前显示
    g_listItems.clear();
    g_searchResults.clear();
    
    // 添加计算模式帮助信息
    std::vector<std::string> helpItems = {
        "=== 计算模式帮助 ===",
        "",
        "基本运算:",
        "• 加法: 2 + 3",
        "• 减法: 5 - 2",
        "• 乘法: 3 * 4",
        "• 除法: 10 / 2",
        "• 混合运算: 2 + 3 * 4",
        "",
        "高级功能:",
        "• 括号: (2 + 3) * 4",
        "• 小数: 3.14 * 2",
        "• 负数: -5 + 3",
        "",
        "示例:",
        "• 简单计算: 1+2",
        "• 复杂计算: (3+4)*5/2",
        "• 带小数: 3.14*2",
        "",
        "操作说明:",
        "• 输入表达式后按Enter计算",
        "• 计算结果会显示在列表中",
        "• 输入'q'退出计算模式",
        "• 输入'help'再次查看帮助"
    };
    
    // 创建帮助文本项
    for (size_t i = 0; i < helpItems.size(); i++) {
        sf::Text item;
        item.setFont(g_font);
        item.setCharacterSize(14);
        
        // 设置不同颜色区分标题和内容
        if (helpItems[i].find("===") != std::string::npos) {
            item.setFillColor(sf::Color::Blue);  // 标题蓝色
        } else if (helpItems[i].empty()) {
            item.setFillColor(sf::Color::Black);  // 空行黑色
        } else if (helpItems[i].find("•") == 0) {
            item.setFillColor(sf::Color(0, 100, 0));  // 列表项深绿色
        } else {
            item.setFillColor(sf::Color::Black);  // 普通文本黑色
        }
        
        item.setString(UTF8ToWide(helpItems[i]));
        item.setPosition(25, 65 + static_cast<float>(i * 20));
        g_listItems.push_back(item);
        g_searchResults.push_back(helpItems[i]);
    }
    
    LogToFile("ShowCalculatorHelp: 显示计算模式帮助");
}

// 显示计算结果
void DisplayCalculationResult(const std::string& expression, double result) {
    std::string resultStr;
    if (!expression.empty()) {
        resultStr = expression + " = " + std::to_string(result);
        if (resultStr.find('.') != std::string::npos) {
            // 去除小数点后的无意义零
            resultStr = resultStr.substr(0, resultStr.find_last_not_of('0') + 1);
            if (resultStr.back() == '.') {
                resultStr = resultStr.substr(0, resultStr.length() - 1);
            }
        }
    } else {
        resultStr = "=== 计算器模式 ===";
    }
    
    // 清空当前显示
    g_listItems.clear();
    g_searchResults.clear();
    
    // 显示计算结果
    sf::Text resultItem;
    resultItem.setFont(g_font);
    resultItem.setCharacterSize(14);
    resultItem.setFillColor(sf::Color::Green);
    resultItem.setString(UTF8ToWide("=== 计算结果 ==="));
    resultItem.setPosition(25, 65);
    g_listItems.push_back(resultItem);
    g_searchResults.push_back("=== 计算结果 ===");
    
    // 清空搜索框，显示计算结果
    currentSearch.clear();
    g_searchText.setString(L"");
    
    sf::Text calculationItem;
    calculationItem.setFont(g_font);
    calculationItem.setCharacterSize(16);
    calculationItem.setFillColor(sf::Color::Black);
    calculationItem.setString(UTF8ToWide(resultStr));
    calculationItem.setPosition(25, 85);
    g_listItems.push_back(calculationItem);
    g_searchResults.push_back(resultStr);
    
    // 添加计算历史（最多保存5条）
    g_calculationHistory.push_back(resultStr);
    if (g_calculationHistory.size() > 5) {
        g_calculationHistory.erase(g_calculationHistory.begin());
    }
    
    // 显示历史记录
    sf::Text historyItem;
    historyItem.setFont(g_font);
    historyItem.setCharacterSize(14);
    historyItem.setFillColor(sf::Color::Blue);
    historyItem.setString(UTF8ToWide("=== 计算历史 ==="));
    historyItem.setPosition(25, 105);
    g_listItems.push_back(historyItem);
    g_searchResults.push_back("=== 计算历史 ===");
    
    for (size_t i = 0; i < g_calculationHistory.size(); i++) {
        sf::Text historyEntry;
        historyEntry.setFont(g_font);
        historyEntry.setCharacterSize(12);
        historyEntry.setFillColor(sf::Color(128, 128, 128));  // 使用RGB值代替Gray
        historyEntry.setString(UTF8ToWide(g_calculationHistory[i]));
        historyEntry.setPosition(25, 125 + static_cast<float>(i * 20));
        g_listItems.push_back(historyEntry);
        g_searchResults.push_back(g_calculationHistory[i]);
    }
}

// 初始化UI组件
void InitializeUI() {
    // 创建搜索框
    g_searchBox.setSize(sf::Vector2f(360, 30));
    g_searchBox.setPosition(20, 20);
    g_searchBox.setFillColor(sf::Color(240, 240, 240));
    g_searchBox.setOutlineColor(sf::Color(180, 180, 180));
    g_searchBox.setOutlineThickness(2);
    
    // 创建搜索文本
    g_searchText.setFont(g_font);
    g_searchText.setCharacterSize(16);
    g_searchText.setFillColor(sf::Color::Black);
    g_searchText.setPosition(25, 25);
    g_searchText.setString(L"");
    
    // 创建列表框（增加高度以显示更多内容）
    g_listBox.setSize(sf::Vector2f(360, 220));
    g_listBox.setPosition(20, 60);
    g_listBox.setFillColor(sf::Color(250, 250, 250));
    g_listBox.setOutlineColor(sf::Color(180, 180, 180));
    g_listBox.setOutlineThickness(2);
    
    // 初始化光标
    g_cursor.setSize(sf::Vector2f(2, 20));  // 光标宽度2像素，高度20像素
    g_cursor.setFillColor(sf::Color::Black);
    g_cursor.setPosition(25, 25);  // 初始位置与搜索文本相同
    
    // 初始化选择项背景
    g_selectionBackground.setSize(sf::Vector2f(360, 20));
    g_selectionBackground.setFillColor(sf::Color(0, 120, 215));  // 蓝色背景
    g_selectionBackground.setPosition(20, 65);  // 初始位置
    
    // 初始化计算结果文本
    g_calculationResult.setFont(g_font);
    g_calculationResult.setCharacterSize(14);
    g_calculationResult.setFillColor(sf::Color(0, 100, 0)); // DarkGreen颜色
    g_calculationResult.setPosition(20, 120);
}

// 添加桌面快捷方式
void AddDesktopShortcuts() {
    wchar_t desktopPath[MAX_PATH] = {0};
    
    // 获取桌面路径（使用宽字符版本）
    if (SHGetSpecialFolderPathW(NULL, desktopPath, CSIDL_DESKTOP, FALSE)) {
        wchar_t searchPath[MAX_PATH] = {0};
        swprintf_s(searchPath, sizeof(searchPath)/sizeof(wchar_t), L"%s\\*.lnk", desktopPath);
        
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath, &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                // 跳过目录
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    ShortcutItem shortcut;
                    
                    // 提取名称（不含扩展名）
                    wchar_t* dotPos = wcsrchr(findData.cFileName, L'.');
                    if (dotPos) {
                        *dotPos = L'\0';  // 移除.lnk扩展名
                    }
                    
                    // 将宽字符文件名转换为UTF-8字符串
                    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, NULL, 0, NULL, NULL);
                    if (utf8Size > 0) {
                        char* utf8Name = new char[utf8Size];
                        WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, utf8Name, utf8Size, NULL, NULL);
                        shortcut.name = utf8Name;
                        delete[] utf8Name;
                    } else {
                        shortcut.name = "未知快捷方式";
                    }
                    
                    // 设置快捷方式路径
                    wchar_t fullPath[MAX_PATH];
                    swprintf_s(fullPath, sizeof(fullPath)/sizeof(wchar_t), L"%s\\%s.lnk", desktopPath, findData.cFileName);
                    
                    // 将宽字符路径转换为UTF-8字符串
                    int pathUtf8Size = WideCharToMultiByte(CP_UTF8, 0, fullPath, -1, NULL, 0, NULL, NULL);
                    if (pathUtf8Size > 0) {
                        char* utf8Path = new char[pathUtf8Size];
                        WideCharToMultiByte(CP_UTF8, 0, fullPath, -1, utf8Path, pathUtf8Size, NULL, NULL);
                        shortcut.path = utf8Path;
                        delete[] utf8Path;
                    } else {
                        shortcut.path = "";
                    }
                    
                    shortcut.type = 2;  // 标记为应用程序
                    shortcut.usageCount = 0;
                    
                    g_shortcuts.push_back(shortcut);
                }
            } while (FindNextFileW(hFind, &findData));
            
            FindClose(hFind);
        }
    }
}

// 初始化常用快捷方式
void InitializeCommonShortcuts() {
    g_shortcuts.clear();
    
    // 添加桌面快捷方式
    AddDesktopShortcuts();
    
    // 桌面文件夹
    ShortcutItem desktop;
    desktop.name = "桌面";
    desktop.type = 0;
    desktop.usageCount = 0;
    char desktopPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, desktopPath, CSIDL_DESKTOP, FALSE);
    desktop.path = desktopPath;
    g_shortcuts.push_back(desktop);
    
    // 显示桌面
    ShortcutItem showDesktop;
    showDesktop.name = "显示桌面";
    showDesktop.path = "explorer.exe shell:::{3080F90D-D7AD-11D9-BD98-0000947B0257}";
    showDesktop.type = 2;
    showDesktop.usageCount = 0;
    g_shortcuts.push_back(showDesktop);
    
    // 开始菜单程序
    ShortcutItem startMenu;
    startMenu.name = "开始菜单";
    startMenu.type = 0;
    startMenu.usageCount = 0;
    char startMenuPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, startMenuPath, CSIDL_PROGRAMS, FALSE);
    startMenu.path = startMenuPath;
    g_shortcuts.push_back(startMenu);
    
    // 下载文件夹
    ShortcutItem downloads;
    downloads.name = "下载";
    downloads.type = 0;
    downloads.usageCount = 0;
    char downloadsPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, downloadsPath, CSIDL_MYDOCUMENTS, FALSE);
    strcat_s(downloadsPath, sizeof(downloadsPath), "\\Downloads");
    downloads.path = downloadsPath;
    g_shortcuts.push_back(downloads);
    
    // 文档文件夹
    ShortcutItem documents;
    documents.name = "文档";
    documents.type = 0;
    documents.usageCount = 0;
    char documentsPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, documentsPath, CSIDL_PERSONAL, FALSE);
    documents.path = documentsPath;
    g_shortcuts.push_back(documents);
    
    // 图片文件夹
    ShortcutItem pictures;
    pictures.name = "图片";
    pictures.type = 0;
    pictures.usageCount = 0;
    char picturesPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, picturesPath, CSIDL_MYPICTURES, FALSE);
    pictures.path = picturesPath;
    g_shortcuts.push_back(pictures);
    
    // 音乐文件夹
    ShortcutItem music;
    music.name = "音乐";
    music.type = 0;
    music.usageCount = 0;
    char musicPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, musicPath, CSIDL_MYMUSIC, FALSE);
    music.path = musicPath;
    g_shortcuts.push_back(music);
    
    // 视频文件夹
    ShortcutItem videos;
    videos.name = "视频";
    videos.type = 0;
    videos.usageCount = 0;
    char videosPath[MAX_PATH];
    SHGetSpecialFolderPathA(NULL, videosPath, CSIDL_MYVIDEO, FALSE);
    videos.path = videosPath;
    g_shortcuts.push_back(videos);
    
    // 回收站
    ShortcutItem recycleBin;
    recycleBin.name = "回收站";
    recycleBin.path = "explorer.exe shell:::{645FF040-5081-101B-9F08-00AA002F954E}";
    recycleBin.type = 2;
    recycleBin.usageCount = 0;
    g_shortcuts.push_back(recycleBin);
    
    // 控制面板
    ShortcutItem controlPanel;
    controlPanel.name = "控制面板";
    controlPanel.path = "control.exe";
    controlPanel.type = 2;
    controlPanel.usageCount = 0;
    g_shortcuts.push_back(controlPanel);
    
    // 计算器
    ShortcutItem calculator;
    calculator.name = "计算器";
    calculator.path = "calc.exe";
    calculator.type = 2;
    calculator.usageCount = 0;
    g_shortcuts.push_back(calculator);
    
    // 记事本
    ShortcutItem notepad;
    notepad.name = "记事本";
    notepad.path = "notepad.exe";
    notepad.type = 2;
    notepad.usageCount = 0;
    g_shortcuts.push_back(notepad);
    
    // 画图
    ShortcutItem paint;
    paint.name = "画图";
    paint.path = "mspaint.exe";
    paint.type = 2;
    paint.usageCount = 0;
    g_shortcuts.push_back(paint);
    
    // 命令提示符
    ShortcutItem cmd;
    cmd.name = "命令提示符";
    cmd.path = "cmd.exe";
    cmd.type = 2;
    cmd.usageCount = 0;
    g_shortcuts.push_back(cmd);
    
    // PowerShell
    ShortcutItem powershell;
    powershell.name = "PowerShell";
    powershell.path = "powershell.exe";
    powershell.type = 2;
    powershell.usageCount = 0;
    g_shortcuts.push_back(powershell);
}

// 搜索并显示结果
void SearchAndDisplayResults(const std::string& query) {
    // 清空当前显示
    g_listItems.clear();
    g_searchResults.clear();
    
    // 检查是否是help命令
    if (query == "help" || query == "帮助") {
        if (g_calculatorMode) {
            ShowCalculatorHelp();
        } else {
            ShowHelp();
        }
        return;
    }
    
    // 检查是否是js命令（进入计算模式）
    if ((query == "js" || query == "计算") && !g_calculatorMode) {
        // 显示提示信息，需要按回车确认进入计算模式
        sf::Text promptItem;
        promptItem.setFont(g_font);
        promptItem.setCharacterSize(14);
        promptItem.setFillColor(sf::Color::Blue);
        promptItem.setString(L"提示: 输入'js'后按回车进入计算模式");
        promptItem.setPosition(25, 65);
        g_listItems.push_back(promptItem);
        g_searchResults.push_back("提示: 输入'js'后按回车进入计算模式");
        return;
    }
    
    // 检查是否是退出计算模式命令
    if ((query == "q" || query == "退出") && g_calculatorMode) {
        // 显示提示信息，需要按回车确认退出计算模式
        sf::Text promptItem;
        promptItem.setFont(g_font);
        promptItem.setCharacterSize(14);
        promptItem.setFillColor(sf::Color::Blue);
        promptItem.setString(L"提示: 输入'q'后按回车退出计算模式");
        promptItem.setPosition(25, 65);
        g_listItems.push_back(promptItem);
        g_searchResults.push_back("提示: 输入'q'后按回车退出计算模式");
        return;
    }
    
    // 如果是计算模式，处理数学表达式
    if (g_calculatorMode) {
        // 显示提示信息，需要按回车进行计算
        if (!query.empty()) {
            sf::Text promptItem;
            promptItem.setFont(g_font);
            promptItem.setCharacterSize(14);
            promptItem.setFillColor(sf::Color::Blue);
            promptItem.setString(L"提示: 输入表达式后按回车进行计算");
            promptItem.setPosition(25, 65);
            g_listItems.push_back(promptItem);
            g_searchResults.push_back("提示: 输入表达式后按回车进行计算");
        }
        return;
    }
    
    // 如果查询为空，显示所有项目
    if (query.empty()) {
        for (size_t i = 0; i < g_shortcuts.size(); i++) {
            sf::Text item;
            item.setFont(g_font);
            item.setCharacterSize(14);
            
            // 设置选择项颜色
            if (static_cast<int>(i) == g_selectedIndex) {
                item.setFillColor(sf::Color::White);  // 选中项白色文字
            } else {
                item.setFillColor(sf::Color::Black);  // 未选中项黑色文字
            }
            
            std::wstring displayText;
            if (g_shortcuts[i].type == 0) // 目录
                displayText = L"DIR: " + UTF8ToWide(g_shortcuts[i].name);
            else if (g_shortcuts[i].type == 1) // URL
                displayText = L"URL: " + UTF8ToWide(g_shortcuts[i].name);
            else // 应用程序
                displayText = L"APP: " + UTF8ToWide(g_shortcuts[i].name);
            
            item.setString(displayText);
            item.setPosition(25, 65 + static_cast<float>(i * 20));
            g_listItems.push_back(item);
            g_searchResults.push_back(g_shortcuts[i].name);
        }
        return;
    }
    
    // 转换为小写进行不区分大小写的搜索
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    int itemIndex = 0;
    
    // 首先查找精确匹配
    for (size_t i = 0; i < g_shortcuts.size(); i++) {
        std::string lowerName = g_shortcuts[i].name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName == lowerQuery) {
            sf::Text item;
            item.setFont(g_font);
            item.setCharacterSize(14);
            
            // 设置选择项颜色
            if (itemIndex == g_selectedIndex) {
                item.setFillColor(sf::Color::White);  // 选中项白色文字
            } else {
                item.setFillColor(sf::Color::Black);  // 未选中项黑色文字
            }
            
            std::wstring displayText;
            if (g_shortcuts[i].type == 0) // 目录
                displayText = L"DIR: " + UTF8ToWide(g_shortcuts[i].name);
            else if (g_shortcuts[i].type == 1) // URL
                displayText = L"URL: " + UTF8ToWide(g_shortcuts[i].name);
            else // 应用程序
                displayText = L"APP: " + UTF8ToWide(g_shortcuts[i].name);
            
            item.setString(displayText);
            item.setPosition(25, 65 + static_cast<float>(itemIndex * 20));
            g_listItems.push_back(item);
            g_searchResults.push_back(g_shortcuts[i].name);
            itemIndex++;
        }
    }
    
    // 然后查找子字符串匹配
    for (size_t i = 0; i < g_shortcuts.size(); i++) {
        std::string lowerName = g_shortcuts[i].name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find(lowerQuery) != std::string::npos) {
            sf::Text item;
            item.setFont(g_font);
            item.setCharacterSize(14);
            
            // 设置选择项颜色
            if (itemIndex == g_selectedIndex) {
                item.setFillColor(sf::Color::White);  // 选中项白色文字
            } else {
                item.setFillColor(sf::Color::Black);  // 未选中项黑色文字
            }
            
            std::wstring displayText;
            if (g_shortcuts[i].type == 0) // 目录
                displayText = L"DIR: " + UTF8ToWide(g_shortcuts[i].name);
            else if (g_shortcuts[i].type == 1) // URL
                displayText = L"URL: " + UTF8ToWide(g_shortcuts[i].name);
            else // 应用程序
                displayText = L"APP: " + UTF8ToWide(g_shortcuts[i].name);
            
            item.setString(displayText);
            item.setPosition(25, 65 + static_cast<float>(itemIndex * 20));
            g_listItems.push_back(item);
            g_searchResults.push_back(g_shortcuts[i].name);
            itemIndex++;
        }
    }
    
    // 如果没有找到匹配项
    if (g_listItems.empty()) {
        sf::Text noResult;
        noResult.setFont(g_font);
        noResult.setCharacterSize(14);
        noResult.setFillColor(sf::Color::Red);
        noResult.setString(L"未找到匹配项");
        noResult.setPosition(25, 65);
        g_listItems.push_back(noResult);
    }
}

// 执行选中的项目
void ExecuteSelectedItem(int index) {
    if (index < 0 || index >= static_cast<int>(g_searchResults.size())) {
        return;
    }
    
    // 找到对应的快捷方式
    std::string selectedName = g_searchResults[index];
    for (size_t i = 0; i < g_shortcuts.size(); i++) {
        if (g_shortcuts[i].name == selectedName) {
            ShortcutItem& item = g_shortcuts[i];
            
            // 执行项目
            HINSTANCE result;
            if (item.type == 0) { // 目录
                result = ShellExecuteA(NULL, "open", item.path.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
            else if (item.type == 1) { // URL
                result = ShellExecuteA(NULL, "open", item.path.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
            else { // 应用程序
                result = ShellExecuteA(NULL, "open", item.path.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
            
            // 更新使用计数
            item.usageCount++;
            
            // 记录日志
            char logMsg[256];
            sprintf_s(logMsg, sizeof(logMsg), "执行项目: %s, 路径: %s, 结果: %p", 
                    item.name.c_str(), item.path.c_str(), result);
            LogToFile(logMsg);
            
            // 不清空搜索框，保持窗口打开状态
            // 用户可以继续搜索其他项目
            break;
        }
    }
}

// 显示启动器窗口
void ShowLauncherWindow() {
    // 创建窗口（使用默认样式，支持移动）
    g_window = new sf::RenderWindow(sf::VideoMode(400, 320), L"快速启动器", sf::Style::Default);
    
    // 设置窗口位置（屏幕中央）
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    g_window->setPosition(sf::Vector2i((desktop.width - 400) / 2, (desktop.height - 320) / 2));
    
    // 初始化UI
    InitializeUI();
    
    // 初始化快捷方式
    InitializeCommonShortcuts();
    
    // 显示默认结果
    SearchAndDisplayResults("");
    
    // 主循环
    std::string currentSearch;
    bool hasFocus = true;
    
    while (g_window->isOpen()) {
        // 处理光标闪烁
        if (g_cursorClock.getElapsedTime().asSeconds() > g_cursorBlinkInterval) {
            g_cursorVisible = !g_cursorVisible;
            g_cursorClock.restart();
        }
        
        // 更新光标位置（跟随文本内容）
        if (hasFocus) {
            // 计算文本宽度来确定光标位置
            sf::FloatRect textBounds = g_searchText.getLocalBounds();
            float cursorX = 25 + textBounds.width + 2;  // 文本右侧2像素位置
            g_cursor.setPosition(cursorX, 25);
        }
        
        sf::Event event;
        while (g_window->pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    g_window->close();
                    break;
                    
                case sf::Event::TextEntered:
                    if (hasFocus) {
                        // 处理文本输入
                        if (event.text.unicode == '\b') { // 退格键
                            if (!currentSearch.empty()) {
                                // 正确删除UTF-8字符（可能由多个字节组成）
                                // 从字符串末尾向前查找UTF-8字符的起始字节
                                size_t pos = currentSearch.length() - 1;
                                while (pos > 0 && (currentSearch[pos] & 0xC0) == 0x80) {
                                    pos--;
                                }
                                currentSearch.erase(pos);
                            }
                        }
                        else if (event.text.unicode >= 32) { // 所有可打印字符（包括中文）
                            // 将Unicode字符转换为UTF-8字符串
                            wchar_t wideChar = static_cast<wchar_t>(event.text.unicode);
                            wchar_t wideStr[2] = {wideChar, 0};
                            
                            // 将宽字符转换为UTF-8
                            int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
                            if (utf8Size > 0) {
                                char* utf8Str = new char[utf8Size];
                                WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str, utf8Size, NULL, NULL);
                                currentSearch += utf8Str;
                                delete[] utf8Str;
                            }
                        }
                        
                        // 更新搜索文本
                        g_searchText.setString(UTF8ToWide(currentSearch));
                        
                        // 在计算模式下，不实时搜索，只更新搜索框
                        // 只有在非计算模式下才执行搜索
                        if (!g_calculatorMode) {
                            SearchAndDisplayResults(currentSearch);
                        }
                    }
                    break;
                    
                case sf::Event::KeyPressed:
                    switch (event.key.code) {
                        case sf::Keyboard::Escape:
                            if (g_calculatorMode) {
                                // 在计算模式下，ESC键退出计算模式
                                ExitCalculatorMode();
                                SearchAndDisplayResults("");
                            } else {
                                g_window->close();
                            }
                            break;
                            
                        case sf::Keyboard::Enter:
                            std::cout << "[DEBUG] Enter键被按下，g_calculatorMode=" << g_calculatorMode << std::endl;
                            if (g_calculatorMode) {
                                std::cout << "[DEBUG] 处于计算模式，currentSearch='" << currentSearch << "'" << std::endl;
                                // 在计算模式下，Enter键触发计算
                                if (!currentSearch.empty()) {
                                    std::cout << "[DEBUG] 开始调用EvaluateJavaScriptExpression" << std::endl;
                                    double result = EvaluateJavaScriptExpression(currentSearch);
                                    std::cout << "[DEBUG] 计算完成，结果=" << result << ", std::isnan=" << std::isnan(result) << std::endl;
                                    
                                    // 检查特殊返回值
                                    if (result == -1.0) {  // 退出计算模式
                                        std::cout << "[DEBUG] 退出计算模式" << std::endl;
                                        ExitCalculatorMode();
                                        SearchAndDisplayResults("");
                                    }
                                    else if (result == -2.0) {  // 进入计算模式（已经是计算模式）
                                        std::cout << "[DEBUG] 已经是计算模式" << std::endl;
                                        // 不做任何操作
                                    }
                                    else if (result == 0.0 && (currentSearch == "help" || currentSearch == "帮助")) {
                                        // help命令
                                        std::cout << "[DEBUG] 显示帮助" << std::endl;
                                        ShowCalculatorHelp();
                                        
                                        // 帮助信息已显示，UI已更新
                                        std::cout << "[DEBUG] 帮助信息已显示，UI已更新" << std::endl;
                                        // 不清空搜索框，保持帮助命令可见，避免覆盖帮助信息
                                        // currentSearch.clear();  // 注释掉清空搜索框
                                        // g_searchText.setString(L"");  // 注释掉
                                    }
                                    else if (!std::isnan(result)) {
                                        // 正常计算结果
                                        std::cout << "[DEBUG] 显示计算结果" << std::endl;
                                        DisplayCalculationResult(currentSearch, result);
                                    }
                                    else {
                                        // 计算失败，显示错误信息
                                        std::cout << "[DEBUG] 计算失败，显示错误信息" << std::endl;
                                        sf::Text errorItem;
                                        errorItem.setFont(g_font);
                                        errorItem.setCharacterSize(14);
                                        errorItem.setFillColor(sf::Color::Red);
                                        errorItem.setString(L"计算错误: 无效的表达式");
                                        errorItem.setPosition(25, 65);
                                        g_listItems.clear();
                                        g_listItems.push_back(errorItem);
                                        g_searchResults.clear();
                                        g_searchResults.push_back("计算错误: 无效的表达式");
                                        
                                        // 计算错误信息已显示，UI已更新
                                        std::cout << "[DEBUG] 计算错误信息已显示，UI已更新" << std::endl;
                                    }
                                    
                                    // 在计算模式下，计算结果已显示，不要清空搜索框以避免覆盖计算结果
                                    std::cout << "[DEBUG] 计算结果已显示，UI已刷新" << std::endl;
                                } else {
                                    std::cout << "[DEBUG] 空输入" << std::endl;
                                }
                            } else {
                                // 普通模式下，检查是否是模式切换命令
                                if ((currentSearch == "js" || currentSearch == "计算") && !g_calculatorMode) {
                                    // 进入计算模式
                                    EnterCalculatorMode();
                                    DisplayCalculationResult("", 0.0);  // 显示欢迎信息
                                    currentSearch.clear();
                                    g_searchText.setString(L"");
                                } else if ((currentSearch == "q" || currentSearch == "退出") && g_calculatorMode) {
                                    // 退出计算模式
                                    ExitCalculatorMode();
                                    SearchAndDisplayResults("");
                                    currentSearch.clear();
                                    g_searchText.setString(L"");
                                } else {
                                    // 执行第一个项目
                                    if (!g_searchResults.empty()) {
                                        ExecuteSelectedItem(0);
                                    }
                                }
                            }
                            break;
                            
                        case sf::Keyboard::Down:
                            // 向下选择
                            if (!g_searchResults.empty()) {
                                g_selectedIndex++;
                                if (g_selectedIndex >= static_cast<int>(g_searchResults.size())) {
                                    g_selectedIndex = 0;  // 循环到顶部
                                }
                                SearchAndDisplayResults(currentSearch);  // 刷新显示
                            }
                            break;
                            
                        case sf::Keyboard::Up:
                            // 向上选择
                            if (!g_searchResults.empty()) {
                                g_selectedIndex--;
                                if (g_selectedIndex < 0) {
                                    g_selectedIndex = static_cast<int>(g_searchResults.size()) - 1;  // 循环到底部
                                }
                                SearchAndDisplayResults(currentSearch);  // 刷新显示
                            }
                            break;
                    }
                    break;
                    
                case sf::Event::MouseButtonPressed:
                    // 检查是否点击了列表框中的项目
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(*g_window);
                        
                        // 检查是否点击了列表框区域
                        if (mousePos.x >= 20 && mousePos.x <= 380 && 
                            mousePos.y >= 60 && mousePos.y <= 260) {
                            
                            // 计算点击的项目索引
                            int itemIndex = (mousePos.y - 65) / 20;
                            if (itemIndex >= 0 && itemIndex < static_cast<int>(g_listItems.size())) {
                                g_selectedIndex = itemIndex;
                                SearchAndDisplayResults(currentSearch);  // 刷新显示
                                
                                // 双击执行（检查时间间隔）
                                static sf::Clock clickClock;
                                if (clickClock.getElapsedTime().asSeconds() < 0.3f) {
                                    ExecuteSelectedItem(itemIndex);
                                }
                                clickClock.restart();
                            }
                        }
                    }
                    break;
            }
        }
        
        // 渲染
        g_window->clear(sf::Color::White);
        g_window->draw(g_searchBox);
        g_window->draw(g_listBox);
        g_window->draw(g_searchText);
        
        // 如果有焦点且光标可见，绘制光标
        if (hasFocus && g_cursorVisible) {
            g_window->draw(g_cursor);
        }
        
        // 如果有选择项，绘制选择背景
        if (g_selectedIndex >= 0 && g_selectedIndex < static_cast<int>(g_listItems.size())) {
            // 更新选择背景位置
            g_selectionBackground.setPosition(20.0f, 65.0f + static_cast<float>(g_selectedIndex) * 20.0f);
            g_window->draw(g_selectionBackground);
        }
        
        for (auto& item : g_listItems) {
            g_window->draw(item);
        }
        
        g_window->display();
    }
    
    // 清理
    delete g_window;
    g_window = nullptr;
}

// 主函数
int main() {
    // 设置控制台代码页为UTF-8，确保中文显示正常
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    LogToFile("SFML版本快速启动器启动");
    
    // 初始化字体
    if (!InitializeFont()) {
        std::cout << "字体初始化失败，程序可能无法正常显示文本\n";
    }
    
    // 显示启动器窗口
    ShowLauncherWindow();
    
    LogToFile("SFML版本快速启动器退出");
    return 0;
}