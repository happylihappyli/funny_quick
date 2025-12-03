// 文件搜索管理器类头文件
// 封装Everything SDK功能，实现文件搜索功能

#ifndef FILE_SEARCH_MANAGER_H
#define FILE_SEARCH_MANAGER_H

#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <vector>
#include <string>

// Everything SDK动态加载版本头文件
#include "../Everything-SDK/include/Everything_dynamic.h"

// 使用file_manager.h中定义的FileSearchResult结构体
#include "file_manager.h"

/**
 * 文件搜索管理器类
 * 封装Everything SDK功能，提供文件搜索功能
 */
class FileSearchManager {
public:
    FileSearchManager();
    ~FileSearchManager();
    
    /**
     * 初始化Everything SDK
     * @return 初始化是否成功
     */
    bool Initialize();
    
    /**
     * 释放Everything SDK资源
     */
    void Cleanup();
    
    /**
     * 执行文件搜索
     * @param query 搜索查询字符串
     * @param maxResults 最大结果数量（默认100）
     * @return 搜索结果列表
     */
    std::vector<FileSearchResult> SearchFiles(const std::wstring& query, int maxResults = 100);
    
    /**
     * 检查Everything SDK是否可用
     * @return SDK是否可用
     */
    bool IsAvailable() const;
    
    /**
     * 获取最后错误信息
     * @return 错误信息字符串
     */
    std::wstring GetLastError() const;
    
    /**
     * 设置搜索排序方式
     * @param sortType 排序类型（0=按名称，1=按路径，2=按大小，3=按扩展名，4=按日期创建，5=按日期修改，6=按日期访问）
     * @param ascending 是否升序排列
     */
    void SetSortOrder(int sortType, bool ascending = true);
    
    /**
     * 设置是否匹配路径
     * @param matchPath 是否匹配路径
     */
    void SetMatchPath(bool matchPath);
    
    /**
     * 设置是否匹配大小写
     * @param matchCase 是否匹配大小写
     */
    void SetMatchCase(bool matchCase);
    
    /**
     * 设置是否匹配完整单词
     * @param matchWholeWord 是否匹配完整单词
     */
    void SetMatchWholeWord(bool matchWholeWord);
    
    /**
     * 设置是否使用正则表达式
     * @param useRegex 是否使用正则表达式
     */
    void SetRegex(bool useRegex);

private:
    HMODULE m_hEverythingDll;           // Everything DLL句柄
    bool m_initialized;                 // 是否已初始化
    std::wstring m_lastError;           // 最后错误信息
    
    // Everything SDK函数指针类型定义
    typedef void (EVERYTHINGAPI *Everything_SetSearchW_t)(LPCWSTR lpString);
    typedef BOOL (EVERYTHINGAPI *Everything_QueryW_t)(BOOL bWait);
    typedef DWORD (EVERYTHINGAPI *Everything_GetNumResults_t)(void);
    typedef LPCWSTR (EVERYTHINGAPI *Everything_GetResultFileNameW_t)(DWORD nIndex);
    typedef void (EVERYTHINGAPI *Everything_GetResultFullPathNameW_t)(DWORD nIndex, LPWSTR buf, DWORD bufsize);
    typedef BOOL (EVERYTHINGAPI *Everything_IsFolderResult_t)(DWORD nIndex);
    typedef BOOL (EVERYTHINGAPI *Everything_IsFileResult_t)(DWORD nIndex);
    typedef DWORD (EVERYTHINGAPI *Everything_GetLastError_t)(void);
    typedef void (EVERYTHINGAPI *Everything_SetSort_t)(DWORD dwSort);
    typedef void (EVERYTHINGAPI *Everything_SetMatchPath_t)(BOOL bEnable);
    typedef void (EVERYTHINGAPI *Everything_SetMatchCase_t)(BOOL bEnable);
    typedef void (EVERYTHINGAPI *Everything_SetMatchWholeWord_t)(BOOL bEnable);
    typedef void (EVERYTHINGAPI *Everything_SetRegex_t)(BOOL bEnable);
    
    // Everything SDK函数指针实例
    Everything_SetSearchW_t m_pEverything_SetSearchW;
    Everything_QueryW_t m_pEverything_QueryW;
    Everything_GetNumResults_t m_pEverything_GetNumResults;
    Everything_GetResultFileNameW_t m_pEverything_GetResultFileNameW;
    Everything_GetResultFullPathNameW_t m_pEverything_GetResultFullPathNameW;
    Everything_IsFolderResult_t m_pEverything_IsFolderResult;
    Everything_IsFileResult_t m_pEverything_IsFileResult;
    Everything_GetLastError_t m_pEverything_GetLastError;
    Everything_SetSort_t m_pEverything_SetSort;
    Everything_SetMatchPath_t m_pEverything_SetMatchPath;
    Everything_SetMatchCase_t m_pEverything_SetMatchCase;
    Everything_SetMatchWholeWord_t m_pEverything_SetMatchWholeWord;
    Everything_SetRegex_t m_pEverything_SetRegex;
    
    /**
     * 动态加载Everything DLL
     * @return 加载是否成功
     */
    bool LoadEverythingDll();
    
    /**
     * 获取函数指针
     * @return 获取是否成功
     */
    bool GetFunctionPointers();
    
    /**
     * 释放Everything DLL
     */
    void UnloadEverythingDll();
    
    /**
     * 设置错误信息
     * @param error 错误信息
     */
    void SetLastError(const std::wstring& error);
    
    /**
     * 设置错误信息（从系统错误代码）
     * @param errorCode 系统错误代码
     */
    void SetLastError(DWORD errorCode);
};

/**
 * 获取当前用户名
 * @return 当前用户名
 */
std::wstring GetCurrentUserName();

#endif // FILE_SEARCH_MANAGER_H