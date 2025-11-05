#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试系统托盘图标功能
"""

import subprocess
import time
import os
import sys

def main():
    print("=== 测试系统托盘图标功能 ===")
    
    # 启动程序
    print("1. 启动 quick_launcher.exe...")
    process = subprocess.Popen([os.path.join("bin", "quick_launcher.exe")])
    print(f"   程序已启动，PID: {process.pid}")
    
    # 等待程序完全启动
    print("2. 等待程序完全启动...")
    time.sleep(3)
    
    # 测试说明
    print("\n3. 请手动执行以下测试步骤:")
    print("   a) 查看系统托盘区域，确认是否有图标显示")
    print("   b) 点击窗口最小化按钮，确认窗口最小化到托盘")
    print("   c) 右键点击托盘图标，确认右键菜单正常显示")
    print("   d) 双击托盘图标，确认窗口恢复显示")
    print("   e) 测试热键 Ctrl+Alt+Q，确认程序正常退出")
    
    # 等待用户测试
    input("\n按回车键继续...")
    
    # 检查进程是否还在运行
    if process.poll() is None:
        print("\n4. 程序仍在运行，正在终止...")
        process.terminate()
        process.wait(timeout=5)
        print("   程序已终止")
    else:
        print("\n4. 程序已退出")
    
    print("\n=== 测试完成 ===")

if __name__ == "__main__":
    main()