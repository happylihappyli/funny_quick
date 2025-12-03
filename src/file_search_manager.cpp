// 文件搜索管理器类实现文件
// 封装Everything SDK功能，实现文件搜索功能

#include "file_search_manager.h"
#include "logger.h"
#include "calculator.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <string>
#include <vector>
#include <windows.h>

/**
 * 将宽字符字符串转换为多字节字符串
 * @param wstr 宽字符字符串
 * @return 多字节字符串
 */
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

/**
 * 文件搜索管理器类实现
 */
FileSearchManager::FileSearchManager() 
    : m_hEverythingDll(NULL)
    , m_initialized(false)
    , m_pEverything_SetSearchW(NULL)
    , m_pEverything_QueryW(NULL)
    , m_pEverything_GetNumResults(NULL)
    , m_pEverything_GetResultFileNameW(NULL)
    , m_pEverything_GetResultFullPathNameW(NULL)
    , m_pEverything_IsFolderResult(NULL)
    , m_pEverything_IsFileResult(NULL)
    , m_pEverything_GetLastError(NULL)
    , m_pEverything_SetSort(NULL)
    , m_pEverything_SetMatchPath(NULL)
    , m_pEverything_SetMatchCase(NULL)
    , m_pEverything_SetMatchWholeWord(NULL)
    , m_pEverything_SetRegex(NULL) {
}

/**
 * 析构函数
 */
FileSearchManager::~FileSearchManager() {
    Cleanup();
}

/**
 * 初始化Everything SDK
 * @return 初始化是否成功
 */
bool FileSearchManager::Initialize() {
    LogToFile(WStringToString(L"FileSearchManager: 开始初始化").c_str());
    
    // 如果已经初始化，直接返回成功
    if (m_initialized) {
        LogToFile(WStringToString(L"FileSearchManager: 已经初始化，跳过").c_str());
        return true;
    }
    
    // 尝试加载Everything DLL
    if (LoadEverythingDll()) {
        // 获取函数指针
        if (GetFunctionPointers()) {
            m_initialized = true;
            LogToFile(WStringToString(L"FileSearchManager: Everything SDK初始化成功").c_str());
            
            // 显示成功提示
            std::wstring successMsg = L"🔍 文件搜索模式已启用（使用Everything SDK）";
            AddHintRowToListView(successMsg.c_str());
            return true;
        } else {
            LogToFile(WStringToString(L"FileSearchManager: 获取Everything函数指针失败").c_str());
            UnloadEverythingDll();
        }
    }
    
    // 如果Everything SDK不可用，使用Windows API模式
    LogToFile(WStringToString(L"FileSearchManager: Everything SDK不可用，使用Windows API模式").c_str());
    
    // 显示警告提示
    std::wstring warningMsg = L"⚠️ Everything SDK不可用，使用Windows API搜索（功能有限）";
    AddHintRowToListView(warningMsg.c_str());
    
    // 显示安装提示
    std::wstring installMsg = L"💡 提示：安装Everything软件以获得更好的文件搜索体验";
    AddHintRowToListView(installMsg.c_str());
    
    m_initialized = true; // Windows API模式总是可用的
    return true;
}

/**
 * 释放Everything SDK资源
 */
void FileSearchManager::Cleanup() {
    if (m_hEverythingDll != NULL) {
        UnloadEverythingDll();
    }
    m_initialized = false;
}

/**
 * 执行文件搜索
 * @param query 搜索查询字符串
 * @param maxResults 最大结果数量
 * @return 搜索结果列表
 */
