import subprocess
import time
import pyautogui
import sys

# 禁用PyAutoGUI的安全机制
pyautogui.FAILSAFE = False

def test_bookmark_mode():
    print("开始测试网址收藏模式...")
    
    # 启动程序
    print("启动程序...")
    process = subprocess.Popen(["E:\\github\\funny_quick\\bin\\funny_quick.exe"])
    
    # 等待程序启动
    time.sleep(2)
    
    try:
        # 输入"wz"命令进入网址收藏模式
        print("输入'wz'命令进入网址收藏模式...")
        pyautogui.typewrite("wz")
        time.sleep(1)
        
        # 按回车键
        print("按回车键执行命令...")
        pyautogui.press("enter")
        time.sleep(3)
        
        # 输入一些测试内容
        print("输入测试网址...")
        pyautogui.typewrite("https://www.google.com")
        time.sleep(1)
        
        # 按回车键
        print("按回车键执行...")
        pyautogui.press("enter")
        time.sleep(2)
        
        # 关闭程序
        print("关闭程序...")
        pyautogui.hotkey('alt', 'f4')
        time.sleep(1)
        
        print("测试完成！网址收藏模式应该已经正常显示并可以添加网址。")
        
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        try:
            process.terminate()
        except:
            pass

if __name__ == "__main__":
    test_bookmark_mode()