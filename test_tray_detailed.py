#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试系统托盘功能 - 详细版
"""

import time
import subprocess
import os
import sys

def check_latest_log():
    """检查最新的日志文件，确认功能是否正常工作"""
    log_dir = "log"
    if not os.path.exists(log_dir):
        print("日志目录不存在")
        return False
    
    # 获取最新的日志文件
    log_files = [f for f in os.listdir(log_dir) if f.endswith('.log')]
    if not log_files:
        print("没有找到日志文件")
        return False
    
    # 按文件名排序（文件名包含时间戳）
    log_files.sort()
    latest_log = os.path.join(log_dir, log_files[-1])
    
    print(f"检查日志文件: {latest_log}")
    
    # 检查日志中的关键信息
    with open(latest_log, 'r', encoding='utf-8') as f:
        content = f.read()
        
        # 检查系统托盘相关日志
        if "AddTrayIcon: 成功添加系统托盘图标" in content:
            print("✓ 系统托盘图标添加成功")
        else:
            print("✗ 系统托盘图标添加失败或未找到日志")
            
        if "CreateTrayMenu: 成功创建托盘右键菜单" in content:
            print("✓ 系统托盘菜单创建成功")
        else:
            print("✗ 系统托盘菜单创建失败或未找到日志")
            
        if "System tray icon and menu initialized" in content:
            print("✓ 系统托盘初始化成功")
        else:
            print("✗ 系统托盘初始化失败或未找到日志")
            
        if "WM_SIZE: 窗口最小化，隐藏到托盘" in content:
            print("✓ 窗口最小化到托盘功能正常")
        else:
            print("✗ 窗口最小化到托盘功能未测试或失败")
            
        if "HandleTrayMessage: 双击托盘图标，显示窗口" in content:
            print("✓ 托盘图标双击功能正常")
        else:
            print("✗ 托盘图标双击功能未测试或失败")
            
        if "HandleTrayMessage: 用户选择显示窗口" in content:
            print("✓ 托盘菜单显示窗口功能正常")
        else:
            print("✗ 托盘菜单显示窗口功能未测试或失败")
            
        if "SetEnglishInputMethod: 已切换到英文输入法" in content:
            print("✓ 自动切换到英文输入法功能正常")
        else:
            print("✗ 自动切换到英文输入法功能未测试或失败")
            
        if "Setting focus to edit control" in content:
            print("✓ 自动聚焦到搜索框功能正常")
        else:
            print("✗ 自动聚焦到搜索框功能未测试或失败")
            
        # 显示完整的日志内容
        print("\n完整日志内容:")
        print("-" * 50)
        print(content)
        print("-" * 50)
        
        return True

def test_tray_functionality():
    """测试系统托盘功能"""
    print("开始测试系统托盘功能...")
    
    # 1. 启动程序
    print("1. 启动程序...")
    subprocess.Popen(["bin\\quick_launcher.exe"])
    time.sleep(3)  # 等待程序启动
    
    print("程序已启动，请手动测试以下功能：")
    print("  a) 最小化窗口，检查是否隐藏到系统托盘")
    print("  b) 在系统托盘区域双击图标，检查是否恢复窗口")
    print("  c) 在系统托盘区域右键图标，检查是否显示菜单")
    print("  d) 测试热键 Ctrl+Alt+Q 是否能显示/隐藏窗口")
    print("  e) 检查窗口显示时是否自动聚焦到搜索框")
    print("  f) 检查窗口显示时是否自动切换到英文输入法")
    
    input("\n完成测试后按Enter键继续...")
    
    # 2. 清理：关闭程序
    print("2. 清理：关闭程序...")
    try:
        subprocess.run(["taskkill", "/F", "/IM", "quick_launcher.exe"], check=False)
        print("已尝试终止 quick_launcher.exe 进程")
    except Exception as e:
        print(f"清理进程失败: {e}")
    
    # 3. 检查日志
    print("3. 检查日志文件...")
    check_latest_log()
    
    print("\n测试完成！")
    return True

if __name__ == "__main__":
    # 检查程序是否存在
    if not os.path.exists("bin\\quick_launcher.exe"):
        print("错误：找不到 bin\\quick_launcher.exe，请先编译程序")
        sys.exit(1)
    
    test_tray_functionality()