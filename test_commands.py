#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试 set 和 help 命令的功能
"""

import os
import sys
import time
import subprocess
import signal

def log_message(msg):
    """打印日志信息"""
    print(f"[{time.strftime('%H:%M:%S')}] {msg}")
    sys.stdout.flush()

def kill_process_by_name(process_name):
    """根据进程名终止进程"""
    try:
        result = subprocess.run(['taskkill', '/F', '/IM', process_name], 
                              capture_output=True, text=True)
        log_message(f"尝试终止 {process_name}: {result.stdout}")
        return result.returncode == 0
    except Exception as e:
        log_message(f"终止进程失败: {e}")
        return False

def main():
    log_message("=== BV快启工具箱 - set和help命令功能测试 ===")
    
    exe_path = "bin\\funny_quick.exe"
    
    # 检查可执行文件是否存在
    if not os.path.exists(exe_path):
        log_message(f"错误: 可执行文件 {exe_path} 不存在")
        return False
        
    log_message(f"找到可执行文件: {exe_path}")
    
    # 先清理可能存在的进程
    log_message("清理可能存在的进程...")
    kill_process_by_name("funny_quick.exe")
    time.sleep(2)
    
    try:
        # 启动程序
        log_message("启动程序...")
        process = subprocess.Popen([exe_path], 
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE,
                                 text=True,
                                 encoding='utf-8',
                                 errors='ignore')
        
        log_message(f"程序已启动，PID: {process.pid}")
        
        # 等待程序启动
        time.sleep(3)
        
        # 检查进程是否还在运行
        if process.poll() is not None:
            stdout, stderr = process.communicate()
            log_message(f"程序已退出，退出码: {process.returncode}")
            log_message(f"标准输出: {stdout}")
            log_message(f"错误输出: {stderr}")
            return False
            
        log_message("程序运行正常")
        
        # 测试set命令
        log_message("开始测试 set 命令...")
        
        # 模拟输入 "set" 然后回车
        import win32gui
        import win32api
        import win32con
        
        # 查找主窗口
        def enum_windows_proc(hwnd, windows):
            if win32gui.IsWindowVisible(hwnd):
                window_text = win32gui.GetWindowText(hwnd)
                if "BV快启工具箱" in window_text:
                    windows.append((hwnd, window_text))
        
        windows = []
        win32gui.EnumWindows(enum_windows_proc, windows)
        
        if not windows:
            log_message("未找到BV快启工具箱窗口")
            return False
            
        hwnd, title = windows[0]
        log_message(f"找到窗口: {title} (句柄: {hwnd})")
        
        # 激活窗口
        win32gui.SetForegroundWindow(hwnd)
        win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
        time.sleep(1)
        
        # 查找编辑框
        edit_hwnd = win32gui.FindWindowEx(hwnd, None, "EDIT", None)
        if not edit_hwnd:
            log_message("未找到编辑框")
            return False
            
        log_message(f"找到编辑框，句柄: {edit_hwnd}")
        
        # 输入 "set"
        win32gui.SetWindowText(edit_hwnd, "set")
        log_message("已在编辑框中输入 'set'")
        time.sleep(1)
        
        # 发送回车键
        win32api.PostMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32api.PostMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
        log_message("已发送回车键")
        time.sleep(2)
        
        # 检查窗口标题是否变化（设置菜单应该显示）
        new_title = win32gui.GetWindowText(hwnd)
        log_message(f"当前窗口标题: {new_title}")
        
        # 检查提示文本
        def enum_children(hwnd, results):
            child_count = win32gui.GetWindowTextLength(hwnd)
            if child_count > 0:
                window_text = win32gui.GetWindowText(hwnd)
                class_name = win32gui.GetClassName(hwnd)
                results.append((hwnd, class_name, window_text))
            
            # 枚举子窗口
            win32gui.EnumChildWindows(hwnd, enum_children, results)
            
        results = []
        enum_children(hwnd, results)
        
        # 查找提示标签
        for hwnd_child, class_name, text in results:
            if "设置" in text and class_name == "Static":
                log_message(f"找到设置菜单提示: {text}")
                success = True
                break
        else:
            log_message("未找到设置菜单提示文本")
            success = False
            
        if success:
            log_message("✓ set 命令测试成功")
        else:
            log_message("✗ set 命令测试失败")
            
        # 测试help命令
        log_message("开始测试 help 命令...")
        
        # 清空编辑框
        win32gui.SetWindowText(edit_hwnd, "")
        time.sleep(1)
        
        # 输入 "help"
        win32gui.SetWindowText(edit_hwnd, "help")
        log_message("已在编辑框中输入 'help'")
        time.sleep(1)
        
        # 发送回车键
        win32api.PostMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32api.PostMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
        log_message("已发送回车键")
        time.sleep(2)
        
        # 检查是否显示帮助信息
        results = []
        enum_children(hwnd, results)
        
        help_found = False
        for hwnd_child, class_name, text in results:
            if "使用帮助" in text or "快捷命令" in text:
                log_message(f"找到帮助信息: {text}")
                help_found = True
                break
                
        if help_found:
            log_message("✓ help 命令测试成功")
        else:
            log_message("✗ help 命令测试失败")
            
        # 让用户查看结果
        log_message("测试完成，程序将继续运行10秒...")
        time.sleep(10)
        
        # 关闭程序
        log_message("关闭程序...")
        win32gui.PostMessage(hwnd, win32con.WM_CLOSE, 0, 0)
        time.sleep(2)
        
        # 检查进程是否已关闭
        try:
            os.kill(process.pid, signal.SIGTERM)
            log_message("已发送终止信号给进程")
        except:
            pass
            
        # 等待进程结束
        try:
            process.wait(timeout=5)
            log_message("程序已正常退出")
        except subprocess.TimeoutExpired:
            log_message("程序未正常退出，强制终止")
            process.kill()
            
        return success
        
    except Exception as e:
        log_message(f"测试过程中发生异常: {e}")
        import traceback
        traceback.print_exc()
        return False
        
    finally:
        # 确保清理所有可能的进程
        kill_process_by_name("funny_quick.exe")

if __name__ == "__main__":
    main()