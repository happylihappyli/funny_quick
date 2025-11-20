#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
基础功能测试脚本
"""

import os
import time
import subprocess
import threading

def check_exe_exists():
    """检查可执行文件是否存在"""
    exe_path = "E:\\GitHub3\\funny_quick\\bin\\funny_quick.exe"
    if os.path.exists(exe_path):
        print(f"✓ 找到可执行文件: {exe_path}")
        return True
    else:
        print(f"✗ 找不到可执行文件: {exe_path}")
        return False

def start_program():
    """启动程序"""
    exe_path = "E:\\GitHub3\\funny_quick\\bin\\funny_quick.exe"
    try:
        print("启动程序...")
        process = subprocess.Popen([exe_path], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE,
                                 creationflags=subprocess.CREATE_NEW_CONSOLE)
        print(f"✓ 程序已启动，进程ID: {process.pid}")
        return process
    except Exception as e:
        print(f"✗ 启动程序失败: {e}")
        return None

def check_logs():
    """检查日志文件"""
    log_files = [
        "bin\\funny_quick.log",
        "bin\\error.log", 
        "bin\\quick_launcher_backup.log"
    ]
    
    print("\n检查日志文件...")
    for log_file in log_files:
        if os.path.exists(log_file):
            try:
                with open(log_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    if content:
                        print(f"✓ 找到日志文件: {log_file}")
                        print(f"  内容预览: {content[-200:]}")
                    else:
                        print(f"✓ 日志文件存在但为空: {log_file}")
            except Exception as e:
                print(f"✗ 读取日志文件失败 {log_file}: {e}")
        else:
            print(f"- 日志文件不存在: {log_file}")

def monitor_process(process):
    """监控进程运行"""
    print("\n监控程序运行状态...")
    try:
        # 等待5秒
        for i in range(5):
            time.sleep(1)
            if process.poll() is not None:
                print(f"✓ 程序已退出，退出代码: {process.returncode}")
                break
            print(f"  程序运行中... {i+1}秒")
        else:
            print("✓ 程序运行5秒后仍然存活")
            
        # 获取输出
        try:
            stdout, stderr = process.communicate(timeout=3)
            if stdout:
                print(f"标准输出: {stdout.decode('utf-8', errors='ignore')}")
            if stderr:
                print(f"错误输出: {stderr.decode('utf-8', errors='ignore')}")
        except subprocess.TimeoutExpired:
            print("程序仍在运行，终止监控")
            process.terminate()
            time.sleep(1)
            if process.poll() is None:
                process.kill()
                
    except Exception as e:
        print(f"监控过程出错: {e}")

def main():
    print("=== BV快启工具箱 - 基础功能测试 ===")
    
    # 检查可执行文件
    if not check_exe_exists():
        return
    
    # 检查日志
    check_logs()
    
    # 启动程序
    process = start_program()
    if not process:
        return
    
    # 监控运行
    monitor_process(process)
    
    # 再次检查日志
    check_logs()
    
    print("\n=== 测试完成 ===")
    print("如果程序能够正常启动并且没有崩溃，说明基础功能正常。")
    print("请手动验证set和help命令的具体功能。")

if __name__ == "__main__":
    main()
    input("按回车键退出...")