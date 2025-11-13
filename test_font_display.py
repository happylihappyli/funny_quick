# -*- coding: utf-8 -*-
"""
测试字体显示效果的脚本
"""
import os
import sys
import subprocess
import time
import psutil
from datetime import datetime

def run_program_with_timeout(program_path, timeout_seconds=10):
    """运行程序并设置超时"""
    try:
        print(f"启动程序: {program_path}")
        start_time = datetime.now()
        print(f"开始时间: {start_time.strftime('%H:%M:%S')}")
        
        # 启动程序
        process = subprocess.Popen([program_path], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE,
                                 universal_newlines=True)
        
        print("程序已启动，PID:", process.pid)
        
        # 等待指定时间
        time.sleep(timeout_seconds)
        
        # 检查程序是否还在运行
        if process.poll() is None:
            print("程序仍在运行中，字体应该已经显示...")
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            print("程序已终止")
        else:
            stdout, stderr = process.communicate()
            print("程序已退出")
            if stdout:
                print("标准输出:", stdout)
            if stderr:
                print("错误输出:", stderr)
        
        end_time = datetime.now()
        print(f"结束时间: {end_time.strftime('%H:%M:%S')}")
        
        return True
        
    except Exception as e:
        print(f"运行程序时出错: {e}")
        return False

def check_log_file():
    """检查日志文件中是否包含字体相关信息"""
    try:
        log_dir = "bin\\log"
        if not os.path.exists(log_dir):
            print("日志目录不存在")
            return False
        
        # 查找最新的日志文件
        log_files = [f for f in os.listdir(log_dir) if f.endswith('.log')]
        if not log_files:
            print("没有找到日志文件")
            return False
        
        latest_log = sorted(log_files)[-1]
        log_path = os.path.join(log_dir, latest_log)
        print(f"检查日志文件: {log_path}")
        
        with open(log_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
        # 检查字体相关日志
        font_logs = []
        for line in content.split('\n'):
            if '字体' in line or 'CreateUIFont' in line or 'ApplyFontToControl' in line:
                font_logs.append(line.strip())
        
        if font_logs:
            print("找到字体相关日志:")
            for log in font_logs:
                print(f"  - {log}")
            return True
        else:
            print("没有找到字体相关日志")
            return False
            
    except Exception as e:
        print(f"检查日志文件时出错: {e}")
        return False

def main():
    print("=== 字体显示效果测试 ===")
    
    # 设置程序路径
    program_path = "bin\\funny_quick.exe"
    
    if not os.path.exists(program_path):
        print(f"程序文件不存在: {program_path}")
        return False
    
    # 检查日志文件（运行前）
    print("\n1. 检查运行前的日志文件...")
    check_log_file()
    
    # 运行程序
    print("\n2. 运行程序测试字体显示...")
    run_program_with_timeout(program_path, timeout_seconds=5)
    
    # 检查日志文件（运行后）
    print("\n3. 检查运行后的日志文件...")
    check_log_file()
    
    print("\n=== 测试完成 ===")
    print("请手动检查以下内容:")
    print("1. 程序窗口中的中文字体是否显示正常（更光滑清晰）")
    print("2. 检查日志文件中的字体创建和应用信息")
    print("3. 比较字体修改前后的显示效果")
    
    return True

if __name__ == "__main__":
    main()