/**
 * @file window_size_handler.h
 * @brief 窗口大小处理相关函数声明
 * @author AI Assistant
 * @date 2024
 */

#pragma once

#include <windows.h>
#include "common.h"

/**
 * @brief 处理WM_EXITSIZEMOVE消息，处理窗口大小调整或移动完成事件
 * @param hwnd 窗口句柄
 * @return 成功返回0
 */
LRESULT HandleWMExitSizeMove(HWND hwnd);