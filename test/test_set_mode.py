#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试set模式内容显示问题的Python脚本

此脚本用于验证set模式中WebView内容是否能够正常显示
"""

import os
import subprocess
import time
import psutil
import sys

def check_process_running(process_name):
    """检查指定进程是否正在运行"""
    for proc in psutil.process_iter(['name']):
        try:
            if process_name.lower() in proc.info['name'].lower():
                return True
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    return False

def main():
    """主函数"""
    print("🔍 开始测试set模式内容显示问题")
    
    # 检查程序是否正在运行
    if check_process_running("funny_quick.exe"):
        print("✅ 程序正在运行中")
        print("💡 请在程序界面中输入 'set' 命令，然后检查WebView中是否显示设置菜单内容")
        print("📋 期望显示的内容：")
        print("   - 退出程序")
        print("   - 快捷方式管理") 
        print("   - 系统设置")
        print("   - 关于软件")
        print("")
        print("🔧 如果内容为空，可能是以下原因：")
        print("   1. 模板文件读取失败")
        print("   2. 占位符替换失败")
        print("   3. WebView2初始化问题")
        print("")
        print("📊 请检查程序日志文件以获取详细信息")
    else:
        print("❌ 程序未运行，请先启动程序")
        print("💡 启动命令: .\\bin\\funny_quick.exe")
    
    print("\n📝 测试完成")

if __name__ == "__main__":
    main()