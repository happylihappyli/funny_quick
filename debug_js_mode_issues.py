#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
JS模式问题诊断脚本
目标：诊断并解决js模式下选中项与删除项不一致、新添加项未显示在ListView第0个位置的问题
"""

import time
import win32gui
import win32con
import win32api

def find_main_window():
    """查找主窗口"""
    def enum_windows_callback(hwnd, windows):
        if win32gui.IsWindowVisible(hwnd):
            window_text = win32gui.GetWindowText(hwnd)
            if "funny_quick" in window_text.lower():
                windows.append((hwnd, window_text))
        return True
    
    windows = []
    win32gui.EnumWindows(enum_windows_callback, windows)
    return windows[0] if windows else (None, None)

def send_input_text(hwnd, text):
    """发送文本输入"""
    win32gui.SetForegroundWindow(hwnd)
    win32gui.SetFocus(hwnd)
    
    # 清空输入框
    win32gui.SendMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_CONTROL, 0)
    win32gui.SendMessage(hwnd, win32con.WM_KEYDOWN, ord('A'), 0)
    win32gui.SendMessage(hwnd, win32con.WM_KEYUP, ord('A'), 0)
    win32gui.SendMessage(hwnd, win32con.WM_KEYUP, win32con.VK_CONTROL, 0)
    
    # 输入新文本
    for char in text:
        win32gui.SendMessage(hwnd, win32con.WM_CHAR, ord(char), 0)
    
    time.sleep(0.1)

def send_enter(hwnd):
    """发送回车键"""
    win32gui.SendMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
    win32gui.SendMessage(hwnd, win32con.WM_KEYUP, win32con.VK_RETURN, 0)
    time.sleep(0.2)

def get_listview_item_text(hwnd_listview, index, subitem=0):
    """获取ListView指定项的文本"""
    try:
        # 获取项目文本
        buffer_size = 256
        buffer = win32gui.PyMakeBuffer(buffer_size)
        result = win32gui.SendMessage(hwnd_listview, win32con.LVM_GETITEMTEXTW, index, buffer)
        
        if result > 0:
            text = buffer[:result].decode('utf-16-le', errors='ignore')
            return text.rstrip('\x00')
        return ""
    except Exception as e:
        print(f"   获取ListView文本失败: {e}")
        return ""

def get_listview_count(hwnd_listview):
    """获取ListView项目数量"""
    return win32gui.SendMessage(hwnd_listview, win32con.LVM_GETITEMCOUNT, 0, 0)

def select_listview_item(hwnd_listview, index):
    """选择ListView指定项"""
    try:
        # 设置选中状态
        win32gui.SendMessage(hwnd_listview, win32con.LVM_SETSELECTIONMARK, 0, index)
        # 设置焦点
        win32gui.SendMessage(hwnd_listview, win32con.LVM_SETFOCUSITEM, 0, index)
        time.sleep(0.1)
        return True
    except Exception as e:
        print(f"   选择ListView项目失败: {e}")
        return False

def right_click_listview_item(hwnd_listview, index):
    """右键点击ListView指定项"""
    try:
        # 获取项目的边界矩形
        rect = win32gui.PyRECT()
        win32gui.SendMessage(hwnd_listview, win32con.LVM_GETITEMRECT, index, rect)
        
        # 计算点击坐标（项目中心点）
        x = rect.left + (rect.right - rect.left) // 2
        y = rect.top + (rect.bottom - rect.top) // 2
        
        # 转换为屏幕坐标
        pt = win32gui.PyPOINT(x, y)
        win32gui.ClientToScreen(hwnd_listview, pt)
        
        # 执行右键点击
        win32gui.SendMessage(hwnd_listview, win32con.WM_RBUTTONDOWN, 0, win32api.MAKELONG(pt.x, pt.y))
        time.sleep(0.1)
        win32gui.SendMessage(hwnd_listview, win32con.WM_RBUTTONUP, 0, win32api.MAKELONG(pt.x, pt.y))
        time.sleep(0.2)
        
        return True
    except Exception as e:
        print(f"   右键点击失败: {e}")
        return False

def test_js_mode_issues():
    """测试js模式问题"""
    print("🔍 开始诊断JS模式问题...")
    
    # 1. 找到主窗口
    print("1. 查找主窗口...")
    hwnd_main, window_title = find_main_window()
    if not hwnd_main:
        print("   ❌ 未找到主窗口，请确保程序正在运行")
        return False
    
    print(f"   ✅ 找到主窗口: {window_title}")
    
    # 2. 获取输入框和ListView句柄
    print("2. 获取控件句柄...")
    hwnd_edit = win32gui.GetDlgItem(hwnd_main, 1001)  # 输入框ID
    hwnd_listview = win32gui.GetDlgItem(hwnd_main, 1002)  # ListView ID
    
    if not hwnd_edit or not hwnd_listview:
        print("   ❌ 无法获取输入框或ListView句柄")
        return False
    
    print("   ✅ 控件句柄获取成功")
    
    # 3. 进入js模式
    print("3. 进入JS模式...")
    send_input_text(hwnd_edit, "js")
    send_enter(hwnd_edit)
    time.sleep(0.5)
    
    # 检查是否进入计算模式
    listview_count = get_listview_count(hwnd_listview)
    if listview_count >= 2:
        mode_text = get_listview_item_text(hwnd_listview, 0)
        if "计算模式" in mode_text:
            print("   ✅ 成功进入JS模式")
        else:
            print("   ❌ 进入JS模式失败")
            return False
    else:
        print("   ❌ 无法确认JS模式状态")
        return False
    
    # 4. 测试数据添加功能
    print("4. 测试新添加项显示位置...")
    
    # 添加第一条计算记录
    print("   a) 添加计算记录: 3 + 3 = 6")
    send_input_text(hwnd_edit, "3+3")
    send_enter(hwnd_edit)
    time.sleep(0.3)
    
    # 添加第二条计算记录
    print("   b) 添加计算记录: 2 + 2 = 4")
    send_input_text(hwnd_edit, "2+2")
    send_enter(hwnd_edit)
    time.sleep(0.3)
    
    # 添加第三条计算记录
    print("   c) 添加计算记录: 1 + 1 = 2")
    send_input_text(hwnd_edit, "1+1")
    send_enter(hwnd_edit)
    time.sleep(0.5)
    
    # 检查显示结果
    print("   📋 检查当前ListView内容:")
    count = get_listview_count(hwnd_listview)
    print(f"   ListView总项目数: {count}")
    
    expected_first = "1 + 1 = 2"  # 最新记录应该在顶部
    actual_first = get_listview_item_text(hwnd_listview, 0)
    expected_last = "3 + 3 = 6"   # 最早记录应该在底部
    actual_last = get_listview_item_text(hwnd_listview, count - 1) if count > 0 else ""
    
    print(f"   第0项（应该是最新）: '{actual_first}'")
    print(f"   第{count-1}项（应该是最早）: '{actual_last}'")
    
    if expected_first in actual_first:
        print("   ✅ 新添加项显示在ListView第0个位置正确")
        add_test_pass = True
    else:
        print("   ❌ 新添加项未显示在ListView第0个位置")
        add_test_pass = False
    
    # 5. 测试删除功能
    print("5. 测试删除功能...")
    
    if count >= 4:  # 至少有提示信息 + 3条计算记录
        # 选择第3行（应该是2+2=4）
        target_delete_index = 3
        print(f"   🎯 尝试删除第{target_delete_index}项...")
        
        # 获取要删除的内容
        content_to_delete = get_listview_item_text(hwnd_listview, target_delete_index)
        print(f"   要删除的内容: '{content_to_delete}'")
        
        # 选择该项
        if select_listview_item(hwnd_listview, target_delete_index):
            # 右键点击触发删除菜单
            if right_click_listview_item(hwnd_listview, target_delete_index):
                time.sleep(0.3)
                
                # 查找并点击删除菜单项
                try:
                    # 查找删除菜单
                    delete_menu_text = "删除此项"
                    hwnd_menu = win32gui.FindWindow(None, None)  # 获取当前活动窗口（应该是菜单）
                    
                    # 尝试通过快捷键确认删除（通常是Delete键或Enter）
                    win32gui.SendMessage(hwnd_main, win32con.WM_KEYDOWN, win32con.VK_DELETE, 0)
                    win32gui.SendMessage(hwnd_main, win32con.WM_KEYUP, win32con.VK_DELETE, 0)
                    time.sleep(0.3)
                    
                    # 检查删除结果
                    new_count = get_listview_count(hwnd_listview)
                    print(f"   删除前项目数: {count}, 删除后: {new_count}")
                    
                    # 查找被删除的内容是否还在
                    deleted_content_exists = False
                    for i in range(new_count):
                        item_text = get_listview_item_text(hwnd_listview, i)
                        if content_to_delete in item_text:
                            deleted_content_exists = True
                            print(f"   ❌ 删除失败，'{content_to_delete}'仍在ListView中（位置{i}）")
                            break
                    
                    if not deleted_content_exists:
                        print("   ✅ 删除操作执行成功")
                        delete_test_pass = True
                    else:
                        delete_test_pass = False
                        
                except Exception as e:
                    print(f"   ❌ 删除操作失败: {e}")
                    delete_test_pass = False
            else:
                print("   ❌ 右键菜单触发失败")
                delete_test_pass = False
        else:
            print("   ❌ 项目选择失败")
            delete_test_pass = False
    else:
        print("   ❌ ListView项目不足，跳过删除测试")
        delete_test_pass = False
    
    # 6. 总结测试结果
    print("\n📊 测试结果总结:")
    print(f"   添加功能测试: {'✅ 通过' if add_test_pass else '❌ 失败'}")
    print(f"   删除功能测试: {'✅ 通过' if delete_test_pass else '❌ 失败'}")
    
    overall_pass = add_test_pass and delete_test_pass
    print(f"\n🎯 整体测试: {'✅ 通过' if overall_pass else '❌ 存在问题'}")
    
    if not overall_pass:
        print("\n🔧 需要修复的问题:")
        if not add_test_pass:
            print("   - 新添加项未显示在ListView第0个位置")
            print("   - 可能原因: DisplayCalculationHistory函数或数据存储逻辑问题")
        if not delete_test_pass:
            print("   - 选中项与删除项不一致")
            print("   - 可能原因: 删除逻辑中的索引转换错误")
    
    return overall_pass

if __name__ == "__main__":
    print("🧪 JS模式问题诊断测试")
    print("=" * 50)
    
    try:
        success = test_js_mode_issues()
        if success:
            print("\n✅ 所有测试通过，JS模式工作正常！")
        else:
            print("\n❌ 发现问题，需要修复！")
    except Exception as e:
        print(f"\n💥 测试过程中发生异常: {e}")
        print("请确保程序正在运行且可以正常操作")