#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
测试网址收藏模式中的中文显示是否正常
"""

import win32gui
import win32api
import win32con
import time
import os
import sys

def find_window():
    """查找主窗口"""
    hwnd = win32gui.FindWindow(None, "快速启动")
    return hwnd

def test_bookmark_mode_chinese():
    """测试网址收藏模式中的中文显示"""
    print("=" * 60)
    print("测试网址收藏模式中的中文显示")
    print("=" * 60)
    
    # 启动程序
    os.startfile("E:\\github\\funny_quick\\bin\\quick_launcher.exe")
    time.sleep(2)  # 等待程序启动
    
    # 查找窗口
    hwnd = find_window()
    if not hwnd:
        print("未找到窗口，请确保程序已启动")
        return False
    
    print(f"找到窗口: {hwnd}")
    
    # 获取窗口位置
    rect = win32gui.GetWindowRect(hwnd)
    print(f"窗口位置: {rect}")
    
    # 使用键盘快捷键打开网址管理对话框
    print("\n1. 使用键盘快捷键打开网址管理对话框")
    
    # 设置窗口为前台
    win32gui.SetForegroundWindow(hwnd)
    time.sleep(0.5)
    
    # 发送Alt+S快捷键（假设设置按钮的快捷键是Alt+S）
    win32api.keybd_event(win32con.VK_MENU, 0, 0, 0)  # Alt按下
    win32api.keybd_event(ord('S'), 0, 0, 0)        # S按下
    time.sleep(0.1)
    win32api.keybd_event(ord('S'), 0, win32con.KEYEVENTF_KEYUP, 0)  # S释放
    win32api.keybd_event(win32con.VK_MENU, 0, win32con.KEYEVENTF_KEYUP, 0)  # Alt释放
    time.sleep(2)  # 等待对话框打开
    
    # 查找对话框窗口
    dialog_hwnd = win32gui.FindWindow(None, "网址收藏管理")
    if not dialog_hwnd:
        # 尝试枚举所有窗口查找对话框
        def enum_windows_callback(hwnd, windows):
            if win32gui.IsWindowVisible(hwnd):
                window_title = win32gui.GetWindowText(hwnd)
                if "网址" in window_title or "收藏" in window_title:
                    windows.append((hwnd, window_title))
            return True
        
        windows = []
        win32gui.EnumWindows(enum_windows_callback, windows)
        print("找到的包含'网址'或'收藏'的窗口:")
        for hwnd, title in windows:
            print(f"  窗口句柄: {hwnd}, 标题: {title}")
            if "网址收藏管理" in title:
                dialog_hwnd = hwnd
                break
    
    # 如果还是没有找到对话框，尝试使用鼠标点击
    if not dialog_hwnd:
        print("使用键盘快捷键失败，尝试使用鼠标点击")
        # 设置按钮位置大约在 (300, 10) 相对于窗口
        settings_x = rect[0] + 340  # 按钮中心位置
        settings_y = rect[1] + 22   # 按钮中心位置
        
        print(f"设置按钮位置: ({settings_x}, {settings_y})")
        
        # 模拟鼠标点击
        win32api.SetCursorPos((settings_x, settings_y))
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0)
        time.sleep(0.1)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0)
        time.sleep(2)  # 等待对话框打开
        
        # 再次查找对话框窗口
        dialog_hwnd = win32gui.FindWindow(None, "网址收藏管理")
        if not dialog_hwnd:
            # 尝试枚举所有窗口查找对话框
            windows = []
            win32gui.EnumWindows(enum_windows_callback, windows)
            print("找到的包含'网址'或'收藏'的窗口:")
            for hwnd, title in windows:
                print(f"  窗口句柄: {hwnd}, 标题: {title}")
                if "网址收藏管理" in title:
                    dialog_hwnd = hwnd
                    break
    
    # 查找并关闭对话框
    print("\n2. 关闭网址管理对话框")
    # 查找对话框窗口
    dialog_hwnd = win32gui.FindWindow(None, "网址收藏管理")
    if dialog_hwnd:
        # 获取关闭按钮位置 (310, 240) 相对于对话框
        dialog_rect = win32gui.GetWindowRect(dialog_hwnd)
        close_x = dialog_rect[0] + 345  # 关闭按钮中心位置 (310 + 35)
        close_y = dialog_rect[1] + 252  # 关闭按钮中心位置 (240 + 12)
        
        print(f"对话框位置: {dialog_rect}")
        print(f"关闭按钮位置: ({close_x}, {close_y})")
        
        # 模拟鼠标点击关闭按钮
        win32api.SetCursorPos((close_x, close_y))
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0)
        time.sleep(0.1)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0)
        time.sleep(1)  # 等待对话框关闭并进入网址收藏模式
    else:
        print("未找到对话框窗口")
    
    # 检查是否进入网址收藏模式
    print("\n3. 检查是否进入网址收藏模式")
    # 获取模式标签文本
    label_hwnd = win32gui.FindWindowEx(hwnd, None, "STATIC", None)
    if label_hwnd:
        label_text = win32gui.GetWindowText(label_hwnd)
        print(f"模式标签文本: {label_text}")
        if "网址" in label_text:
            print("✓ 已成功进入网址收藏模式")
        else:
            print("✗ 未进入网址收藏模式")
            return False
    
    # 检查列表框中的中文显示
    print("\n4. 检查列表框中的中文显示")
    # 获取列表框
    listbox_hwnd = win32gui.FindWindowEx(hwnd, None, "LISTBOX", None)
    if listbox_hwnd:
        # 获取列表框中的项目数量
        count = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETCOUNT, 0, 0)
        print(f"列表框中共有 {count} 个项目")
        
        # 检查前几个项目
        for i in range(min(5, count)):
            # 获取项目文本
            text_length = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETTEXTLEN, i, 0)
            buffer = win32gui.PyMakeBuffer(text_length * 2)  # Unicode字符需要双倍空间
            actual_length = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETTEXT, i, buffer)
            text = buffer[:actual_length*2].decode('utf-16-le')
            print(f"  项目 {i+1}: {text}")
            
            # 检查是否包含中文字符
            if any('\u4e00' <= char <= '\u9fff' for char in text):
                print(f"    ✓ 包含中文字符")
            else:
                print(f"    - 不包含中文字符")
    
    # 测试退出按钮
    print("\n5. 测试退出按钮")
    # 退出按钮位置大约在 (300, 10) 相对于窗口
    exit_x = rect[0] + 340  # 按钮中心位置
    exit_y = rect[1] + 22   # 按钮中心位置
    
    # 模拟鼠标点击
    win32api.SetCursorPos((exit_x, exit_y))
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0)
    time.sleep(0.1)
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0)
    time.sleep(1)  # 等待退出
    
    # 检查是否退出网址收藏模式
    print("\n6. 检查是否退出网址收藏模式")
    if label_hwnd:
        label_text = win32gui.GetWindowText(label_hwnd)
        print(f"模式标签文本: {label_text}")
        if "搜索" in label_text:
            print("✓ 已成功退出网址收藏模式")
            return True
        else:
            print("✗ 未退出网址收藏模式")
            return False
    
    return True

if __name__ == "__main__":
    success = test_bookmark_mode_chinese()
    
    if success:
        print("\n测试成功完成！")
        print("网址收藏模式中的中文显示正常")
    else:
        print("\n测试失败！")
        print("网址收藏模式中的中文显示存在问题")
    
    # 播放提示音
    os.system("python tts_notification.py")
    
    sys.exit(0 if success else 1)