#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import pyttsx3

def announce_completion():
    """语音提示任务完成"""
    try:
        engine = pyttsx3.init()
        # 设置中文语音
        voices = engine.getProperty('voices')
        if voices:
            # 尝试使用中文语音
            for voice in voices:
                if 'chinese' in voice.name.lower() or '中文' in voice.name:
                    engine.setProperty('voice', voice.id)
                    break
        
        engine.say("任务完成！中英文版本都已成功编译，请查看bin目录中的可执行文件")
        engine.runAndWait()
        print("语音提示已播放")
    except Exception as e:
        print(f"语音提示播放失败: {e}")

if __name__ == "__main__":
    announce_completion()