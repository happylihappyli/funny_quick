#ifndef DIR_MODE_MANAGER_H
#define DIR_MODE_MANAGER_H

#include <windows.h>
#include <vector>
#include <string>
#include <set>

// 前向声明
extern bool g_dirMode;
extern std::set<std::wstring> g_expandedPaths;

/**
 * @brief 进入目录浏览模式
 * 
 * 此函数用于切换到目录浏览模式，显示驱动器和常用路径
 */
void EnterDirMode();

/**
 * @brief 退出目录浏览模式
 * 
 * 此函数用于退出目录浏览模式，恢复默认界面
 */
void ExitDirMode();

/**
 * @brief 更新目录浏览模式的WebView2显示
 * 
 * 此函数生成目录浏览模式的HTML界面，显示驱动器、常用路径和已展开目录
 */
void UpdateDirModeWebView();

/**
 * @brief 获取系统所有驱动器列表
 * 
 * @return std::vector<std::wstring> 驱动器列表（如C:\, D:\等）
 */
std::vector<std::wstring> GetDrives();

/**
 * @brief 获取常用路径列表
 * 
 * @return std::vector<std::pair<std::wstring, bool>> 常用路径列表，包含显示名称和是否为目录
 */
std::vector<std::pair<std::wstring, bool>> GetCommonPaths();

/**
 * @brief 获取指定目录的内容
 * 
 * @param path 目录路径
 * @return std::vector<std::pair<std::wstring, bool>> 目录内容列表，包含文件名和是否为目录
 */
std::vector<std::pair<std::wstring, bool>> GetDirectoryContents(const WCHAR* path);

#endif // DIR_MODE_MANAGER_H