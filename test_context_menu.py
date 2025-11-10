import win32gui
import win32api
import win32con
import time
import os
import sys

def find_window():
    """查找主窗口"""
    hwnd = win32gui.FindWindow(None, "快速启动")
    return hwnd

def test_context_menu():
    """测试右键菜单功能"""
    print("开始测试右键菜单功能...")
    
    # 启动程序
    os.startfile("E:\\github\\funny_quick\\bin\\quick_launcher.exe")
    time.sleep(2)  # 等待程序启动
    
    # 查找窗口
    hwnd = find_window()
    if not hwnd:
        print("未找到窗口，请确保程序已启动")
        return
    
    print(f"找到窗口: {hwnd}")
    
    # 获取窗口位置
    rect = win32gui.GetWindowRect(hwnd)
    print(f"窗口位置: {rect}")
    
    # 尝试进入计算模式
    print("尝试进入计算模式...")
    # 发送Ctrl+F1热键
    win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
    win32api.keybd_event(win32con.VK_F1, 0, 0, 0)
    win32api.keybd_event(win32con.VK_F1, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(1)
    
    # 输入一些测试表达式
    test_expressions = ["1+1=2", "2*3=6", "10/2=5"]
    for expr in test_expressions:
        win32gui.SetForegroundWindow(hwnd)
        time.sleep(0.5)
        win32api.keybd_event(win32con.VK_END, 0, 0, 0)  # 移到末尾
        win32api.keybd_event(win32con.VK_END, 0, win32con.KEYEVENTF_KEYUP, 0)
        time.sleep(0.2)
        
        # 输入表达式
        for char in expr:
            win32api.keybd_event(ord(char.upper()), 0, 0, 0)
            win32api.keybd_event(ord(char.upper()), 0, win32con.KEYEVENTF_KEYUP, 0)
            time.sleep(0.05)
        
        # 按回车
        win32api.keybd_event(win32con.VK_RETURN, 0, 0, 0)
        win32api.keybd_event(win32con.VK_RETURN, 0, win32con.KEYEVENTF_KEYUP, 0)
        time.sleep(0.5)
    
    print("已输入测试表达式，请在程序中:")
    print("1. 在历史记录列表上右键点击")
    print("2. 选择'删除此项'测试删除功能")
    print("3. 再次右键点击，选择'清空历史'测试清空功能")
    print("4. 检查功能是否正常工作")
    
    # 播放提示音
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak("右键菜单测试完成，请检查功能是否正常")
    except:
        print("右键菜单测试完成，请检查功能是否正常")

if __name__ == "__main__":
    test_context_menu()