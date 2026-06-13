#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TTS语音通知脚本
"""

import pyttsx3
import sys
import time

def speak_message(message):
    """使用TTS播放消息"""
    try:
        engine = pyttsx3.init()
        
        # 设置语音属性
        engine.setProperty('rate', 150)  # 语音速度
        engine.setProperty('volume', 0.8)  # 音量 (0.0 to 1.0)
        
        # 播放消息
        engine.say(message)
        engine.runAndWait()
        
        print(f"✅ TTS播放完成: {message}")
        return True
        
    except Exception as e:
        print(f"❌ TTS播放失败: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) > 1:
        message = " ".join(sys.argv[1:])
    else:
        message = "任务运行完毕，过来看看！"
    
    speak_message(message)