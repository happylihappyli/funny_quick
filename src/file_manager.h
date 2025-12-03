#pragma once

#include <windows.h>
#include <string>
#include <vector>

/**
 * @brief 文件搜索结果结构体
 */
struct FileSearchResult
{
    std::wstring fileName;    // 文件名
    std::wstring fullPath;    // 文件完整路径
    std::wstring size;        // 文件大小
    std::wstring modified;    // 修改时间
    std::wstring fileType;    // 文件类型
    bool isFolder;            // 是否为文件夹
    bool isFile;              // 是否为文件
    int matchScore;           // 匹配度分数（0-100）
};

// 外部函数声明（在gui_main.cpp中定义）
extern void UpdateListViewColumns();
extern void ClearListView();
extern void AddHintRowToListView(const WCHAR* hintText);
extern void UpdateFileModeWebView();

/**
 * @brief 进入文件模式
 */
void EnterFileMode();

/**
 * @brief 退出文件模式
 */
void ExitFileMode();

/**
 * @brief 搜索文件
 * @param query 搜索查询字符串
 */
void SearchFiles(const WCHAR* query);

/**
 * @brief 显示文件搜索结果
 */
void DisplayFileSearchResults();

/**
 * @brief 打开文件
 * @param filePath 文件路径
 */
void OpenFile(const std::wstring& filePath);

/**
 * @brief 打开文件所在文件夹
 * @param filePath 文件路径
 */
void OpenFileFolder(const std::wstring& filePath);

/**
 * @brief 显示文件模式帮助信息
 */
void ShowFileModeHelpInfo();