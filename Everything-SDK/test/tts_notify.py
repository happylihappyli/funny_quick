#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文本转语音通知脚本
用于在任务完成时播放语音提示
"""

import os
import sys
import time

def tts_speak(message):
    """
    使用Windows语音合成播放消息
    
    Args:
        message (str): 要播放的语音消息
    """
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak(message)
        print(f"语音提示: {message}")
        return True
    except ImportError:
        print("警告: 未安装pywin32，无法使用语音合成")
        return False
    except Exception as e:
        print(f"语音合成错误: {e}")
        return False

def main():
    """主函数"""
    # 播放编译完成提示
    message = "任务运行完毕，过来看看！"
    
    print("=" * 50)
    print("编译任务完成通知")
    print("=" * 50)
    print(f"提示消息: {message}")
    
    # 播放语音提示
    success = tts_speak(message)
    
    if success:
        print("✓ 语音提示播放成功")
    else:
        print("⚠ 语音提示播放失败，请检查pywin32安装")
    
    print("=" * 50)

if __name__ == "__main__":
    main()