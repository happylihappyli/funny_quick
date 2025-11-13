#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试网址收藏管理页面的标签中文显示
"""

import time
import win32gui
import win32con
import win32api

def find_window_by_title(title_keywords):
    """根据标题关键词查找窗口"""
    def callback(hwnd, hwnd_list):
        if win32gui.IsWindowVisible(hwnd):
            window_title = win32gui.GetWindowText(hwnd)
            if any(keyword in window_title for keyword in title_keywords):
                hwnd_list.append((hwnd, window_title))
        return True
    
    hwnd_list = []
    win32gui.EnumWindows(callback, hwnd_list)
    return hwnd_list

def find_child_windows(parent_hwnd):
    """查找父窗口下的所有子窗口"""
    child_windows = []
    
    def enum_child_proc(hwnd_child, param):
        class_name = win32gui.GetClassName(hwnd_child)
        window_text = win32gui.GetWindowText(hwnd_child)
        rect = win32gui.GetWindowRect(hwnd_child)
        child_windows.append({
            'hwnd': hwnd_child,
            'class_name': class_name,
            'text': window_text,
            'rect': rect
        })
        return True
    
    win32gui.EnumChildWindows(parent_hwnd, enum_child_proc, None)
    return child_windows

def main():
    print("测试网址收藏管理页面的标签中文显示")
    print("=" * 50)
    
    # 查找程序窗口
    windows = find_window_by_title(["快速启动"])
    if not windows:
        print("未找到程序窗口")
        return
    
    print(f"找到 {len(windows)} 个程序窗口")
    hwnd, title = windows[0]
    print(f"窗口句柄: {hwnd}, 标题: {title}")
    
    # 查找设置按钮并点击
    print("\n1. 查找并点击设置按钮")
    child_windows = find_child_windows(hwnd)
    settings_button = None
    
    for child in child_windows:
        if child['class_name'] == 'Button':
            print(f"找到按钮: '{child['text']}'")
            if '设置' in child['text']:
                settings_button = child
                break
    
    if not settings_button:
        print("未找到设置按钮")
        return
    
    # 点击设置按钮
    rect = settings_button['rect']
    center_x = (rect[0] + rect[2]) // 2
    center_y = (rect[1] + rect[3]) // 2
    
    print(f"点击设置按钮，位置: ({center_x}, {center_y})")
    win32api.SetCursorPos((center_x, center_y))
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0)
    time.sleep(0.1)
    win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0)
    time.sleep(2)  # 等待对话框打开
    
    # 查找网址收藏管理对话框
    print("\n2. 查找网址收藏管理对话框")
    dialog_windows = find_window_by_title(["网址收藏管理"])
    if not dialog_windows:
        print("未找到网址收藏管理对话框，尝试查找所有窗口")
        # 查找所有窗口
        all_windows = []
        def enum_windows_callback(hwnd, param):
            if win32gui.IsWindowVisible(hwnd):
                window_title = win32gui.GetWindowText(hwnd)
                if window_title:  # 只显示有标题的窗口
                    all_windows.append((hwnd, window_title))
            return True
        win32gui.EnumWindows(enum_windows_callback, None)
        
        for hwnd, title in all_windows:
            print(f"窗口: {title}")
            if "网址" in title or "收藏" in title:
                print(f"  -> 可能是目标对话框")
                dialog_windows = [(hwnd, title)]
                break
    
    if not dialog_windows:
        print("仍未找到网址收藏管理对话框")
        return
    
    dialog_hwnd, dialog_title = dialog_windows[0]
    print(f"找到对话框: {dialog_title}")
    
    # 检查对话框中的标签文本
    print("\n3. 检查对话框中的标签文本")
    child_windows = find_child_windows(dialog_hwnd)
    
    for child in child_windows:
        if child['class_name'] == 'Static':
            label_text = child['text']
            if label_text:  # 只检查非空标签
                print(f"标签文本: '{label_text}'")
                
                # 检查是否包含中文字符
                if any('\u4e00' <= char <= '\u9fff' for char in label_text):
                    print(f"  ✓ 包含中文字符，显示正常")
                else:
                    print(f"  - 不包含中文字符，可能是英文标签")
    
    # 关闭对话框
    print("\n4. 关闭对话框")
    # 查找关闭按钮
    close_button = None
    for child in child_windows:
        if child['class_name'] == 'Button' and '关闭' in child['text']:
            close_button = child
            break
    
    if close_button:
        rect = close_button['rect']
        center_x = (rect[0] + rect[2]) // 2
        center_y = (rect[1] + rect[3]) // 2
        
        print(f"点击关闭按钮，位置: ({center_x}, {center_y})")
        win32api.SetCursorPos((center_x, center_y))
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0)
        time.sleep(0.1)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0)
    else:
        print("未找到关闭按钮")
    
    print("\n测试完成")

if __name__ == "__main__":
    main()