std::vector<FileSearchResult> FileSearchManager::SearchFiles(const std::wstring& query, int maxResults) {
    std::vector<FileSearchResult> results;
    
    if (query.empty()) {
        SetLastError(std::wstring(L"搜索查询不能为空"));
        return results;
    }
    
    LogToFile(WStringToString(L"FileSearchManager: 开始搜索文件，查询: " + query).c_str());
    
    // 如果Everything SDK可用，使用Everything搜索
    if (m_initialized) {
        LogToFile(WStringToString(L"FileSearchManager: 使用Everything SDK进行搜索").c_str());
        
        // 设置搜索条件
        m_pEverything_SetSearchW(query.c_str());
        
        // 设置最大结果数量
        if (maxResults > 0) {
            // Everything_SetMax函数可能不存在，这里使用默认值
            // 我们会在获取结果时限制数量
        }
        
        // 执行查询
        if (!m_pEverything_QueryW(TRUE)) {
            DWORD errorCode = m_pEverything_GetLastError();
            std::wstringstream errorMsg;
            errorMsg << L"查询失败，错误代码: " << errorCode;
            SetLastError(std::wstring(errorMsg.str()));
            LogToFile(WStringToString(L"FileSearchManager: Everything查询失败，错误: " + errorMsg.str()).c_str());
            // 继续尝试使用Windows API搜索
        } else {
            // 获取结果数量
            DWORD resultCount = m_pEverything_GetNumResults();
            if (resultCount == 0) {
                LogToFile(WStringToString(L"FileSearchManager: Everything搜索未找到结果").c_str());
                return results; // 没有找到结果
            }
            
            // 限制结果数量
            DWORD actualCount = (resultCount > (DWORD)maxResults) ? (DWORD)maxResults : resultCount;
            
            // 获取结果
            for (DWORD i = 0; i < actualCount; i++) {
                FileSearchResult result;
                
                // 获取文件名
                LPCWSTR fileName = m_pEverything_GetResultFileNameW(i);
                if (fileName != NULL) {
                    result.fileName = fileName;
                }
                
                // 获取完整路径
                WCHAR fullPath[MAX_PATH] = {0};
                m_pEverything_GetResultFullPathNameW(i, fullPath, MAX_PATH);
                result.fullPath = fullPath;
                
                // 判断是否为文件夹或文件
                result.isFolder = m_pEverything_IsFolderResult(i);
                result.isFile = m_pEverything_IsFileResult(i);
                
                // 设置文件类型
                if (result.isFolder) {
                    result.fileType = L"文件夹";
                } else if (result.isFile) {
                    // 从文件名中提取扩展名作为文件类型
                    size_t dotPos = result.fileName.find_last_of(L'.');
                    if (dotPos != std::wstring::npos && dotPos < result.fileName.length() - 1) {
                        result.fileType = result.fileName.substr(dotPos + 1) + L" 文件";
                    } else {
                        result.fileType = L"文件";
                    }
                } else {
                    result.fileType = L"未知";
                }
                
                results.push_back(result);
            }
            
            LogToFile(WStringToString(L"FileSearchManager: Everything搜索完成，找到 " + std::to_wstring(results.size()) + L" 个结果").c_str());
            return results;
        }
    }
    
    // 如果Everything SDK不可用或搜索失败，使用改进的Windows API搜索
    LogToFile(WStringToString(L"FileSearchManager: 使用改进的Windows API进行文件搜索").c_str());
    
    // 改进的搜索逻辑：更接近Everything的功能
    // 1. 搜索更多常用路径
    // 2. 支持文件名和路径搜索
    // 3. 更好的匹配算法
    
    std::vector<std::wstring> searchPaths = {
        L"C:\\Users\\*",
        L"C:\\Program Files\\*",
        L"C:\\Program Files (x86)\\*",
        L"D:\\*",
        L"E:\\*",
        L"C:\\Windows\\*",
        L"C:\\Temp\\*",
        L"C:\\Users\\" + GetCurrentUserName() + L"\\Desktop\\*",
        L"C:\\Users\\" + GetCurrentUserName() + L"\\Documents\\*",
        L"C:\\Users\\" + GetCurrentUserName() + L"\\Downloads\\*"
    };
    
    // 构建多种搜索模式
    std::vector<std::wstring> searchPatterns = {
        L"*" + query + L"*",           // 包含搜索词
        L"*" + query + L"*.*",         // 包含搜索词，有扩展名
        query + L"*",                  // 以搜索词开头
        L"*" + query,                  // 以搜索词结尾
    };
    
    for (const auto& basePath : searchPaths) {
        if (results.size() >= maxResults && maxResults > 0) {
            break;
        }
        
        for (const auto& pattern : searchPatterns) {
            if (results.size() >= maxResults && maxResults > 0) {
                break;
            }
            
            std::wstring fullSearchPath = basePath + pattern;
            
            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(fullSearchPath.c_str(), &findData);
            
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    // 跳过.和..目录
                    if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
                        continue;
                    }
                    
                    // 检查是否已经包含相同路径的结果
                    bool duplicate = false;
                    std::wstring currentPath = basePath.substr(0, basePath.length() - 1) + L"\\" + findData.cFileName;
                    for (const auto& existingResult : results) {
                        if (existingResult.fullPath == currentPath) {
                            duplicate = true;
                            break;
                        }
                    }
                    
                    if (duplicate) {
                        continue;
                    }
                    
                    FileSearchResult result;
                    result.fileName = findData.cFileName;
                    result.fullPath = currentPath;
                    result.isFolder = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    result.isFile = !result.isFolder;
                    
                    // 设置文件类型
                    if (result.isFolder) {
                        result.fileType = L"文件夹";
                    } else {
                        size_t dotPos = result.fileName.find_last_of(L'.');
                        if (dotPos != std::wstring::npos && dotPos < result.fileName.length() - 1) {
                            result.fileType = result.fileName.substr(dotPos + 1) + L" 文件";
                        } else {
                            result.fileType = L"文件";
                        }
                    }
                    
                    // 计算匹配度（简单实现）
                    std::wstring lowerFileName = result.fileName;
                    std::wstring lowerQuery = query;
                    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::towlower);
                    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
                    
                    // 文件名完全匹配得分最高
                    if (lowerFileName == lowerQuery) {
                        result.matchScore = 100;
                    }
                    // 文件名以搜索词开头得分次高
                    else if (lowerFileName.find(lowerQuery) == 0) {
                        result.matchScore = 80;
                    }
                    // 文件名包含搜索词得分中等
                    else if (lowerFileName.find(lowerQuery) != std::wstring::npos) {
                        result.matchScore = 60;
                    }
                    // 路径包含搜索词得分较低
                    else {
                        std::wstring lowerFullPath = result.fullPath;
                        std::transform(lowerFullPath.begin(), lowerFullPath.end(), lowerFullPath.begin(), ::towlower);
                        if (lowerFullPath.find(lowerQuery) != std::wstring::npos) {
                            result.matchScore = 40;
                        } else {
                            result.matchScore = 0;
                        }
                    }
                    
                    // 只添加匹配度大于0的结果
                    if (result.matchScore > 0) {
                        results.push_back(result);
                    }
                    
                    if (results.size() >= maxResults && maxResults > 0) {
                        break;
                    }
                } while (FindNextFileW(hFind, &findData));
                
                FindClose(hFind);
            }
        }
    }
    
    // 按匹配度排序
    std::sort(results.begin(), results.end(), [](const FileSearchResult& a, const FileSearchResult& b) {
        return a.matchScore > b.matchScore;
    });
    
    LogToFile(WStringToString(L"FileSearchManager: Windows API搜索完成，找到 " + std::to_wstring(results.size()) + L" 个结果").c_str());
    return results;
}

