import time
import subprocess
import win32gui
import win32con
import win32api
import win32com.client

def find_window(title_pattern):
    """查找窗口"""
    def callback(hwnd, hwnd_list):
        if win32gui.IsWindowVisible(hwnd) and win32gui.GetWindowText(hwnd):
            hwnd_list.append(hwnd)
        return True
    
    hwnd_list = []
    win32gui.EnumWindows(callback, hwnd_list)
    
    for hwnd in hwnd_list:
        title = win32gui.GetWindowText(hwnd)
        if title_pattern.lower() in title.lower():
            return hwnd
    
    return None

def test_expression():
    """测试表达式计算"""
    print("启动程序...")
    subprocess.Popen([r"E:\github\funny_quick\bin\quick_launcher.exe"])
    time.sleep(2)  # 等待程序启动
    
    # 查找窗口
    hwnd = find_window("quick")
    if not hwnd:
        print("未找到程序窗口")
        return False
    
    print(f"找到窗口句柄: {hwnd}")
    
    # 激活窗口
    win32gui.SetForegroundWindow(hwnd)
    time.sleep(0.5)
    
    # 测试表达式1: 2+3+4+5
    print("测试表达式: 2+3+4+5")
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('2'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('+'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('3'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('+'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('4'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('+'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('5'), 0)
    time.sleep(0.5)
    
    # 按回车键
    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
    time.sleep(1)
    
    # 测试表达式2: 2+3+4+5=7
    print("测试表达式: 2+3+4+5=7")
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('2'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('+'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('3'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('+'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('4'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('+'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('5'), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('='), 0)
    time.sleep(0.1)
    win32api.PostMessage(hwnd, win32con.WM_CHAR, ord('7'), 0)
    time.sleep(0.5)
    
    # 按回车键
    win32api.PostMessage(hwnd, win32con.WM_KEYDOWN, win32con.VK_RETURN, 0)
    time.sleep(1)
    
    # 关闭窗口
    win32api.PostMessage(hwnd, win32con.WM_CLOSE, 0, 0)
    
    print("测试完成，请检查日志文件查看计算结果")
    return True

if __name__ == "__main__":
    try:
        test_expression()
    except Exception as e:
        print(f"测试过程中发生错误: {str(e)}")
    
    # 播放语音提示
    try:
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak("测试完成，请查看结果")
    except:
        pass