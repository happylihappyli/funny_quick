/**
 * @file window_size_handler.cpp
 * @brief 窗口大小处理相关函数实现
 * @author AI Assistant
 * @date 2024
 */

#include "window_size_handler.h"
#include "logger.h"
#include "ui_manager.h"

/**
 * @brief 处理WM_EXITSIZEMOVE消息，处理窗口大小调整或移动完成事件
 * @param hwnd 窗口句柄
 * @return 成功返回0
 */
LRESULT HandleWMExitSizeMove(HWND hwnd)
{
    // 用户完成调整窗口大小或移动窗口后，保存窗口设置
    LogToFile("WM_EXITSIZEMOVE: 窗口大小调整完成，保存窗口设置");
    SaveWindowSettings();
    return 0;
}