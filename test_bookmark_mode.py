# -*- coding: utf-8 -*-
"""
测试网址模式和删除功能
"""
import subprocess
import time
import sys
import os

def run_command(cmd):
    """执行命令并返回结果"""
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, encoding='utf-8')
        return result.stdout, result.stderr, result.returncode
    except Exception as e:
        print(f"执行命令出错: {e}")
        return "", str(e), 1

def test_program():
    """测试程序功能"""
    print("开始测试网址模式和删除功能...")
    
    # 检查程序是否存在
    if not os.path.exists("bin\\funny_quick.exe"):
        print("错误: 找不到 bin\\funny_quick.exe")
        return False
    
    print("1. 程序已启动，请手动测试以下功能:")
    print("   - 在输入框中输入 'wz' 并按回车，检查是否切换到网址模式")
    print("   - 在网址模式下，选择一个网址项，按Delete键，检查是否弹出删除确认对话框")
    print("   - 在网址模式下，右键点击列表项，检查是否出现删除菜单")
    print("   - 测试删除功能是否正常工作")
    
    # 启动程序
    try:
        subprocess.Popen(["bin\\funny_quick.exe"])
        print("程序已启动，请进行手动测试")
        return True
    except Exception as e:
        print(f"启动程序失败: {e}")
        return False

if __name__ == "__main__":
    success = test_program()
    if success:
        print("\n测试脚本执行完成")
        print("请手动验证功能是否正常工作")
    else:
        print("\n测试失败")
    
    # 使用TTS播放提示音
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak("测试程序已启动，请手动验证功能")
    except:
        print("无法播放语音提示")
    
    input("\n按回车键退出...")