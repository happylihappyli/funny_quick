#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简单测试set命令功能的脚本
"""

import os
import time
import subprocess
import win32gui
import win32con
import win32api

def find_main_window():
    """查找主窗口"""
    def enum_windows_callback(hwnd, windows):
        if win32gui.IsWindowVisible(hwnd):
            window_text = win32gui.GetWindowText(hwnd)
            if "BV快启" in window_text or "funny_quick" in window_text:
                windows.append((hwnd, window_text))
        return True
    
    windows = []
    win32gui.EnumWindows(enum_windows_callback, windows)
    return windows

def find_child_window(parent_hwnd, class_name=None, window_text=None):
    """查找子窗口"""
    def enum_child_callback(hwnd, children):
        if win32gui.IsWindow(hwnd):
            if class_name:
                if win32gui.GetClassName(hwnd) == class_name:
                    children.append(hwnd)
            elif window_text:
                if window_text in win32gui.GetWindowText(hwnd):
                    children.append(hwnd)
            else:
                children.append(hwnd)
        return True
    
    children = []
    try:
        win32gui.EnumChildWindows(parent_hwnd, enum_child_callback, children)
    except:
        pass
    return children

def test_set_command():
    """测试set命令功能"""
    print("开始测试set命令功能...")
    
    exe_path = "E:\\GitHub3\\funny_quick\\bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print(f"错误：找不到可执行文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        print("启动程序...")
        subprocess.Popen([exe_path])
        time.sleep(3)  # 等待程序启动
        
        # 查找主窗口
        windows = find_main_window()
        if not windows:
            print("错误：找不到主窗口")
            return False
        
        main_hwnd = windows[0][0]
        main_title = windows[0][1]
        print(f"找到主窗口: {main_title}")
        
        # 激活窗口
        win32gui.SetForegroundWindow(main_hwnd)
        win32gui.ShowWindow(main_hwnd, win32con.SW_RESTORE)
        time.sleep(0.5)
        
        # 查找编辑框（通常是类名为"Edit"的窗口）
        edit_windows = find_child_window(main_hwnd, "Edit")
        if not edit_windows:
            print("错误：找不到编辑框")
            return False
        
        edit_hwnd = edit_windows[0]
        print("找到编辑框")
        
        # 清空并输入"set"
        win32gui.SetFocus(edit_hwnd)
        time.sleep(0.2)
        
        # 全选并删除
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_CONTROL, 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, ord('A'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYUP, ord('A'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_CONTROL, 0)
        time.sleep(0.1)
        
        # 输入"set"
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('s'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('e'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('t'), 0)
        time.sleep(0.1)
        
        # 按回车键
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
        time.sleep(1)
        
        print("已发送set命令和回车键")
        
        # 查找列表控件
        list_windows = find_child_window(main_hwnd, "SysListView32")
        if not list_windows:
            print("错误：找不到列表控件")
            return False
        
        list_hwnd = list_windows[0]
        print("找到列表控件")
        
        # 获取列表项数量
        item_count = win32api.SendMessage(list_hwnd, win32con.LVM_GETITEMCOUNT, 0, 0)
        print(f"列表项数量: {item_count}")
        
        if item_count >= 3:
            print("✓ set命令成功显示菜单项")
            
            # 获取菜单项文本
            for i in range(min(item_count, 5)):
                # 准备LVITEM结构
                item_text = [''] * 512
                lvitem = win32gui.PackLVITEM(0, i, 0, 0, 0, item_text, 512)
                
                try:
                    # 获取项文本
                    result = win32api.SendMessage(list_hwnd, win32con.LVM_GETITEMTEXTW, i, lvitem)
                    if result > 0:
                        print(f"菜单项 {i}: {item_text[0]}")
                except Exception as e:
                    print(f"获取第{i}项文本时出错: {e}")
            
            print("✓ set命令功能测试完成")
            return True
        else:
            print(f"✗ 菜单项数量不足: {item_count}")
            return False
            
    except Exception as e:
        print(f"测试过程中出错: {e}")
        import traceback
        traceback.print_exc()
        return False

def test_help_command():
    """测试help命令功能"""
    print("\n开始测试help命令功能...")
    
    exe_path = "E:\\GitHub3\\funny_quick\\bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print(f"错误：找不到可执行文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        print("启动程序...")
        subprocess.Popen([exe_path])
        time.sleep(3)  # 等待程序启动
        
        # 查找主窗口
        windows = find_main_window()
        if not windows:
            print("错误：找不到主窗口")
            return False
        
        main_hwnd = windows[0][0]
        print("找到主窗口")
        
        # 激活窗口
        win32gui.SetForegroundWindow(main_hwnd)
        win32gui.ShowWindow(main_hwnd, win32con.SW_RESTORE)
        time.sleep(0.5)
        
        # 查找编辑框
        edit_windows = find_child_window(main_hwnd, "Edit")
        if not edit_windows:
            print("错误：找不到编辑框")
            return False
        
        edit_hwnd = edit_windows[0]
        
        # 清空并输入"help"
        win32gui.SetFocus(edit_hwnd)
        time.sleep(0.2)
        
        # 全选并删除
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_CONTROL, 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, ord('A'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYUP, ord('A'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_CONTROL, 0)
        time.sleep(0.1)
        
        # 输入"help"
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('h'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('e'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('l'), 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_CHAR, ord('p'), 0)
        time.sleep(0.1)
        
        # 按回车键
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
        win32api.SendMessage(edit_hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
        time.sleep(1)
        
        print("已发送help命令和回车键")
        
        # 查找是否有对话框弹出
        def enum_dialogs_callback(hwnd, dialogs):
            if win32gui.IsWindow(hwnd) and win32gui.IsWindowVisible(hwnd):
                class_name = win32gui.GetClassName(hwnd)
                if class_name == "#32770":  # 对话框类名
                    window_text = win32gui.GetWindowText(hwnd)
                    if window_text and ("帮助" in window_text or "关于" in window_text):
                        dialogs.append((hwnd, window_text))
            return True
        
        dialogs = []
        win32gui.EnumWindows(enum_dialogs_callback, dialogs)
        
        if dialogs:
            print(f"✓ 检测到帮助对话框: {dialogs[0][1]}")
            print("✓ help命令功能测试完成")
            return True
        else:
            # 检查列表控件是否有帮助信息
            list_windows = find_child_window(main_hwnd, "SysListView32")
            if list_windows:
                list_hwnd = list_windows[0]
                item_count = win32api.SendMessage(list_hwnd, win32con.LVM_GETITEMCOUNT, 0, 0)
                print(f"列表项数量: {item_count}")
                if item_count > 0:
                    print("✓ help命令可能显示在列表中")
                    return True
            print("✗ help命令未显示帮助信息")
            return False
            
    except Exception as e:
        print(f"help命令测试过程中出错: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == "__main__":
    print("=== BV快启工具箱 - set和help命令功能测试 ===")
    
    # 测试set命令
    set_success = test_set_command()
    
    # 测试help命令
    help_success = test_help_command()
    
    print(f"\n=== 测试结果 ===")
    print(f"set命令测试: {'✓ 成功' if set_success else '✗ 失败'}")
    print(f"help命令测试: {'✓ 成功' if help_success else '✗ 失败'}")
    
    if set_success and help_success:
        print("\n🎉 所有测试通过！新功能已成功集成。")
    else:
        print("\n⚠️ 部分测试失败，请检查功能实现。")
    
    input("按回车键退出...")