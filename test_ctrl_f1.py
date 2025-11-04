# 测试Ctrl+F1快捷键功能
import time
import ctypes
import win32con
import win32api
import win32gui

# 发送Ctrl+F1快捷键
def send_ctrl_f1():
    # 模拟按下Ctrl键
    win32api.keybd_event(win32con.VK_CONTROL, 0, 0, 0)
    time.sleep(0.1)
    # 模拟按下F1键
    win32api.keybd_event(win32con.VK_F1, 0, 0, 0)
    time.sleep(0.1)
    # 释放F1键
    win32api.keybd_event(win32con.VK_F1, 0, win32con.KEYEVENTF_KEYUP, 0)
    time.sleep(0.1)
    # 释放Ctrl键
    win32api.keybd_event(win32con.VK_CONTROL, 0, win32con.KEYEVENTF_KEYUP, 0)
    print("已发送Ctrl+F1快捷键")

if __name__ == "__main__":
    print("等待2秒后发送Ctrl+F1快捷键...")
    time.sleep(2)
    send_ctrl_f1()
    print("测试完成")