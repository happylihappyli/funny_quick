#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
创建一个简单的测试程序，直接调用ShowBookmarkDialog函数
"""

import ctypes
import os
import sys
import time

# 创建测试程序
test_program = r'''
#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>

// 定义外部函数
extern void ShowBookmarkDialog();
extern void EnterBookmarkMode();
extern void ExitBookmarkMode();

int main() {
    printf("测试网址收藏模式中的中文显示\n");
    printf("1. 调用EnterBookmarkMode进入网址收藏模式\n");
    
    // 直接调用EnterBookmarkMode
    EnterBookmarkMode();
    
    printf("2. 等待5秒，检查中文显示\n");
    Sleep(5000);
    
    printf("3. 调用ExitBookmarkMode退出网址收藏模式\n");
    ExitBookmarkMode();
    
    printf("测试完成\n");
    return 0;
}
'''

# 写入测试程序文件
with open("E:\\github\\funny_quick\\test_bookmark_mode.cpp", "w", encoding="utf-8") as f:
    f.write(test_program)

print("已创建测试程序: test_bookmark_mode.cpp")

# 创建编译脚本
compile_script = r'''
@echo off
chcp 65001
echo 开始编译测试程序...
cd /d E:\github\funny_quick

g++ -o test_bookmark_mode.exe test_bookmark_mode.cpp gui_main.cpp resource.res -lgdi32 -luser32 -lcomdlg32 -lcomctl32 -std=c++23

if %ERRORLEVEL% EQU 0 (
    echo 编译成功
    echo 运行测试程序...
    test_bookmark_mode.exe
) else (
    echo 编译失败
)
'''

with open("E:\\github\\funny_quick\\compile_test.bat", "w", encoding="gb2312") as f:
    f.write(compile_script)

print("已创建编译脚本: compile_test.bat")
print("运行编译脚本...")

# 运行编译脚本
os.system("E:\\github\\funny_quick\\compile_test.bat")

# 播放提示音
os.system("python tts_notification.py")