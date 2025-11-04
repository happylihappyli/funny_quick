import win32api
import win32con
import win32gui
import time
import subprocess
import os

# 启动程序
print("启动程序...")
subprocess.Popen(["bin\\quick_launcher.exe"], cwd=os.getcwd())

# 等待程序启动
time.sleep(2)

# 测试最小化功能
print("测试最小化功能...")
# 模拟点击最小化按钮
hwnd = win32gui.FindWindow("QuickLauncherClass", "快速启动")
if hwnd:
    # 发送最小化命令
    win32api.PostMessage(hwnd, win32con.WM_SYSCOMMAND, win32con.SC_MINIMIZE, 0)
    print("已发送最小化命令")
    
    # 等待最小化完成
    time.sleep(1)
    
    # 测试Ctrl+F2快捷键显示窗口
    print("测试Ctrl+F2快捷键...")
    # 模拟按下Ctrl+F2
    win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
    win32api.keybd_event(win32con.VK_F2, 0, 0, 0)
    win32api.keybd_event(win32con.VK_F2, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
    print("已发送Ctrl+F2快捷键")
    
    # 等待窗口显示
    time.sleep(1)
    
    # 再次测试最小化
    print("再次测试最小化...")
    win32api.PostMessage(hwnd, win32con.WM_SYSCOMMAND, win32con.SC_MINIMIZE, 0)
    print("已再次发送最小化命令")
    
    time.sleep(1)
    
    # 测试Ctrl+F1快捷键（应该也能显示窗口）
    print("测试Ctrl+F1快捷键...")
    win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
    win32api.keybd_event(win32con.VK_F1, 0, 0, 0)
    win32api.keybd_event(win32con.VK_F1, 0, win32con.KEYEVENTF_KEYUP, 0)
    win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
    print("已发送Ctrl+F1快捷键")
    
    time.sleep(1)
    
    print("测试完成！")
else:
    print("未找到程序窗口")