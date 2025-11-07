import os
import subprocess
import time
import psutil
import sys

def run_test():
    """测试计算模式下的表达式计算"""
    print("开始测试计算模式...")
    
    # 启动程序
    print("启动 quick_launcher.exe...")
    exe_path = r"E:\github\funny_quick\bin\quick_launcher.exe"
    
    # 检查程序是否已经在运行
    for proc in psutil.process_iter(['pid', 'name']):
        try:
            if proc.info['name'] == 'quick_launcher.exe':
                print("程序已在运行，终止它...")
                proc.terminate()
                proc.wait(timeout=5)
                break
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    
    # 启动程序
    process = subprocess.Popen(exe_path)
    time.sleep(2)  # 等待程序启动
    
    try:
        # 使用SendKeys模拟键盘输入
        import SendKeys
        
        # 输入"js"进入计算模式
        print("输入'js'进入计算模式...")
        SendKeys.SendKeys("js")
        time.sleep(0.5)
        
        # 按回车键
        print("按回车键...")
        SendKeys.SendKeys("{ENTER}")
        time.sleep(1)
        
        # 输入"1+2"
        print("输入'1+2'...")
        SendKeys.SendKeys("1+2")
        time.sleep(0.5)
        
        # 按回车键计算
        print("按回车键计算...")
        SendKeys.SendKeys("{ENTER}")
        time.sleep(2)
        
        # 关闭程序
        process.terminate()
        process.wait(timeout=5)
        
        return True
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        
        # 如果程序还在运行，尝试终止它
        try:
            process.terminate()
            process.wait(timeout=5)
        except:
            pass
            
        return False

if __name__ == "__main__":
    success = run_test()
    if success:
        print("测试完成")
    else:
        print("测试失败")
    
    # 播放语音提示
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak("测试完成，请查看结果")
    except:
        print("无法播放语音提示")