/**
 * 检查Everything SDK是否可用
 * @return SDK是否可用
 */
bool FileSearchManager::IsAvailable() const {
    return m_initialized;
}

/**
 * 获取最后错误信息
 * @return 错误信息字符串
 */
std::wstring FileSearchManager::GetLastError() const {
    return m_lastError;
}

/**
 * 设置搜索排序方式
 * @param sortType 排序类型
 * @param ascending 是否升序排列
 */
void FileSearchManager::SetSortOrder(int sortType, bool ascending) {
    if (!m_initialized) return;
    
    // Everything排序类型定义
    // 0=按名称，1=按路径，2=按大小，3=按扩展名
    // 4=按日期创建，5=按日期修改，6=按日期访问
    // 升序：0-6，降序：8-14
    DWORD everythingSortType = sortType;
    if (!ascending) {
        everythingSortType += 8; // 降序排序类型
    }
    
    m_pEverything_SetSort(everythingSortType);
}

/**
 * 设置是否匹配路径
 * @param matchPath 是否匹配路径
 */
void FileSearchManager::SetMatchPath(bool matchPath) {
    if (!m_initialized || m_pEverything_SetMatchPath == NULL) return;
    m_pEverything_SetMatchPath(matchPath);
}

/**
 * 设置是否匹配大小写
 * @param matchCase 是否匹配大小写
 */
