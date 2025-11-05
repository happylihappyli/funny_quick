#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试系统托盘功能 - 简化版
"""

import time
import subprocess
import os

def test_tray_functionality():
    """测试系统托盘功能"""
    print("开始测试系统托盘功能...")
    
    # 1. 启动程序
    print("1. 启动程序...")
    subprocess.Popen(["bin\\quick_launcher.exe"])
    time.sleep(2)  # 等待程序启动
    
    print("程序已启动，请手动测试以下功能：")
    print("  a) 最小化窗口，检查是否隐藏到系统托盘")
    print("  b) 在系统托盘区域双击图标，检查是否恢复窗口")
    print("  c) 在系统托盘区域右键图标，检查是否显示菜单")
    print("  d) 测试热键 Ctrl+Alt+Q 是否能显示/隐藏窗口")
    print("  e) 检查窗口显示时是否自动聚焦到搜索框")
    print("  f) 检查窗口显示时是否自动切换到英文输入法")
    
    input("按Enter键继续测试...")
    
    # 2. 清理：关闭程序
    print("2. 清理：关闭程序...")
    try:
        subprocess.run(["taskkill", "/F", "/IM", "quick_launcher.exe"], check=False)
        print("已尝试终止 quick_launcher.exe 进程")
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