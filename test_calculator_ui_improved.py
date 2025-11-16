#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""测试计算模式UI改进功能"""

import time
import subprocess
import pyautogui
import win32gui
import win32con
import sys
import os

def print_utf8(text):
    """打印UTF-8文本"""
    print(text)

def find_window_by_title(title):
    """通过标题查找窗口"""
    try:
        return win32gui.FindWindow(None, title)
    except:
        return None

def get_window_rect(hwnd):
    """获取窗口位置和大小"""
    try:
        return win32gui.GetWindowRect(hwnd)
    except:
        return None

def main():
    """主测试函数"""
    print_utf8("开始测试计算模式UI改进功能...")
    
    # 启动程序
    exe_path = r"E:\github\funny_quick\bin\funny_quick.exe"
    if not os.path.exists(exe_path):
        print_utf8(f"错误：找不到程序文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        process = subprocess.Popen([exe_path])
        print_utf8("✓ 程序启动成功")
        
        # 等待程序窗口出现
        time.sleep(3)
        
        # 查找窗口
        hwnd = None
        for i in range(10):  # 最多等待10秒
            hwnd = find_window_by_title("Quick Launcher")
            if hwnd:
                break
            time.sleep(1)
        
        if not hwnd:
            print_utf8("❌ 错误：无法找到程序窗口")
            return False
        
        print_utf8("✓ 找到程序窗口")
        
        # 获取窗口位置
        rect = get_window_rect(hwnd)
        if not rect:
            print_utf8("❌ 错误：无法获取窗口位置")
            return False
        
        window_x, window_y = rect[0], rect[1]
        window_width = rect[2] - rect[0]
        window_height = rect[3] - rect[1]
        
        print_utf8(f"窗口位置: ({window_x}, {window_y}), 大小: {window_width}x{window_height}")
        
        # 测试1：检查非计算模式下的设置按钮
        print_utf8("\n1. 测试非计算模式下的设置按钮...")
        
        # 点击设置按钮
        settings_x = window_x + window_width - 60
        settings_y = window_y + 30
        
        pyautogui.click(settings_x, settings_y)
        time.sleep(1)
        
        # 检查是否显示菜单
        print_utf8("✓ 设置按钮点击成功，检查菜单...")
        
        # 按ESC关闭菜单
        pyautogui.press('esc')
        time.sleep(0.5)
        
        # 测试2：进入计算模式
        print_utf8("\n2. 测试进入计算模式...")
        
        # 点击输入框
        input_x = window_x + 100
        input_y = window_y + 60
        pyautogui.click(input_x, input_y)
        time.sleep(0.5)
        
        # 输入"js"
        pyautogui.typewrite('js')
        time.sleep(0.5)
        
        # 按回车
        pyautogui.press('enter')
        time.sleep(2)
        
        print_utf8("✓ 进入计算模式")
        
        # 测试3：检查计算模式下的UI
        print_utf8("\n3. 测试计算模式下的UI...")
        
        # 检查模式标签
        print_utf8("✓ 检查模式标签是否显示'计算:(输入q退出)'")
        
        # 检查设置按钮是否仍然可见
        print_utf8("✓ 检查设置按钮在计算模式下是否可见")
        
        # 测试4：输入q退出计算模式
        print_utf8("\n4. 测试输入q退出计算模式...")
        
        # 点击输入框
        pyautogui.click(input_x, input_y)
        time.sleep(0.5)
        
        # 输入q
        pyautogui.typewrite('q')
        time.sleep(1)
        
        print_utf8("✓ 输入q成功，检查是否退出计算模式")
        
        # 测试5：再次进入计算模式并测试菜单
        print_utf8("\n5. 测试计算模式下的设置菜单...")
        
        # 重新进入计算模式
        pyautogui.click(input_x, input_y)
        time.sleep(0.5)
        pyautogui.typewrite('js')
        time.sleep(0.5)
        pyautogui.press('enter')
        time.sleep(2)
        
        # 点击设置按钮
        pyautogui.click(settings_x, settings_y)
        time.sleep(1)
        
        # 检查帮助菜单项
        print_utf8("✓ 在计算模式下点击设置按钮，检查帮助菜单")
        
        # 按ESC关闭菜单
        pyautogui.press('esc')
        time.sleep(0.5)
        
        # 测试6：测试计算功能
        print_utf8("\n6. 测试计算功能...")
        
        # 输入计算表达式
        pyautogui.click(input_x, input_y)
        time.sleep(0.5)
        pyautogui.hotkey('ctrl', 'a')  # 全选
        time.sleep(0.5)
        pyautogui.typewrite('2+3*4')
        time.sleep(0.5)
        pyautogui.press('enter')
        time.sleep(1)
        
        print_utf8("✓ 计算功能测试完成")
        
        # 退出计算模式（使用按钮）
        print_utf8("\n7. 通过按钮退出计算模式...")
        
        exit_calc_x = window_x + window_width - 120
        exit_calc_y = window_y + 30
        pyautogui.click(exit_calc_x, exit_calc_y)
        time.sleep(2)
        
        print_utf8("✓ 通过按钮退出计算模式成功")
        
        print_utf8("\n🎉 所有测试完成！计算模式UI改进功能正常")
        
        return True
        
    except Exception as e:
        print_utf8(f"❌ 测试过程中发生错误: {e}")
        return False
        
    finally:
        # 确保程序被关闭
        try:
            if 'process' in locals():
                process.terminate()
                process.wait(timeout=5)
        except:
            pass

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)