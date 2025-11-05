#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
播放语音提示
"""

import win32com.client

def speak(text):
    """使用TTS播放语音提示"""
    speaker = win32com.client.Dispatch("SAPI.SpVoice")
    speaker.Speak(text)

if __name__ == "__main__":
    speak("任务运行完毕，过来看看！")