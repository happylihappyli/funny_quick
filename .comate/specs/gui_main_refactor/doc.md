# gui_main.cpp 重构需求

## 需求背景
`gui_main.cpp` 文件当前有6372行代码，需要进行模块化重构，将文件大小控制在1000行以内，提高代码的可维护性和可读性。

## 用户故事
作为开发者，我希望 `gui_main.cpp` 文件能够模块化，以便：
1. 更容易理解和维护代码
2. 提高代码的复用性
3. 降低模块间的耦合度
4. 便于团队协作开发

## 使用场景
- 当前 `gui_main.cpp` 包含了窗口处理、WebView2集成、计算器功能、书签管理、目录浏览等多个功能模块
- 需要将这些功能拆分到独立的文件中
- 保留 `gui_main.cpp` 作为主要的窗口消息处理和程序入口

## 技术方案

### 模块拆分策略
1. **计算器模块** (`calculator.cpp/.h`)
   - 表达式解析函数：`parseNumber`, `parseTerm`, `parseExpression`
   - 计算器相关函数：`EnterCalculatorMode`, `ExitCalculatorMode`, `EvaluateExpression`, `DisplayCalculationHistory`
   - 计算历史管理：`SaveCalculationHistory`, `LoadCalculationHistory`

2. **书签管理模块** (`bookmarks.cpp/.h`)
   - 书签相关函数：`EnterBookmarkMode`, `ExitBookmarkMode`, `AddBookmark`, `DeleteBookmark`
   - 书签文件操作：`SaveBookmarks`, `LoadBookmarks`, `SearchBookmarks`
   - Chrome同步：`SyncChromeBookmarks`

3. **目录浏览模块** (`directory_browser.cpp/.h`)
   - 目录操作函数：`EnterDirMode`, `ExitDirMode`, `UpdateDirModeWebView`
   - 目录内容获取：`GetDrives`, `GetCommonPaths`, `GetDirectoryContents`

4. **WebView2集成模块** (`webview_manager.cpp/.h`)
   - WebView2初始化和管理：`InitializeWebView2`, `UpdateWebView2Content`
   - HTML生成函数：`CreateWebView2HTML`, `UpdateCalculatorModeWebView`, `UpdateBookmarkModeWebView`, `UpdateSettingsMenuWebView`, `UpdateHelpInfoWebView`, `UpdateDirModeWebView`
   - 基本用法显示：`ShowBasicUsage`

5. **UI管理模块** (`ui_manager.cpp/.h`) - 已存在，需要整合
   - 字体管理：`CreateUIFont`, `ApplyFontToControl`
   - 布局管理：`LayoutControls`
   - 列表视图操作：`UpdateListViewColumns`, `AddHintRowToListView`, `AddMultiLineHintsToListView`, `GetHintRowCount`, `GetFirstActualItemIndex`, `LogListViewContents`

6. **搜索和命令处理模块** (`command_handler.cpp/.h`) - 已存在，需要整合
   - 搜索功能：`SearchAndDisplayResults`
   - 命令执行：`ProcessCommand`, `ExecuteSelectedItem`
   - 快捷方式管理：`InitializeCommonShortcuts`, `AddDesktopShortcuts`

7. **系统托盘模块** (`tray.cpp/.h`) - 已存在，需要整合
   - 托盘图标管理：`AddTrayIcon`, `RemoveTrayIcon`, `CreateTrayMenu`, `HandleTrayMessage`

8. **浮动按钮模块** (`float_button.cpp/.h`) - 已存在，需要整合
   - 浮动按钮窗口过程和相关函数

### 保留在 gui_main.cpp 的内容
- 主窗口过程 `WindowProc` 及其消息处理器
- 程序入口 `WinMain`
- 编辑框子类化过程 `EditSubclassProc`
- 窗口消息处理函数：`HandleWmCreate`, `HandleWmHotKey`, `HandleWmDestroy`, `HandleWmSize`, `HandleWmNotify`, `HandleWmCommand`, `HandleWmKeyDown`, `HandleWmContextMenu`
- 全局变量定义（但会根据模块进行分组）

### 实现细节

#### 数据实体设计
- 将全局变量按功能模块分组到对应头文件中
- 使用 `extern` 声明在头文件，定义在对应的cpp文件中
- 创建模块初始化函数来管理模块状态

#### 数据流设计
1. 主窗口接收消息
2. 根据消息类型调用对应模块的处理函数
3. 模块处理完成后返回结果
4. 主窗口根据结果更新UI

#### 逻辑时序设计
1. 程序启动 → 初始化各个模块
2. 窗口创建 → 创建UI控件
3. 用户输入 → 消息路由到对应模块
4. 模块处理 → 更新显示内容

#### 接口设计
每个模块将提供统一的接口：
- 初始化函数：`ModuleName_Initialize()`
- 清理函数：`ModuleName_Cleanup()`
- 处理函数：`ModuleName_HandleAction()`

## 预期成果
1. `gui_main.cpp` 文件大小减少到1000行以内
2. 代码按功能模块清晰分离
3. 每个模块职责单一，便于维护
4. 保持原有功能完整性
5. 编译无错误，运行正常

## 风险评估
- 模块间依赖可能较复杂，需要仔细处理
- 全局变量的管理需要特别注意
- 需要确保所有功能正确迁移
- 编译顺序和依赖关系需要调整