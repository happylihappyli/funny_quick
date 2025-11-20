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

// 全局变量定义
sf::RenderWindow* g_window = nullptr;
sf::Font g_font;
sf::Text g_searchText;
sf::RectangleShape g_searchBox;
sf::RectangleShape g_listBox;
std::vector<sf::Text> g_listItems;
std::vector<std::string> g_searchResults;

// 快捷方式结构体
struct ShortcutItem {
    std::string name;
    std::string path;
    int type; // 0: 目录, 1: URL, 2: 应用程序
    int usageCount;
};

std::vector<ShortcutItem> g_shortcuts;

// 日志函数
void LogToFile(const std::string& message) {
    // 创建日志目录
    CreateDirectoryA("log", NULL);
    
    // 生成带时间戳的日志文件名
    SYSTEMTIME st;
    GetLocalTime(&st);
    char filename[256];
    sprintf(filename, "log\\quick_launcher_%04d%02d%02d_%02d%02d%02d.log", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    // 写入日志
    FILE* file = fopen(filename, "a");
    if (file) {
        char timestamp[64];
        sprintf(timestamp, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        
        fprintf(file, "%s%s\n", timestamp, message.c_str());
        fclose(file);
    }
}

// 初始化字体
bool InitializeFont() {
    // 尝试加载系统字体
    if (g_font.loadFromFile("C:\\Windows\\Fonts\\msyh.ttc")) {
        return true;
    }
    if (g_font.loadFromFile("C:\\Windows\\Fonts\\simhei.ttf")) {
        return true;
    }
    if (g_font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
        return true;
    }
    
    // 如果系统字体加载失败，创建默认字体
    std::cout << "警告：无法加载系统字体，使用默认字体\n";
    return false;
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
    
    // 创建列表框
    g_listBox.setSize(sf::Vector2f(360, 200));
    g_listBox.setPosition(20, 60);
    g_listBox.setFillColor(sf::Color(250, 250, 250));
    g_listBox.setOutlineColor(sf::Color(180, 180, 180));
    g_listBox.setOutlineThickness(2);
}

// 添加桌面快捷方式
void AddDesktopShortcuts() {
    char desktopPath[MAX_PATH] = {0};
    
    // 获取桌面路径
    if (SHGetSpecialFolderPathA(NULL, desktopPath, CSIDL_DESKTOP, FALSE)) {
        char searchPath[MAX_PATH] = {0};
        sprintf(searchPath, "%s\\*.lnk", desktopPath);
        
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath, &findData);
        
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                // 跳过目录
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    ShortcutItem shortcut;
                    
                    // 提取名称（不含扩展名）
                    char* dotPos = strrchr(findData.cFileName, '.');
                    if (dotPos) {
                        *dotPos = '\0';  // 移除.lnk扩展名
                    }
                    
                    // 设置快捷方式名称和路径
                    shortcut.name = findData.cFileName;
                    char fullPath[MAX_PATH];
                    sprintf(fullPath, "%s\\%s.lnk", desktopPath, findData.cFileName);
                    shortcut.path = fullPath;
                    shortcut.type = 2;  // 标记为应用程序
                    shortcut.usageCount = 0;
                    
                    g_shortcuts.push_back(shortcut);
                }
            } while (FindNextFileA(hFind, &findData));
            
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
    strcat(downloadsPath, "\\Downloads");
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
    g_listItems.clear();
    g_searchResults.clear();
    
    // 清空列表框
    
    if (query.empty()) {
        // 显示所有项目
        for (size_t i = 0; i < g_shortcuts.size(); i++) {
            sf::Text item;
            item.setFont(g_font);
            item.setCharacterSize(14);
            item.setFillColor(sf::Color::Black);
            
            std::string displayText;
            if (g_shortcuts[i].type == 0) // 目录
                displayText = "DIR: " + g_shortcuts[i].name;
            else if (g_shortcuts[i].type == 1) // URL
                displayText = "URL: " + g_shortcuts[i].name;
            else // 应用程序
                displayText = "APP: " + g_shortcuts[i].name;
            
            item.setString(displayText);
            item.setPosition(25, 65 + static_cast<float>(i * 20));
            g_listItems.push_back(item);
            g_searchResults.push_back(g_shortcuts[i].name);
        }
        return;
    }
    
    // 搜索匹配项
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    int itemIndex = 0;
    for (size_t i = 0; i < g_shortcuts.size(); i++) {
        std::string lowerName = g_shortcuts[i].name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        // 检查精确匹配
        if (lowerName == lowerQuery) {
            sf::Text item;
            item.setFont(g_font);
            item.setCharacterSize(14);
            item.setFillColor(sf::Color::Black);
            
            std::string displayText;
            if (g_shortcuts[i].type == 0) // 目录
                displayText = "DIR: " + g_shortcuts[i].name;
            else if (g_shortcuts[i].type == 1) // URL
                displayText = "URL: " + g_shortcuts[i].name;
            else // 应用程序
                displayText = "APP: " + g_shortcuts[i].name;
            
            item.setString(displayText);
            item.setPosition(25, 65 + static_cast<float>(itemIndex * 20));
            g_listItems.push_back(item);
            g_searchResults.push_back(g_shortcuts[i].name);
            itemIndex++;
        }
        else {
            // 检查子字符串匹配
            if (lowerName.find(lowerQuery) != std::string::npos) {
                sf::Text item;
                item.setFont(g_font);
                item.setCharacterSize(14);
                item.setFillColor(sf::Color::Black);
                
                std::string displayText;
                if (g_shortcuts[i].type == 0) // 目录
                    displayText = "DIR: " + g_shortcuts[i].name;
                else if (g_shortcuts[i].type == 1) // URL
                    displayText = "URL: " + g_shortcuts[i].name;
                else // 应用程序
                    displayText = "APP: " + g_shortcuts[i].name;
                
                item.setString(displayText);
                item.setPosition(25, 65 + static_cast<float>(itemIndex * 20));
                g_listItems.push_back(item);
                g_searchResults.push_back(g_shortcuts[i].name);
                itemIndex++;
            }
        }
    }
    
    // 如果没有找到匹配项
    if (g_listItems.empty()) {
        sf::Text noResult;
        noResult.setFont(g_font);
        noResult.setCharacterSize(14);
        noResult.setFillColor(sf::Color::Red);
        noResult.setString("未找到匹配项");
        noResult.setPosition(25, 65);
        g_listItems.push_back(noResult);
    }
}

