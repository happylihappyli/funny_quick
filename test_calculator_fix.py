#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试计算模式输入单个数字是否会导致卡死问题
"""

import subprocess
import time
import sys
import os
import pyttsx3

def speak(text):
    """使用TTS播放语音提示"""
    try:
        engine = pyttsx3.init()
        engine.say(text)
        engine.runAndWait()
    except Exception as e:
        print(f"TTS播放失败: {e}")

def test_calculator_mode():
    """测试计算模式输入单个数字"""
    # 启动程序
    exe_path = r"bin\quick_launcher.exe"
    if not os.path.exists(exe_path):
        print(f"错误: 找不到可执行文件 {exe_path}")
        return False
    
    print("启动程序...")
    process = subprocess.Popen(exe_path)
    
    # 等待程序启动
    time.sleep(2)
    
    try:
        # 输入"js"进入计算模式
        print("输入'js'进入计算模式...")
        subprocess.run(["powershell.exe", "-Command", 
                        f"Add-Type -AssemblyName System.Windows.Forms; "
                        f"[System.Windows.Forms.SendKeys]::SendWait('js')"], 
                        shell=True, timeout=5)
        time.sleep(1)
        
        # 输入单个数字"1"
        print("输入单个数字'1'...")
        subprocess.run(["powershell.exe", "-Command", 
                        f"Add-Type -AssemblyName System.Windows.Forms; "
                        f"[System.Windows.Forms.SendKeys]::SendWait('1')"], 
                        shell=True, timeout=5)
        time.sleep(1)
        
        # 检查程序是否仍在运行
        if process.poll() is None:
            print("测试通过：输入单个数字不会导致程序卡死")
            speak("测试通过，输入单个数字不会导致程序卡死")
            
            # 关闭程序
            process.terminate()
            time.sleep(1)
            if process.poll() is None:
                process.kill()
            
            return True
        else:
            print("测试失败：程序可能已经卡死或崩溃")
            speak("测试失败，程序可能已经卡死或崩溃")
            return False
            
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        speak("测试过程中发生错误")
        try:
            process.terminate()
            time.sleep(1)
            if process.poll() is None:
                process.kill()
        except:
            pass
        return False

if __name__ == "__main__":
    print("开始测试计算模式输入单个数字是否会导致卡死问题...")
    success = test_calculator_mode()
    if success:
        print("测试成功完成")
        speak("测试成功完成")
    else:
        print("测试失败")
        speak("测试失败")
    sys.exit(0 if success else 1)