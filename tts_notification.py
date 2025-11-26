#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Funny Quick TTS通知模块
提供文本转语音功能，用于程序完成时的语音提示
"""

import platform
import sys
import subprocess
import time
from datetime import datetime

def play_tts_message(message="任务运行完毕，过来看看！"):
    """
    播放TTS语音消息
    
    Args:
        message (str): 要播放的消息内容，默认为"任务运行完毕，过来看看！"
    """
    try:
        if platform.system() == "Windows":
            # Windows系统使用PowerShell命令
            # 使用Microsoft Speech Platform或Windows内置的SAPI
            powershell_cmd = f'''
            Add-Type -AssemblyName System.speech;
            $speak = New-Object System.Speech.Synthesis.SpeechSynthesizer;
            $speak.Rate = 0;  # 语速：-10到10，0为正常语速
            $speak.Volume = 100;  # 音量：0到100
            $speak.Speak("{message}");
            '''
            
            # 使用subprocess调用PowerShell
            result = subprocess.run(
                ["powershell", "-NoProfile", "-Command", powershell_cmd],
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="ignore",
                timeout=10
            )
            
            if result.returncode == 0:
                print(f"TTS播放成功: {message}")
                return True
            else:
                print(f"TTS播放失败，尝试备用方案: {result.stderr}")
                # 尝试备用方案
                return play_tts_fallback(message)
                
        else:
            # Linux/macOS系统备用方案
            return play_tts_fallback(message)
            
    except subprocess.TimeoutExpired:
        print("TTS播放超时")
        return play_tts_fallback(message)
    except Exception as e:
        print(f"TTS播放异常: {e}")
        return play_tts_fallback(message)

def play_tts_fallback(message="任务运行完毕，过来看看！"):
    """
    备用TTS方案
    """
    try:
        if platform.system() == "Windows":
            # Windows备用方案：使用简单的win32com
            try:
                import win32com.client
                speaker = win32com.client.Dispatch("SAPI.SpVoice")
                speaker.Rate = 0  # 语速
                speaker.Volume = 100  # 音量
                speaker.Speak(message)
                print(f"备用TTS播放成功: {message}")
                return True
            except ImportError:
                print("win32com未安装，使用简化的提示音")
                import winsound
                winsound.Beep(1000, 500)  # 1000Hz，持续500ms
                print(f"简化TTS: {message}")
                return True
        else:
            # Linux/macOS备用方案
            print(f"备用TTS: {message}")
            return False
            
    except Exception as e:
        print(f"备用TTS也失败: {e}")
        return False

def announce_compilation_time(start_time, end_time):
    """
    播报编译时间信息
    
    Args:
        start_time (datetime): 开始时间
        end_time (datetime): 结束时间
    """
    duration = end_time - start_time
    duration_seconds = int(duration.total_seconds())
    
    message = f"编译已完成，耗时{duration_seconds}秒，任务运行完毕，过来看看！"
    play_tts_message(message)

def announce_task_completion(task_name="任务"):
    """
    播报任务完成信息
    
    Args:
        task_name (str): 任务名称
    """
    message = f"{task_name}已完成，任务运行完毕，过来看看！"
    play_tts_message(message)

def test_tts():
    """测试TTS功能"""
    print("开始测试TTS功能...")
    
    # 测试基本消息
    success1 = play_tts_message("这是TTS功能测试")
    
    # 等待一下
    time.sleep(2)
    
    # 测试编译时间播报
    start = datetime.now()
    time.sleep(1)
    end = datetime.now()
    announce_compilation_time(start, end)
    
    return success1

if __name__ == "__main__":
    # 如果直接运行此脚本，进行TTS测试
    if len(sys.argv) > 1:
        message = sys.argv[1]
        play_tts_message(message)
    else:
        test_tts()