void FileSearchManager::SetMatchCase(bool matchCase) {
    if (!m_initialized || m_pEverything_SetMatchCase == NULL) return;
    m_pEverything_SetMatchCase(matchCase);
}

/**
 * 设置是否匹配完整单词
 * @param matchWholeWord 是否匹配完整单词
 */
void FileSearchManager::SetMatchWholeWord(bool matchWholeWord) {
    if (!m_initialized || m_pEverything_SetMatchWholeWord == NULL) return;
    m_pEverything_SetMatchWholeWord(matchWholeWord);
}

/**
 * 设置是否使用正则表达式
 * @param useRegex 是否使用正则表达式
 */
void FileSearchManager::SetRegex(bool useRegex) {
    if (!m_initialized || m_pEverything_SetRegex == NULL) return;
    m_pEverything_SetRegex(useRegex);
}

/**
 * 动态加载Everything DLL
 * @return 加载是否成功
 */
bool FileSearchManager::LoadEverythingDll() {
    LogToFile(WStringToString(L"FileSearchManager: 开始加载Everything DLL").c_str());
    
    // 尝试多个可能的DLL路径
    const wchar_t* dllPaths[] = {
        L"Everything-SDK\\dll\\Everything64.dll",  // 项目相对路径
        L"dll\\Everything64.dll",                   // 相对路径
        L"Everything64.dll"                         // 当前目录
    };
    
    int numPaths = sizeof(dllPaths) / sizeof(dllPaths[0]);
    
    for (int i = 0; i < numPaths; i++) {
        m_hEverythingDll = LoadLibraryW(dllPaths[i]);
        if (m_hEverythingDll != NULL) {
            std::wstring successMsg = L"FileSearchManager: 成功加载Everything DLL，路径: " + std::wstring(dllPaths[i]);
            LogToFile(WStringToString(successMsg).c_str());
            return true;
        } else {
            DWORD errorCode = ::GetLastError();
            std::wstringstream errorMsg;
            errorMsg << L"FileSearchManager: 加载DLL失败，路径: " << dllPaths[i] << L"，错误代码: " << errorCode;
            LogToFile(WStringToString(errorMsg.str()).c_str());
        }
    }
    
    // 如果以上路径都失败，尝试系统路径
    m_hEverythingDll = LoadLibraryW(L"Everything64.dll");
    if (m_hEverythingDll != NULL) {
        LogToFile(WStringToString(L"FileSearchManager: 成功从系统路径加载Everything DLL").c_str());
        return true;
    } else {
        DWORD errorCode = ::GetLastError();
        std::wstringstream errorMsg;
        errorMsg << L"FileSearchManager: 从系统路径加载DLL失败，错误代码: " << errorCode;
        LogToFile(WStringToString(errorMsg.str()).c_str());
    }
    
    LogToFile(WStringToString(L"FileSearchManager: 所有DLL加载尝试都失败，将使用Windows文件搜索API").c_str());
    return false;
}

/**
 * 获取函数指针
 * @return 获取是否成功
 */
