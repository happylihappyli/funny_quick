# 链接器错误修复需求文档

## 需求场景具体处理逻辑
修复C++项目中的链接器错误，包括全局变量重复定义和无法解析的外部符号问题。项目是一个Windows桌面应用程序，使用SCons构建系统。

## 架构技术方案
采用标准的C++单一定义规则（One Definition Rule）解决方案：
1. 在头文件中声明全局变量为`extern`
2. 在一个专门的源文件中定义全局变量
3. 确保每个全局变量只在一个地方定义
4. 修复缺失函数的实现

## 影响文件
- `src/global_definitions_correct.cpp` - 需要保留所有全局变量定义
- `src/gui_main.cpp` - 需要移除重复的全局变量定义
- `src/calculator.cpp` - 需要移除重复的全局变量定义  
- `src/bookmarks.cpp` - 需要移除重复的全局变量定义
- `src/directory_browser.cpp` - 需要移除重复的全局变量定义
- `src/webview_manager.cpp` - 需要移除重复的全局变量定义
- 缺失的函数实现文件

## 实现细节

### 重复定义的全局变量
以下全局变量在多个文件中重复定义，需要保留在`global_definitions_correct.cpp`中，在其他文件中移除定义：

#### GUI相关全局变量
- `HINSTANCE g_hInstance`
- `HWND g_hMainWindow` 
- `HWND g_hEdit`
- `HWND g_hListView`
- `HWND g_hExitCalcButton`
- `HWND g_hSettingsButton`
- `HWND g_hExitBookmarkButton`
- `HWND g_hCalcMenuButton`
- `HFONT g_hFont`
- `bool g_ignoreNextReturn`
- `bool g_windowInitializing`

#### 模式标志变量
- `bool g_calculatorMode`
- `bool g_bookmarkMode`
- `bool g_dirMode`
- `bool g_updatingEditBox`

#### 数据结构变量
- `std::vector<CalculationRecord> g_calculationHistory`
- `std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks`
- `std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults`
- `std::set<std::wstring> g_expandedPaths`
- `std::wstring g_currentDirPath`

#### WebView2相关变量
- `HWND g_hWebView2`
- `ICoreWebView2Controller* g_webViewController`
- `ICoreWebView2* g_webView`
- `std::wstring g_cachedHelpHtml`
- `std::wstring g_cachedSettingsHtml`
- `bool g_helpHtmlCached`
- `bool g_settingsHtmlCached`

### 无法解析的外部符号
需要实现以下缺失的函数：
- `WindowProc` - 窗口过程函数
- `LoadWindowSettings` - 加载窗口设置函数
- `HandleSettingsMenuItemClick` - 处理设置菜单点击函数
- `ShowSettingsMenu` - 显示设置菜单函数
- `ShowHelpInfo` - 显示帮助信息函数
- `g_notifyIconData` - 托盘图标数据变量

### 修改代码示例

#### 移除重复定义（在gui_main.cpp中）
```cpp
// 需要移除这些定义行：
// HINSTANCE g_hInstance = NULL;
// HWND g_hMainWindow = NULL;
// HWND g_hEdit = NULL;
// HWND g_hListView = NULL;
// HWND g_hExitCalcButton = NULL;
// HWND g_hSettingsButton = NULL;
// HWND g_hExitBookmarkButton = NULL;
// HWND g_hCalcMenuButton = NULL;
// HFONT g_hFont = NULL;
// bool g_ignoreNextReturn = false;
// bool g_windowInitializing = false;

// 保留extern声明（已在头文件中）
```

#### 移除重复定义（在calculator.cpp中）
```cpp
// 需要移除这些定义行：
// bool g_calculatorMode = false;
// bool g_updatingEditBox = false;
// std::vector<CalculationRecord> g_calculationHistory;
```

#### 移除重复定义（在bookmarks.cpp中）
```cpp
// 需要移除这些定义行：
// bool g_bookmarkMode = false;
// std::vector<std::pair<std::wstring, std::wstring>> g_bookmarks;
// std::vector<std::pair<std::wstring, std::wstring>> g_bookmarkSearchResults;
```

## 边界条件与异常处理
1. 确保所有头文件中的全局变量声明都使用`extern`关键字
2. 验证移除定义后编译器不会报告变量未定义
3. 确保缺失函数的实现与调用签名完全匹配
4. 处理可能的命名空间或作用域问题

## 数据流动路径
1. 全局变量定义：`global_definitions_correct.cpp`
2. 全局变量声明：各个`.h`头文件
3. 全局变量使用：各个`.cpp`实现文件
4. 函数实现：新增或现有文件

## 预期成果
1. 消除所有LNK2005重复定义错误
2. 解决所有LNK2019无法解析的外部符号错误
3. 项目成功编译链接生成可执行文件
4. 保持所有功能模块正常工作
5. 遵循C++最佳实践和单一定义规则

## 验证方法
1. 运行SCons构建命令确认无链接器错误
2. 检查生成的.exe文件是否正常创建
3. 基本功能测试确保程序可以启动运行
4. 确认各个模式（计算器、书签、目录浏览等）正常切换