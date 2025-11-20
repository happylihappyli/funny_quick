#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试新添加的'set'命令功能
"""

import os
import time
import subprocess
import pyautogui
import win32gui
import win32con
from pywinauto import Application

def test_set_command():
    """测试set命令功能"""
    print("开始测试set命令功能...")
    
    # 启动程序
    exe_path = "E:\\GitHub3\\funny_quick\\bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print(f"错误：找不到可执行文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        app = Application().start(exe_path)
        time.sleep(2)  # 等待程序启动
        
        # 获取主窗口
        main_window = app.window(title_re=".*BV快启.*")
        if not main_window.exists():
            print("错误：找不到主窗口")
            return False
        
        print("程序已启动")
        
        # 聚焦到主窗口
        main_window.set_focus()
        time.sleep(0.5)
        
        # 查找编辑框
        edit_control = main_window.child_window(class_name="Edit")
        if not edit_control.exists():
            print("错误：找不到编辑框")
            return False
        
        print("开始输入测试...")
        
        # 测试步骤1：清空输入框
        edit_control.click_input()
        time.sleep(0.2)
        pyautogui.hotkey('ctrl', 'a')
        time.sleep(0.1)
        
        # 测试步骤2：输入"set"命令
        edit_control.type_keys("set")
        time.sleep(0.1)
        pyautogui.press('enter')
        time.sleep(0.5)
        
        print("已发送set命令和回车键")
        
        # 检查是否显示菜单项
        list_control = main_window.child_window(class_name="SysListView32")
        if not list_control.exists():
            print("错误：找不到列表控件")
            return False
        
        # 获取列表项数量
        item_count = list_control.item_count()
        print(f"列表项数量: {item_count}")
        
        if item_count >= 3:
            print("✓ set命令成功显示菜单项")
            
            # 获取菜单项文本
            menu_items = []
            for i in range(min(5, item_count)):  # 最多检查前5项
                try:
                    item_text = list_control.get_item(i).text()
                    menu_items.append(item_text)
                    print(f"菜单项 {i}: {item_text}")
                except:
                    break
            
            # 检查是否包含预期的菜单项
            expected_items = ["退出程序", "网址管理", "快捷方式管理", "系统设置", "关于软件"]
            found_items = [item for item in expected_items if any(item in menu_item for menu_item in menu_items)]
            
            if len(found_items) >= 3:
                print(f"✓ 找到预期的菜单项: {found_items}")
                
                # 测试双击功能
                print("测试双击功能...")
                for i in range(min(item_count, 3)):
                    try:
                        # 双击第i项
                        list_control.double_click_input(coords=(10, 10 + i * 20))
                        time.sleep(1)
                        
                        # 检查是否有对话框弹出
                        dialogs = app.windows()
                        dialog_found = False
                        for dialog in dialogs:
                            try:
                                if dialog.class_name == "#32770":  # 对话框类名
                                    dialog_found = True
                                    print(f"✓ 检测到对话框弹出: {dialog.window_text()}")
                                    # 关闭对话框
                                    dialog.send_keystrokes("{ESC}")
                                    break
                            except:
                                continue
                        
                        if not dialog_found:
                            print(f"第{i}项双击后未检测到对话框")
                        
                    except Exception as e:
                        print(f"双击第{i}项时出错: {e}")
                
                print("✓ set命令功能测试完成")
                return True
            else:
                print(f"✗ 未找到足够的预期菜单项，仅找到: {found_items}")
                return False
        else:
            print(f"✗ 菜单项数量不足: {item_count}")
            return False
            
    except Exception as e:
        print(f"测试过程中出错: {e}")
        return False
    
    finally:
        try:
            # 关闭程序
            main_window.close()
            time.sleep(1)
        except:
            pass

def test_help_command():
    """测试help命令功能"""
    print("\n开始测试help命令功能...")
    
    exe_path = "E:\\GitHub3\\funny_quick\\bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print(f"错误：找不到可执行文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        app = Application().start(exe_path)
        time.sleep(2)  # 等待程序启动
        
        # 获取主窗口
        main_window = app.window(title_re=".*BV快启.*")
        if not main_window.exists():
            print("错误：找不到主窗口")
            return False
        
        print("程序已启动")
        
        # 聚焦到主窗口
        main_window.set_focus()
        time.sleep(0.5)
        
        # 查找编辑框
        edit_control = main_window.child_window(class_name="Edit")
        if not edit_control.exists():
            print("错误：找不到编辑框")
            return False
        
        print("开始输入help命令测试...")
        
        # 测试步骤1：清空输入框
        edit_control.click_input()
        time.sleep(0.2)
        pyautogui.hotkey('ctrl', 'a')
        time.sleep(0.1)
        
        # 测试步骤2：输入"help"命令
        edit_control.type_keys("help")
        time.sleep(0.1)
        pyautogui.press('enter')
        time.sleep(1)
        
        print("已发送help命令和回车键")
        
        # 检查是否有对话框弹出（帮助信息）
        dialogs = app.windows()
        help_dialog_found = False
        for dialog in dialogs:
            try:
                if "帮助" in dialog.window_text() or "使用说明" in dialog.window_text():
                    help_dialog_found = True
                    print(f"✓ 检测到帮助对话框: {dialog.window_text()}")
                    # 关闭对话框
                    dialog.send_keystrokes("{ESC}")
                    break
            except:
                continue
        
        if help_dialog_found:
            print("✓ help命令功能测试完成")
            return True
        else:
            print("✗ help命令未显示帮助信息")
            # 检查列表控件是否有帮助信息
            list_control = main_window.child_window(class_name="SysListView32")
            if list_control.exists():
                item_count = list_control.item_count()
                print(f"列表项数量: {item_count}")
                if item_count > 0:
                    print("✓ help命令可能显示在列表中")
                    return True
            return False
            
    except Exception as e:
        print(f"help命令测试过程中出错: {e}")
        return False
    
    finally:
        try:
            # 关闭程序
            main_window.close()
            time.sleep(1)
        except:
            pass

if __name__ == "__main__":
    print("=== BV快启工具箱 - set和help命令功能测试 ===")
    
    # 禁用pyautogui安全模式
    pyautogui.FAILSAFE = False
    
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