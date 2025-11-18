#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
任务完成语音提示
"""

import sys
import os

def play_completion_sound():
    """播放任务完成语音提示"""
    try:
        # 尝试使用winsound模块播放系统声音
        import winsound
        # 播放系统声音"星号"
        winsound.MessageBeep(winsound.MB_ICONASTERISK)
        
        # 播放三次
        for i in range(2):
            winsound.MessageBeep(winsound.MB_ICONEXCLAMATION)
        
        print("任务运行完毕，过来看看！")
    except ImportError:
        # 如果winsound不可用，使用print提示
        print("任务运行完毕，过来看看！")
    except Exception as e:
        print(f"播放声音时出错: {e}")
        print("任务运行完毕，过来看看！")

if __name__ == "__main__":
    play_completion_sound()