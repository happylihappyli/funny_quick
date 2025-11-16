#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
任务完成语音提示
"""

import sys
import os
import time

def save_user_input():
    """保存用户当前任务的输入记录"""
    user_input = """
用户输入记录 - 任务完成时间: """ + time.strftime('%Y-%m-%d %H:%M:%S') + """

任务内容: 为funny_quick项目添加"退出计算模式"菜单功能

具体修改:
1. 在resource.h中新增了ID_SETTINGS_EXIT_CALCULATOR(2017)常量定义
2. 在CreateSettingsMenu函数中添加了"退出计算模式"菜单项，位于分隔线和"退出程序"之间
3. 在HandleSettingsMenuCommand函数中添加了对ID_SETTINGS_EXIT_CALCULATOR的处理逻辑
4. 修改了resource.h文件的编码格式为UTF-8+BOM解决了编译错误
5. 成功编译项目并验证了功能正常工作

编译结果: 成功
功能状态: 正常

备注:
- 解决了ID_SETTINGS_EXIT未声明的编译错误
- 修复了编码问题导致的编译失败
- 菜单功能已集成到设置菜单中
- 程序运行时菜单显示正常

"""
    
    # 保存到用户输入记录文件
    filename = "user_input_completion.txt"
    try:
        with open(filename, 'w', encoding='utf-8') as f:
            f.write(user_input)
        print(f"用户输入记录已保存到: {filename}")
    except Exception as e:
        print(f"保存用户输入记录失败: {e}")

def play_completion_sound():
    """播放任务完成语音提示"""
    try:
        # 尝试使用TTS模块
        import pyttsx3
        engine = pyttsx3.init()
        
        # 设置中文语音
        voices = engine.getProperty('voices')
        for voice in voices:
            if 'chinese' in voice.name.lower() or 'chinese' in voice.id.lower():
                engine.setProperty('voice', voice.id)
                break
        
        # 播放中文语音
        engine.say("任务运行完毕，过来看看！菜单功能已添加完成，编译成功！")
        engine.runAndWait()
        print("TTS语音播放完成")
        
    except ImportError:
        # 如果pyttsx3不可用，使用winsound模块播放系统声音
        try:
            import winsound
            # 播放系统声音"星号"
            winsound.MessageBeep(winsound.MB_ICONASTERISK)
            
            # 播放几次
            for i in range(2):
                winsound.MessageBeep(winsound.MB_ICONEXCLAMATION)
            
            print("任务运行完毕，过来看看！")
        except ImportError:
            # 如果winsound也不可用，使用print提示
            print("任务运行完毕，过来看看！")
        except Exception as e:
            print(f"播放声音时出错: {e}")
            print("任务运行完毕，过来看看！")
    except Exception as e:
        print(f"播放TTS时出错: {e}")
        print("任务运行完毕，过来看看！")

if __name__ == "__main__":
    # 保存用户输入记录
    save_user_input()
    
    # 播放完成通知
    play_completion_sound()