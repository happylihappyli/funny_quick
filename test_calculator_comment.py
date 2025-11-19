#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
计算器注释功能测试脚本
测试在计算模式中输入"1+2 #xxx"时的注释提取功能
"""

import time
import sys
import os

# 添加路径
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

try:
    import win32gui
    import win32con
    import win32api
    import win32ui
    from ctypes import wintypes
    import psutil
except ImportError:
    print("需要安装pywin32和psutil库：")
    print("pip install pywin32 psutil")
    sys.exit(1)

def find_main_window():
    """查找主窗口"""
    def enum_windows_callback(hwnd, windows):
        if win32gui.IsWindowVisible(hwnd):
            window_text = win32gui.GetWindowText(hwnd)
            if "快速启动" in window_text:
                windows.append((hwnd, window_text))
        return True
    
    windows = []
    win32gui.EnumWindows(enum_windows_callback, windows)
    
    if windows:
        return windows[0][0]  # 返回第一个匹配的窗口句柄
    else:
        print("错误：未找到快速启动窗口")
        return None

def enter_calculator_mode(hwnd):
    """进入计算模式"""
    print("尝试进入计算模式...")
    
    # 查找编辑控件
    edit_hwnd = win32gui.FindWindowEx(hwnd, 0, "Edit", None)
    if not edit_hwnd:
        print("错误：未找到编辑控件")
        return False
    
    # 获取当前模式（通过标签文本）
    label_hwnd = win32gui.FindWindowEx(hwnd, 0, "Static", None)
    if label_hwnd:
        label_text = win32gui.GetWindowText(label_hwnd)
        print(f"当前标签文本: {label_text}")
    
    # 输入计算模式命令
    win32gui.SetForegroundWindow(hwnd)
    win32gui.SetFocus(edit_hwnd)
    
    # 清空编辑框
    win32gui.SendMessage(edit_hwnd, win32con.EM_SETSEL, 0, -1)
    win32gui.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_DELETE, 0)
    win32gui.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_DELETE, 0)
    
    # 输入进入计算模式的命令
    calc_command = "calculator"
    for char in calc_command:
        win32gui.SendMessage(edit_hwnd, win32con.WM_CHAR, ord(char), 0)
    
    # 按回车键
    win32gui.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
    win32gui.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
    
    time.sleep(1)  # 等待模式切换
    
    # 检查是否成功进入计算模式
    if label_hwnd:
        new_label_text = win32gui.GetWindowText(label_hwnd)
        if "计算模式" in new_label_text:
            print("成功进入计算模式")
            return True
        else:
            print(f"标签文本未变化: {new_label_text}")
    
    return False

def test_calculation_with_comment(hwnd):
    """测试带注释的计算"""
    print("\n开始测试带注释的计算...")
    
    # 查找编辑控件
    edit_hwnd = win32gui.FindWindowEx(hwnd, 0, "Edit", None)
    if not edit_hwnd:
        print("错误：未找到编辑控件")
        return False
    
    # 输入带注释的表达式
    test_expressions = [
        "1+2 #基本加法",
        "3*4 #乘法运算", 
        "10-3 #减法测试",
        "15/5 #除法运算",
        "2+3*4 #复杂运算"
    ]
    
    for expr in test_expressions:
        print(f"\n测试表达式: {expr}")
        
        # 清空编辑框
        win32gui.SendMessage(edit_hwnd, win32con.EM_SETSEL, 0, -1)
        win32gui.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_DELETE, 0)
        win32gui.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_DELETE, 0)
        
        # 输入表达式
        for char in expr:
            win32gui.SendMessage(edit_hwnd, win32con.WM_CHAR, ord(char), 0)
        
        # 按回车键执行计算
        win32gui.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32gui.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
        
        time.sleep(0.5)  # 等待计算完成
        
        # 检查列表框中的结果显示
        listbox_hwnd = win32gui.FindWindowEx(hwnd, 0, "ListBox", None)
        if listbox_hwnd:
            item_count = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETCOUNT, 0, 0)
            print(f"列表框项数: {item_count}")
            
            if item_count > 0:
                # 获取最新的项目（索引0）
                text_length = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETTEXTLEN, 0, 0)
                if text_length > 0:
                    buffer = win32gui.PyMakeBuffer(text_length + 1)
                    actual_length = win32gui.SendMessage(listbox_hwnd, win32con.LB_GETTEXT, 0, buffer)
                    if actual_length >= 0:
                        result_text = str(buffer[:actual_length], 'utf-16-le', errors='ignore')
                        print(f"显示结果: {result_text}")
                        
                        # 检查是否包含注释信息
                        if "注释:" in result_text or "|" in result_text:
                            print("✓ 注释功能正常工作")
                        else:
                            print("⚠ 未检测到注释信息")
        
        time.sleep(1)  # 等待用户查看

def check_log_for_comment_support():
    """检查日志文件中的注释相关日志"""
    print("\n检查最新的日志文件...")
    
    log_dir = "bin\\log"
    if not os.path.exists(log_dir):
        print("日志目录不存在")
        return
    
    # 获取最新的日志文件
    log_files = [f for f in os.listdir(log_dir) if f.endswith('.log')]
    if not log_files:
        print("没有找到日志文件")
        return
    
    latest_log = max(log_files, key=lambda f: os.path.getmtime(os.path.join(log_dir, f)))
    log_path = os.path.join(log_dir, latest_log)
    
    print(f"分析日志文件: {latest_log}")
    
    # 读取日志并查找注释相关条目
    comment_logs = []
    with open(log_path, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            if "注释" in line or "ExtractExpression" in line or "EvaluateExpression" in line:
                comment_logs.append(line.strip())
    
    if comment_logs:
        print("找到相关的日志条目:")
        for log_entry in comment_logs[-10:]:  # 显示最后10条
            print(f"  {log_entry}")
    else:
        print("未找到注释相关的日志条目")

def main():
    """主函数"""
    print("=" * 60)
    print("计算器注释功能测试")
    print("=" * 60)
    
    try:
        # 查找主窗口
        hwnd = find_main_window()
        if not hwnd:
            print("请先启动快速启动程序")
            return
        
        print(f"找到主窗口: {win32gui.GetWindowText(hwnd)}")
        
        # 进入计算模式
        if not enter_calculator_mode(hwnd):
            print("进入计算模式失败，程序可能已在计算模式")
        
        # 测试带注释的计算
        test_calculation_with_comment(hwnd)
        
        # 检查日志
        check_log_for_comment_support()
        
        print("\n" + "=" * 60)
        print("测试完成！")
        print("请查看窗口中的计算历史，确认注释功能是否正常工作")
        print("期望看到格式: 表达式 = 结果 | 注释: 注释内容")
        print("=" * 60)
        
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()