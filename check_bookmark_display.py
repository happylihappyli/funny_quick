#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查网址收藏模式中的中文显示
"""

import time
import win32gui
import win32con
import win32api
import win32process
import psutil
import os

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

def get_window_text(hwnd):
    """获取窗口中的文本内容"""
    try:
        # 获取窗口标题
        window_title = win32gui.GetWindowText(hwnd)
        print(f"窗口标题: {window_title}")
        
        # 获取子窗口中的文本
        def enum_child_proc(hwnd_child, param):
            class_name = win32gui.GetClassName(hwnd_child)
            window_text = win32gui.GetWindowText(hwnd_child)
            if window_text:
                print(f"  子窗口文本 ({class_name}): {window_text}")
            return True
        
        win32gui.EnumChildWindows(hwnd, enum_child_proc, None)
        
        # 尝试获取静态控件的文本
        static_hwnd = win32gui.FindWindowEx(hwnd, 0, "Static", None)
        while static_hwnd:
            static_text = win32gui.GetWindowText(static_hwnd)
            if static_text:
                print(f"  静态控件文本: {static_text}")
            static_hwnd = win32gui.FindWindowEx(hwnd, static_hwnd, "Static", None)
            
    except Exception as e:
        print(f"获取窗口文本时出错: {e}")

def main():
    print("检查网址收藏模式中的中文显示")
    print("=" * 50)
    
    # 查找程序窗口
    windows = find_window_by_title(["快速启动"])
    if not windows:
        print("未找到程序窗口")
        return
    
    print(f"找到 {len(windows)} 个程序窗口")
    for hwnd, title in windows:
        print(f"\n窗口句柄: {hwnd}, 标题: {title}")
        get_window_text(hwnd)
        
        # 检查是否处于网址收藏模式
        # 查找包含"收藏"或"网址"的文本
        def enum_child_proc(hwnd_child, param):
            class_name = win32gui.GetClassName(hwnd_child)
            window_text = win32gui.GetWindowText(hwnd_child)
            if window_text and ("收藏" in window_text or "网址" in window_text):
                print(f"  找到收藏模式相关文本: {window_text}")
            return True
        
        win32gui.EnumChildWindows(hwnd, enum_child_proc, None)
    
    print("\n检查完成")

if __name__ == "__main__":
    main()