#define _CRT_SECURE_NO_WARNINGS 1
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include "logger.h"

// 日志文件句柄（静态变量）
static FILE* g_logFileHandle = NULL;
static WCHAR g_logFileName[MAX_PATH] = {0};

// 关闭日志文件
void CloseLogFile()
{
    if (g_logFileHandle)
    {
        fflush(g_logFileHandle);
        fclose(g_logFileHandle);
        g_logFileHandle = NULL;
    }
}

// Log function - 改进版本，确保日志可靠写入文件
void LogToFile(const char* message)
{
    if (!message) return;
    
    // 确保bin目录存在
    CreateDirectoryW(L"bin", NULL);
    // 确保log目录存在
    CreateDirectoryW(L"bin\\log", NULL);
    
    // Generate unique log filename based on current date and time
    static bool fileNameGenerated = false;
    static bool bomWritten = false;
    
    if (!fileNameGenerated) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        // Format: bin\log\quick_launcher_YYYYMMDD_HHMMSS.log
        wsprintfW(g_logFileName, L"bin\\log\\quick_launcher_%04d%02d%02d_%02d%02d%02d.log",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond);
        fileNameGenerated = true;
        
        // 打开日志文件并保持打开状态以提高性能
        g_logFileHandle = _wfopen(g_logFileName, L"ab");
        if (g_logFileHandle)
        {
            // Check if file is empty and write BOM
            fseek(g_logFileHandle, 0, SEEK_END);
            long fileSize = ftell(g_logFileHandle);
            if (fileSize == 0)
            {
                // Write UTF-8 BOM
                const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
                fwrite(bom, 1, 3, g_logFileHandle);
                fflush(g_logFileHandle);
            }
            bomWritten = true;
        }
    }
    
    // 如果文件句柄无效，尝试重新打开
    if (!g_logFileHandle)
    {
        g_logFileHandle = _wfopen(g_logFileName, L"ab");
        if (g_logFileHandle && !bomWritten)
        {
            fseek(g_logFileHandle, 0, SEEK_END);
            long fileSize = ftell(g_logFileHandle);
            if (fileSize == 0)
            {
                const unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
                fwrite(bom, 1, 3, g_logFileHandle);
                fflush(g_logFileHandle);
            }
            bomWritten = true;
        }
    }
    
    if (g_logFileHandle)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        // Create full log entry with timestamp and message
        WCHAR fullLogEntry[2048] = {0};
        wsprintfW(fullLogEntry, L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] %hs\n", 
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, message);
        
        // Convert to UTF-8 and write
        int utf8Size = WideCharToMultiByte(CP_UTF8, 0, fullLogEntry, -1, NULL, 0, NULL, NULL);
        if (utf8Size > 0)
        {
            char* utf8Buffer = new char[utf8Size];
            WideCharToMultiByte(CP_UTF8, 0, fullLogEntry, -1, utf8Buffer, utf8Size, NULL, NULL);
            fwrite(utf8Buffer, 1, utf8Size - 1, g_logFileHandle); // -1 to exclude null terminator
            delete[] utf8Buffer;
            
            // 立即刷新到磁盘，确保日志及时写入
            fflush(g_logFileHandle);
        }
    }
    else
    {
        // If we can't open the log file, try to create a simple error log in current directory
        FILE* errorFile = fopen("log_error.txt", "a");
        if (errorFile)
        {
            SYSTEMTIME st;
            GetLocalTime(&st);
            fprintf(errorFile, "[%04d-%02d-%02d %02d:%02d:%02d] Failed to open log file. Message: %s\n",
                    st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, message);
            fflush(errorFile);
            fclose(errorFile);
        }
    }
}