bool FileSearchManager::GetFunctionPointers() {
    if (m_hEverythingDll == NULL) {
        return false;
    }
    
    // 获取必要的函数指针
    m_pEverything_SetSearchW = (Everything_SetSearchW_t)GetProcAddress(m_hEverythingDll, "Everything_SetSearchW");
    m_pEverything_QueryW = (Everything_QueryW_t)GetProcAddress(m_hEverythingDll, "Everything_QueryW");
    m_pEverything_GetNumResults = (Everything_GetNumResults_t)GetProcAddress(m_hEverythingDll, "Everything_GetNumResults");
    m_pEverything_GetResultFileNameW = (Everything_GetResultFileNameW_t)GetProcAddress(m_hEverythingDll, "Everything_GetResultFileNameW");
    m_pEverything_GetResultFullPathNameW = (Everything_GetResultFullPathNameW_t)GetProcAddress(m_hEverythingDll, "Everything_GetResultFullPathNameW");
    m_pEverything_IsFolderResult = (Everything_IsFolderResult_t)GetProcAddress(m_hEverythingDll, "Everything_IsFolderResult");
    m_pEverything_IsFileResult = (Everything_IsFileResult_t)GetProcAddress(m_hEverythingDll, "Everything_IsFileResult");
    m_pEverything_GetLastError = (Everything_GetLastError_t)GetProcAddress(m_hEverythingDll, "Everything_GetLastError");
    m_pEverything_SetSort = (Everything_SetSort_t)GetProcAddress(m_hEverythingDll, "Everything_SetSort");
    
    // 可选函数指针（可能不存在）
    m_pEverything_SetMatchPath = (Everything_SetMatchPath_t)GetProcAddress(m_hEverythingDll, "Everything_SetMatchPath");
    m_pEverything_SetMatchCase = (Everything_SetMatchCase_t)GetProcAddress(m_hEverythingDll, "Everything_SetMatchCase");
    m_pEverything_SetMatchWholeWord = (Everything_SetMatchWholeWord_t)GetProcAddress(m_hEverythingDll, "Everything_SetMatchWholeWord");
    m_pEverything_SetRegex = (Everything_SetRegex_t)GetProcAddress(m_hEverythingDll, "Everything_SetRegex");
    
    // 检查必要的函数是否成功加载
    if (m_pEverything_SetSearchW == NULL || m_pEverything_QueryW == NULL || 
        m_pEverything_GetNumResults == NULL || m_pEverything_GetResultFileNameW == NULL ||
        m_pEverything_GetResultFullPathNameW == NULL || m_pEverything_IsFolderResult == NULL ||
        m_pEverything_IsFileResult == NULL || m_pEverything_GetLastError == NULL ||
        m_pEverything_SetSort == NULL) {
        return false;
    }
    
    return true;
}

/**
 * 释放Everything DLL
 */
void FileSearchManager::UnloadEverythingDll() {
    if (m_hEverythingDll != NULL) {
        FreeLibrary(m_hEverythingDll);
        m_hEverythingDll = NULL;
    }
    
    // 重置所有函数指针
    m_pEverything_SetSearchW = NULL;
    m_pEverything_QueryW = NULL;
    m_pEverything_GetNumResults = NULL;
    m_pEverything_GetResultFileNameW = NULL;
    m_pEverything_GetResultFullPathNameW = NULL;
    m_pEverything_IsFolderResult = NULL;
    m_pEverything_IsFileResult = NULL;
    m_pEverything_GetLastError = NULL;
    m_pEverything_SetSort = NULL;
    m_pEverything_SetMatchPath = NULL;
    m_pEverything_SetMatchCase = NULL;
    m_pEverything_SetMatchWholeWord = NULL;
    m_pEverything_SetRegex = NULL;
}

/**
 * 设置错误信息
 * @param error 错误信息
 */
void FileSearchManager::SetLastError(const std::wstring& error) {
    m_lastError = error;
}

/**
 * 获取当前用户名
 * @return 当前用户名
 */
std::wstring GetCurrentUserName() {
    DWORD bufferSize = 0;
    GetUserNameW(NULL, &bufferSize);
    
    if (bufferSize == 0) {
        return L"User"; // 默认用户名
    }
    
    std::vector<wchar_t> buffer(bufferSize);
    if (GetUserNameW(buffer.data(), &bufferSize)) {
        return std::wstring(buffer.data());
    }
    
    return L"User"; // 默认用户名
}

/**
 * 设置错误信息（从系统错误代码）
 * @param errorCode 系统错误代码
 */
void FileSearchManager::SetLastError(DWORD errorCode) {
    wchar_t* errorMsg = NULL;
    DWORD result = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&errorMsg,
        0,
        NULL
    );
    
    if (result > 0 && errorMsg != NULL) {
        m_lastError = errorMsg;
        LocalFree(errorMsg);
    } else {
        std::wstringstream ss;
        ss << L"系统错误代码: " << errorCode;
        m_lastError = ss.str();
    }
}