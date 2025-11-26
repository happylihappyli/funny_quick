#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS语音提示脚本
用于播放任务完成提示音
"""

import pyttsx3
import time

def play_completion_message():
    """播放任务完成提示音"""
    try:
        # 初始化TTS引擎
        engine = pyttsx3.init()
        
        # 设置中文语音
        voices = engine.getProperty('voices')
        # 尝试选择中文语音
        for voice in voices:
            if 'chinese' in voice.name.lower() or 'chinese' in voice.id.lower():
                engine.setProperty('voice', voice.id)
                break
        
        # 设置语速
        engine.setProperty('rate', 180)  # 语速适中
        
        # 设置音量
        engine.setProperty('volume', 0.8)  # 80%音量
        
        # 播放提示音
        completion_message = "任务运行完毕，过来看看！JS模式修复验证成功，所有功能正常工作！"
        engine.say(completion_message)
        engine.runAndWait()
        
        print("✅ TTS语音提示播放完成")
        return True
        
    except Exception as e:
        print(f"❌ TTS语音提示失败: {str(e)}")
        return False

def play_startup_message():
    """播放启动提示音"""
    try:
        engine = pyttsx3.init()
        
        # 尝试设置中文语音
        voices = engine.getProperty('voices')
        for voice in voices:
            if 'chinese' in voice.name.lower():
                engine.setProperty('voice', voice.id)
                break
        
        engine.setProperty('rate', 160)
        engine.setProperty('volume', 0.7)
        
        startup_message = "开始JS模式修复验证..."
        engine.say(startup_message)
        engine.runAndWait()
        
        return True
        
    except Exception as e:
        print(f"❌ 启动提示音失败: {str(e)}")
        return False

if __name__ == "__main__":
    print("🎵 TTS语音提示脚本")
    print("=" * 40)
    
    # 播放启动消息
    print("▶️ 播放启动提示音...")
    play_startup_message()
    
    time.sleep(1)
    
    # 播放完成消息
    print("▶️ 播放任务完成提示音...")
    play_completion_message()
    
    print("🎉 TTS语音提示任务完成")