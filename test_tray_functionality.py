#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试系统托盘功能
"""

import time
import subprocess
import psutil
import pyautogui
import os

def is_process_running(process_name):
    """检查进程是否正在运行"""
    for proc in psutil.process_iter(['pid', 'name']):
        try:
            if process_name.lower() in proc.info['name'].lower():
                return True
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return False

def test_tray_functionality():
    """测试系统托盘功能"""
    print("开始测试系统托盘功能...")
    
    # 1. 启动程序
    print("1. 启动程序...")
    subprocess.Popen(["bin\\quick_launcher.exe"])
    time.sleep(2)  # 等待程序启动
    
    if not is_process_running("quick_launcher.exe"):
        print("错误：程序未能成功启动")
        return False
    
    print("程序已成功启动")
    
    # 2. 测试最小化到托盘
    print("2. 测试最小化到托盘...")
    try:
        # 模拟点击最小化按钮
        pyautogui.click(pyautogui.size()[0]//2, 20)  # 点击窗口标题栏中间位置
        time.sleep(0.5)
        pyautogui.press('space')  # 激活窗口
        time.sleep(0.5)
        pyautogui.hotkey('alt', 'space')  # 打开窗口菜单
        time.sleep(0.5)
        pyautogui.press('n')  # 选择最小化
        time.sleep(2)
        print("窗口已最小化，应该隐藏到系统托盘")
    except Exception as e:
        print(f"最小化操作失败: {e}")
    
    # 3. 测试托盘图标双击
    print("3. 测试托盘图标双击...")
    try:
        # 尝试在系统托盘区域双击
        tray_x, tray_y = pyautogui.size()[0]-100, pyautogui.size()[1]-20
        pyautogui.doubleClick(tray_x, tray_y)
        time.sleep(2)
        print("已尝试双击系统托盘区域")
    except Exception as e:
        print(f"托盘双击操作失败: {e}")
    
    # 4. 测试托盘右键菜单
    print("4. 测试托盘右键菜单...")
    try:
        # 尝试在系统托盘区域右键
        tray_x, tray_y = pyautogui.size()[0]-100, pyautogui.size()[1]-20
        pyautogui.rightClick(tray_x, tray_y)
        time.sleep(1)
        pyautogui.press('down')  # 选择菜单项
        time.sleep(0.5)
        pyautogui.press('enter')  # 确认选择
        time.sleep(2)
        print("已尝试右键系统托盘区域")
    except Exception as e:
        print(f"托盘右键操作失败: {e}")
    
    # 5. 测试热键功能
    print("5. 测试热键功能...")
    try:
        # 测试Ctrl+Alt+Q热键
        pyautogui.hotkey('ctrl', 'alt', 'q')
        time.sleep(2)
        print("已尝试Ctrl+Alt+Q热键")
    except Exception as e:
        print(f"热键操作失败: {e}")
    
    # 6. 清理：关闭程序
    print("6. 清理：关闭程序...")
    try:
        for proc in psutil.process_iter(['pid', 'name']):
            try:
                if "quick_launcher.exe" in proc.info['name']:
                    proc.terminate()
                    print(f"已终止进程: {proc.info['name']} (PID: {proc.info['pid']})")
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                pass
    except Exception as e:
        print(f"清理进程失败: {e}")
    
    print("测试完成！")
    return True

if __name__ == "__main__":
    # 检查程序是否存在
    if not os.path.exists("bin\\quick_launcher.exe"):
        print("错误：找不到 bin\\quick_launcher.exe，请先编译程序")
        exit(1)
    
    test_tray_functionality()