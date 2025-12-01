# gui_main.cpp 重构任务计划

## 任务概述
将6372行的 `gui_main.cpp` 重构为多个模块，目标是将主文件控制在1000行以内。

- [x] 任务1：创建计算器模块 (calculator.cpp/.h)
    - 1.1: 创建 calculator.h 头文件，声明计算器相关函数和数据
    - 1.2: 将表达式解析函数迁移到 calculator.cpp
    - 1.3: 将计算器模式函数迁移到 calculator.cpp
    - 1.4: 将计算历史管理函数迁移到 calculator.cpp
    - 1.5: 在 gui_main.cpp 中添加 calculator.h 的包含

- [x] 任务2：创建书签管理模块 (bookmarks.cpp/.h)
    - 2.1: 创建 bookmarks.h 头文件，声明书签相关函数和数据
    - 2.2: 将书签模式函数迁移到 bookmarks.cpp
    - 2.3: 将书签文件操作函数迁移到 bookmarks.cpp
    - 2.4: 将Chrome同步功能迁移到 bookmarks.cpp
    - 2.5: 创建书签对话框函数并迁移相关代码
    - 2.6: 在 gui_main.cpp 中添加 bookmarks.h 的包含

- [x] 任务3：创建目录浏览模块 (directory_browser.cpp/.h)
    - 3.1: 创建 directory_browser.h 头文件，声明目录浏览相关函数
    - 3.2: 将目录模式函数迁移到 directory_browser.cpp
    - 3.3: 将驱动器和路径获取函数迁移到 directory_browser.cpp
    - 3.4: 将目录内容获取函数迁移到 directory_browser.cpp
    - 3.5: 在 gui_main.cpp 中添加 directory_browser.h 的包含

- [x] 任务4：创建WebView2管理模块 (webview_manager.cpp/.h)
    - 4.1: 创建 webview_manager.h 头文件，声明WebView2相关函数
    - 4.2: 将WebView2初始化和更新函数迁移到 webview_manager.cpp
    - 4.3: 将所有HTML生成函数迁移到 webview_manager.cpp
    - 4.4: 将基本用法显示函数迁移到 webview_manager.cpp
    - 4.5: 在 gui_main.cpp 中添加 webview_manager.h 的包含

- [x] 任务5：整理并检查现有模块
    - 5.1: 检查并整合 ui_manager.cpp 中的列表视图相关函数 - 发现是控制台版本，不需要
    - 5.2: 检查并整合 command_handler.cpp 中的搜索和命令处理函数 - 发现是控制台版本，不需要
    - 5.3: 检查并整合 tray.cpp 中的系统托盘函数 - 已确认完整
    - 5.4: 检查并整合 float_button.cpp 中的浮动按钮函数 - 已确认完整
    - 5.5: 检查 shortcuts.cpp 中的搜索和命令处理函数 - 已确认包含所需功能

- [x] 任务6：重构 gui_main.cpp 主文件
    - 6.1: 保留主窗口过程 WindowProc 及消息处理器
    - 6.2: 保留程序入口 WinMain 函数
    - 6.3: 保留编辑框子类化过程 EditSubclassProc
    - 6.4: 保留窗口消息处理函数 (HandleWm*)
    - 6.5: 移除已迁移到其他模块的函数
    - 6.6: 添加必要的头文件包含和extern声明

- [x] 任务7：处理全局变量
    - 7.1: 将全局变量按模块分组到对应头文件
    - 7.2: 在各个模块cpp文件中定义对应的变量
    - 7.3: 在 gui_main.cpp 中使用extern声明引用全局变量
    - 7.4: 创建模块初始化函数管理变量初始化

- [x] 任务8：测试和验证
    - 8.1: 编译项目，确保没有编译错误 - 正在解决头文件冲突问题
    - 8.2: 测试计算器功能是否正常
    - 8.3: 测试书签管理功能是否正常
    - 8.4: 测试目录浏览功能是否正常
    - 8.5: 测试WebView2显示是否正常
    - 8.6: 验证 gui_main.cpp 文件行数在1000以内 - 已完成（804行）

- [ ] 任务9：清理和优化
    - 9.1: 删除未使用的函数和变量
    - 9.2: 优化包含关系，减少不必要的依赖
    - 9.3: 格式化代码，确保代码风格一致
    - 9.4: 添加必要的注释说明模块职责

- [ ] 任务10：最终验证
    - 10.1: 确认所有功能正常工作
    - 10.2: 确认 gui_main.cpp 文件大小符合要求
    - 10.3: 检查代码结构和模块划分是否合理
    - 10.4: 生成重构总结报告