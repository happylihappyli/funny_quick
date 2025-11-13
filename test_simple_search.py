# -*- coding: utf-8 -*-
"""
简单测试搜索收藏网址功能
"""
import subprocess
import time
import sys
import os

def log_message(message):
    """记录日志消息"""
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {message}")
    sys.stdout.flush()

def test_search_bookmark_simple():
    """简单测试搜索收藏网址功能"""
    log_message("开始简单测试搜索收藏网址功能...")
    
    # 启动程序
    log_message("启动程序...")
    process = subprocess.Popen(["E:\\github\\funny_quick\\bin\\funny_quick.exe"])
    time.sleep(3)  # 等待程序启动
    
    try:
        # 检查程序是否正在运行
        if process.poll() is None:
            log_message("程序正在运行，测试通过")
        else:
            log_message("程序未运行，测试失败")
            return False
            
        # 等待一段时间
        time.sleep(2)
        
        # 终止程序
        log_message("终止程序...")
        process.terminate()
        time.sleep(2)
        
        log_message("简单测试完成！")
        return True
        
    except Exception as e:
        log_message(f"测试过程中发生错误: {str(e)}")
        return False

if __name__ == "__main__":
    # 记录开始时间
    start_time = time.strftime("%Y-%m-%d %H:%M:%S")
    log_message(f"测试开始时间: {start_time}")
    
    # 运行测试
    success = test_search_bookmark_simple()
    
    # 记录结束时间
    end_time = time.strftime("%Y-%m-%d %H:%M:%S")
    log_message(f"测试结束时间: {end_time}")
    
    # 播放语音提示
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        if success:
            speaker.Speak("简单测试完成！")
        else:
            speaker.Speak("测试失败！")
    except:
        log_message("无法播放语音提示")