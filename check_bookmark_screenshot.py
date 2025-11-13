#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
检查网址收藏列表中的中文显示 - 使用截图方法
"""

import time
import win32gui
import win32con
import win32api
import win32process
import os
import subprocess
from PIL import Image, ImageGrab, ImageDraw, ImageFont

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

def get_window_rect(hwnd):
    """获取窗口矩形区域"""
    try:
        left, top, right, bottom = win32gui.GetWindowRect(hwnd)
        return (left, top, right, bottom)
    except Exception as e:
        print(f"获取窗口区域时出错: {e}")
        return None

def capture_window(hwnd):
    """截取窗口图像"""
    try:
        # 获取窗口区域
        rect = get_window_rect(hwnd)
        if not rect:
            return None
            
        left, top, right, bottom = rect
        width = right - left
        height = bottom - top
        
        # 截取窗口图像
        screenshot = ImageGrab.grab(bbox=(left, top, right, bottom))
        return screenshot
    except Exception as e:
        print(f"截取窗口图像时出错: {e}")
        return None

def save_screenshot(image, filename):
    """保存截图"""
    try:
        image.save(filename)
        print(f"截图已保存到: {filename}")
        return True
    except Exception as e:
        print(f"保存截图时出错: {e}")
        return False

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
        
        # 截取窗口图像
        screenshot = capture_window(hwnd)
        if screenshot:
            # 保存截图
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            filename = f"bookmark_mode_{timestamp}.png"
            if save_screenshot(screenshot, filename):
                print(f"已保存网址收藏模式截图: {filename}")
                
                # 尝试打开截图
                try:
                    os.startfile(filename)
                except:
                    print("无法自动打开截图，请手动查看文件")
        else:
            print("无法截取窗口图像")
    
    print("\n检查完成 - 请查看截图以确认中文显示是否正常")

if __name__ == "__main__":
    main()