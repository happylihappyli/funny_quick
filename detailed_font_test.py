# -*- coding: utf-8 -*-
"""
详细测试字体显示效果的脚本
"""
import os
import sys
import subprocess
import time
import glob
from datetime import datetime

def run_program_detailed(program_path, timeout_seconds=10):
    """详细运行程序并收集输出"""
    try:
        print(f"启动程序: {program_path}")
        start_time = datetime.now()
        print(f"开始时间: {start_time.strftime('%H:%M:%S')}")
        
        # 获取运行前的日志文件列表
        log_dir = "bin\\log"
        if os.path.exists(log_dir):
            old_logs = set(os.listdir(log_dir))
            old_logs = {f for f in old_logs if f.endswith('.log')}
        else:
            old_logs = set()
        
        # 启动程序
        process = subprocess.Popen([program_path], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE,
                                 universal_newlines=True,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
        
        print("程序已启动，PID:", process.pid)
        
        # 等待指定时间
        time.sleep(timeout_seconds)
        
        # 获取运行后的日志文件列表
        if os.path.exists(log_dir):
            new_logs = set(os.listdir(log_dir))
            new_logs = {f for f in new_logs if f.endswith('.log')}
            new_files = new_logs - old_logs
        else:
            new_files = set()
        
        print(f"发现新日志文件: {new_files}")
        
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
        
        # 返回新创建的日志文件列表
        return new_files
        
    except Exception as e:
        print(f"运行程序时出错: {e}")
        return set()

def check_all_log_files(log_files):
    """检查所有日志文件中是否包含字体相关信息"""
    font_logs_found = False
    for log_file in log_files:
        log_path = f"bin\\log\\{log_file}"
        try:
            with open(log_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            print(f"\n=== 检查日志文件: {log_file} ===")
            
            # 检查字体相关日志
            font_keywords = ['字体', 'CreateUIFont', 'ApplyFontToControl', 'UI字体已创建', '字体句柄']
            font_logs = []
            for line in content.split('\n'):
                for keyword in font_keywords:
                    if keyword in line:
                        font_logs.append(line.strip())
                        break
            
            if font_logs:
                print("找到字体相关日志:")
                for log in font_logs:
                    print(f"  - {log}")
                font_logs_found = True
            else:
                print("没有找到字体相关日志")
                
            # 显示文件的前几行和后几行
            lines = content.split('\n')
            if len(lines) > 0:
                print(f"文件总行数: {len(lines)}")
                print("前5行:")
                for line in lines[:5]:
                    if line.strip():
                        print(f"  {line}")
                if len(lines) > 10:
                    print("后5行:")
                    for line in lines[-5:]:
                        if line.strip():
                            print(f"  {line}")
                            
        except Exception as e:
            print(f"检查日志文件 {log_file} 时出错: {e}")
    
    return font_logs_found

def main():
    print("=== 字体显示效果详细测试 ===")
    
    # 设置程序路径
    program_path = "bin\\funny_quick.exe"
    
    if not os.path.exists(program_path):
        print(f"程序文件不存在: {program_path}")
        return False
    
    # 运行程序
    print("\n运行程序测试字体显示...")
    new_log_files = run_program_detailed(program_path, timeout_seconds=8)
    
    # 检查日志文件
    print("\n检查日志文件...")
    font_logs_found = check_all_log_files(new_log_files)
    
    print(f"\n=== 测试完成 ===")
    print(f"字体相关日志找到: {'是' if font_logs_found else '否'}")
    
    if font_logs_found:
        print("✅ 字体功能正常：日志中找到了字体创建和应用相关的信息")
        print("✅ 字体应该已经应用到控件上，界面显示应该更光滑清晰")
    else:
        print("⚠️  字体功能可能有问题：日志中没有找到字体相关信息")
        print("⚠️  可能原因：")
        print("  1. 程序没有执行到字体创建代码")
        print("  2. 日志记录有问题")
        print("  3. 程序提前退出")
    
    print("\n手动检查建议:")
    print("1. 运行程序并观察窗口中文字体的显示效果")
    print("2. 对比修改前后的字体清晰度和光滑度")
    print("3. 检查不同控件的字体一致性")
    
    return True

if __name__ == "__main__":
    main()