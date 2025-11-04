# 快捷命令启动工具 (Quick Launch Tool)

这是一个用C++编写的快捷命令启动工具，可以代替Windows启动菜单的部分功能。

## 功能特性

- **目录快速访问**：输入目录关键词，自动打开资源管理器并显示匹配的常用目录
- **图片预览**：输入图片文件路径，自动预览图片
- **网页快速打开**：输入网址，自动用Chrome浏览器打开（如果没有Chrome则使用默认浏览器）
- **常用目录管理**：内置常用目录如Users、Program Files、Windows、Downloads
- **友好的命令行界面**：彩色提示和帮助信息

## 编译方法

使用SCons构建系统编译项目：

```bash
scons
```

清理编译文件：

```bash
scons -c
```

## 使用说明

1. 运行编译生成的 `quick_launcher.exe`
2. 在提示符 `QuickLaunch >` 后输入命令

### 支持的命令类型

- **目录路径**：如 `C:\Users` 或关键词如 `downloads`
- **图片文件**：如 `image.jpg`、`photo.png` 等
- **网址**：如 `https://www.example.com` 或 `www.google.com`
- **帮助命令**：输入 `help` 查看使用说明
- **退出命令**：输入 `exit` 或 `quit` 退出程序

## 示例

```
QuickLaunch > downloads     # 打开下载文件夹
QuickLaunch > C:\Users      # 打开用户目录
QuickLaunch > image.jpg     # 预览图片
QuickLaunch > www.google.com # 打开谷歌主页
QuickLaunch > help          # 显示帮助信息
QuickLaunch > exit          # 退出程序
```

## 系统要求

- Windows 操作系统
- Visual Studio 编译器或兼容的C++编译器
- SCons 构建系统

## 项目结构

- `main.cpp` - 主程序入口
- `command_handler.h/cpp` - 命令处理核心逻辑
- `ui_manager.h/cpp` - 用户界面管理
- `SConstruct` - SCons构建配置文件
- `README.md` - 项目说明文档

## 注意事项

- 常用目录在程序中硬编码，如需修改请编辑 `main.cpp` 文件
- 图片预览功能使用系统默认图片查看器
- Chrome浏览器路径默认为 `C:\Program Files\Google\Chrome\Application\chrome.exe`