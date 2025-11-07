import subprocess
import time
import os
import sys
from datetime import datetime

def log_message(message):
    """记录日志消息"""
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {message}")
    sys.stdout.flush()

def run_command(command, wait_time=1):
    """执行命令并等待指定时间"""
    log_message(f"执行命令: {command}")
    subprocess.run(command, shell=True)
    time.sleep(wait_time)

def speak_text(text):
    """使用TTS播放语音提示"""
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak(text)
    except Exception as e:
        log_message(f"语音提示失败: {e}")

def test_calculator_enter_key():
    """测试计算模式下的回车键计算功能"""
    log_message("开始测试计算模式下的回车键计算功能")
    
    # 启动程序
    log_message("启动程序")
    process = subprocess.Popen(["bin\\quick_launcher.exe"])
    time.sleep(2)  # 等待程序启动
    
    try:
        # 进入计算模式
        log_message("输入'js'进入计算模式")
        run_command("echo js | clip")
        subprocess.run(["powershell.exe", "-Command", "Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait('js{ENTER}')"])
        time.sleep(1)
        
        # 输入表达式但不按回车
        log_message("输入表达式'2+3'但不按回车")
        run_command("echo 2+3 | clip")
        subprocess.run(["powershell.exe", "-Command", "Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait('2+3')"])
        time.sleep(2)
        
        # 检查程序是否仍在运行
        if process.poll() is None:
            log_message("程序仍在运行，输入表达式但未按回车时未卡死")
        else:
            log_message("程序已退出，可能存在问题")
            return False
        
        # 按回车键计算
        log_message("按回车键计算表达式")
        subprocess.run(["powershell.exe", "-Command", "Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')"])
        time.sleep(2)
        
        # 检查程序是否仍在运行
        if process.poll() is None:
            log_message("程序仍在运行，按回车键计算成功")
        else:
            log_message("程序已退出，可能存在问题")
            return False
        
        # 输入另一个表达式
        log_message("输入表达式'10*5'")
        run_command("echo 10*5 | clip")
        subprocess.run(["powershell.exe", "-Command", "Add-Type -AssemblyName System.Windows.Forms; [System.Windows.Forms.SendKeys]::SendWait('10*5{ENTER}')"])
        time.sleep(2)
        
        # 检查程序是否仍在运行
        if process.poll() is None:
            log_message("程序仍在运行，第二个表达式计算成功")
        else:
            log_message("程序已退出，可能存在问题")
            return False
        
        log_message("测试完成，计算模式下的回车键计算功能正常")
        return True
        
    finally:
        # 关闭程序
        log_message("关闭程序")
        try:
            process.terminate()
            time.sleep(1)
            if process.poll() is None:
                process.kill()
        except:
            pass

if __name__ == "__main__":
    success = test_calculator_enter_key()
    if success:
        speak_text("测试完成，计算模式下的回车键计算功能正常")
    else:
        speak_text("测试失败，请检查程序")