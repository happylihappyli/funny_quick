#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试计算模式功能
"""

import subprocess
import time
import win32gui
import win32con
import win32api
import win32process
import psutil

def find_window_by_title(title):
    """通过标题查找窗口"""
    def callback(hwnd, hwnd_list):
        if win32gui.IsWindowVisible(hwnd) and title in win32gui.GetWindowText(hwnd):
            hwnd_list.append(hwnd)
        return True
    
    hwnd_list = []
    win32gui.EnumWindows(callback, hwnd_list)
    return hwnd_list[0] if hwnd_list else None

def send_keys(hwnd, text):
    """向窗口发送文本"""
    win32gui.SetForegroundWindow(hwnd)
    time.sleep(0.1)
    
    # 发送文本
    for char in text:
        if char == '+':
            win32api.keybd_event(win32con.VK_ADD, 0, 0, 0)
            win32api.keybd_event(win32con.VK_ADD, 0, win32con.KEYEVENTF_KEYUP, 0)
        elif char == '=':
            win32api.keybd_event(win32con.VK_OEM_PLUS, 0, 0, 0)
            win32api.keybd_event(win32con.VK_OEM_PLUS, 0, win32con.KEYEVENTF_KEYUP, 0)
        elif char == '-':
            win32api.keybd_event(win32con.VK_OEM_MINUS, 0, 0, 0)
            win32api.keybd_event(win32con.VK_OEM_MINUS, 0, win32con.KEYEVENTF_KEYUP, 0)
        elif char == '*':
            win32api.keybd_event(win32con.VK_MULTIPLY, 0, 0, 0)
            win32api.keybd_event(win32con.VK_MULTIPLY, 0, win32con.KEYEVENTF_KEYUP, 0)
        elif char == '/':
            win32api.keybd_event(win32con.VK_DIVIDE, 0, 0, 0)
            win32api.keybd_event(win32con.VK_DIVIDE, 0, win32con.KEYEVENTF_KEYUP, 0)
        elif char.isdigit():
            vk = win32con.VK_0 + int(char)
            win32api.keybd_event(vk, 0, 0, 0)
            win32api.keybd_event(vk, 0, win32con.KEYEVENTF_KEYUP, 0)
        else:
            # 对于其他字符，发送Unicode字符
            win32api.PostMessage(hwnd, win32con.WM_CHAR, ord(char), 0)
        time.sleep(0.05)
    
    # 发送回车键
    win32api.keybd_event(win32con.VK_RETURN, 0, 0, 0)
    win32api.keybd_event(win32con.VK_RETURN, 0, win32con.KEYEVENTF_KEYUP, 0)

def test_calculator():
    """测试计算器功能"""
    print("启动测试程序...")
    
    # 启动程序
    process = subprocess.Popen(["E:\\github\\funny_quick\\bin\\quick_launcher.exe"])
    time.sleep(2)  # 等待程序启动
    
    try:
        # 查找窗口
        hwnd = find_window_by_title("快速启动器")
        if not hwnd:
            print("未找到程序窗口")
            return
        
        print("找到程序窗口，开始测试...")
        
        # 进入计算模式
        send_keys(hwnd, "js")
        time.sleep(1)
        
        # 测试1+2
        print("测试1+2...")
        send_keys(hwnd, "1+2")
        time.sleep(1)
        
        # 测试3*4
        print("测试3*4...")
        send_keys(hwnd, "3*4")
        time.sleep(1)
        
        # 测试10/2
        print("测试10/2...")
        send_keys(hwnd, "10/2")
        time.sleep(1)
        
        # 测试5-3
        print("测试5-3...")
        send_keys(hwnd, "5-3")
        time.sleep(1)
        
        print("测试完成！")
        
    except Exception as e:
        print(f"测试过程中出错: {e}")
    finally:
        # 关闭程序
        try:
            process.terminate()
        except:
            pass

if __name__ == "__main__":
    test_calculator()