// 执行选中的项目
void ExecuteSelectedItem(INT_PTR index) {
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
            sprintf(logMsg, "执行项目: %s, 路径: %s, 结果: %Id", 
                    item.name.c_str(), item.path.c_str(), (INT_PTR)result);
            LogToFile(logMsg);
            
            // 隐藏窗口
            if (g_window) {
                g_window->close();
            }
            break;
        }
    }
}

// 显示启动器窗口
void ShowLauncherWindow() {
    // 创建窗口
    g_window = new sf::RenderWindow(sf::VideoMode(400, 300), "快速启动器", sf::Style::None);
    
    // 设置窗口位置（屏幕中央）
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    g_window->setPosition(sf::Vector2i((desktop.width - 400) / 2, (desktop.height - 300) / 2));
    
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
                                currentSearch.pop_back();
                            }
                        }
                        else if (event.text.unicode >= 32 && event.text.unicode < 128) { // 可打印字符
                            currentSearch += static_cast<char>(event.text.unicode);
                        }
                        
                        // 更新搜索文本
                        g_searchText.setString(currentSearch);
                        
                        // 执行搜索
                        SearchAndDisplayResults(currentSearch);
                    }
                    break;
                    
                case sf::Event::KeyPressed:
                    switch (event.key.code) {
                        case sf::Keyboard::Escape:
                            g_window->close();
                            break;
                            
                        case sf::Keyboard::Enter:
                            // 执行第一个项目
                            if (!g_searchResults.empty()) {
                                ExecuteSelectedItem(0);
                            }
                            break;
                            
                        case sf::Keyboard::Down:
                            // 向下选择（TODO: 实现选择逻辑）
                            break;
                            
                        case sf::Keyboard::Up:
                            // 向上选择（TODO: 实现选择逻辑）
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
                                ExecuteSelectedItem(itemIndex);
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