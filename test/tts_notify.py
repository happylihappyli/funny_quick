#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
TTS语音通知脚本

此脚本用于在编译或任务完成时通过语音通知用户
"""

import sys
import os
import platform

def tts_notify(message):
    """
    使用TTS语音通知用户
    
    Args:
        message: 要播报的消息内容
    """
    try:
        # 检查操作系统
        if platform.system() == "Windows":
            # Windows系统使用SAPI语音
            import win32com.client
            speaker = win32com.client.Dispatch("SAPI.SpVoice")
            speaker.Speak(message)
            print(f"语音通知: {message}")
        else:
            # 其他系统使用espeak或其他TTS引擎
            print(f"语音通知: {message}")
            print("当前系统不支持语音播报")
    except Exception as e:
        print(f"语音通知失败: {e}")
        print(f"消息内容: {message}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        message = sys.argv[1]
        tts_notify(message)
    else:
        print("使用方法: python tts_notify.py '消息内容'")
        tts_notify("请提供要播报的消息内容")