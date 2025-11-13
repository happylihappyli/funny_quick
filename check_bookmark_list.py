#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查网址收藏列表中的中文显示
"""

import time
import win32gui
import win32con
import win32api
import win32process

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

def get_listbox_items(hwnd):
    """获取列表框中的所有项目"""
    try:
        # 查找列表框控件
        listbox_hwnd = win32gui.FindWindowEx(hwnd, 0, "ListBox", None)
        if not listbox_hwnd:
            print("未找到列表框控件")
            return []
        
        # 获取列表框中的项目数量
        count = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETCOUNT, 0, 0)
        print(f"列表框中共有 {count} 个项目")
        
        items = []
        for i in range(count):
            # 获取每个项目的文本
            try:
                # 使用WM_GETTEXT消息获取文本
                item_hwnd = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETITEMRECT, i, 0)
                if item_hwnd:
                    text_length = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETTEXTLEN, i, 0)
                    if text_length > 0:
                        # 创建缓冲区
                        buffer = win32gui.PyMakeBuffer(text_length + 1)
                        win32gui.SendMessage(listbox_hwnd, win32con.LB_GETTEXT, i, buffer)
                        item_text = win32gui.PyGetString(buffer)
                        items.append(item_text)
                        print(f"  项目 {i}: {item_text}")
            except Exception as e:
                print(f"  获取项目 {i} 时出错: {e}")
        
        return items
    except Exception as e:
        print(f"获取列表框项目时出错: {e}")
        return []

def main():
    print("检查网址收藏列表中的中文显示")
    print("=" * 50)
    
    # 查找程序窗口
    windows = find_window_by_title(["快速启动"])
    if not windows:
        print("未找到程序窗口")
        return
    
    print(f"找到 {len(windows)} 个程序窗口")
    for hwnd, title in windows:
        print(f"\n窗口句柄: {hwnd}, 标题: {title}")
        items = get_listbox_items(hwnd)
        
        # 检查是否有中文项目
        chinese_items = [item for item in items if any('\u4e00' <= char <= '\u9fff' for char in item)]
        if chinese_items:
            print(f"\n找到 {len(chinese_items)} 个包含中文的项目:")
            for item in chinese_items:
                print(f"  - {item}")
        else:
            print("\n未找到包含中文的项目")
    
    print("\n检查完成")

if __name__ == "__main__":
    main()