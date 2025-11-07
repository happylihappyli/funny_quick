# -*- coding: utf-8 -*-
"""
测试UI布局修改的脚本
"""

import subprocess
import time
import sys

def test_ui_layout():
    print("=" * 60)
    print("测试UI布局修改 - 非计算模式显示设置按钮")
    print("=" * 60)
    print("\n修改内容:")
    print("1. 添加了设置按钮，在非计算模式下显示")
    print("2. 在计算模式下隐藏设置按钮，显示退出计算按钮")
    print("3. 设置按钮点击后会显示提示信息")
    print("\n测试步骤:")
    print("1. 启动 quick_launcher.exe")
    print("2. 检查右上角是否显示'设置'按钮")
    print("3. 点击'设置'按钮，检查是否弹出提示信息")
    print("4. 在输入框中输入 'js' 并按回车，进入计算模式")
    print("5. 检查右上角是否显示'退出计算'按钮，'设置'按钮是否隐藏")
    print("6. 点击'退出计算'按钮，检查是否退出计算模式")
    print("7. 检查右上角是否重新显示'设置'按钮")
    print("\n预期结果:")
    print("- 非计算模式下应显示'设置'按钮")
    print("- 计算模式下应显示'退出计算'按钮，隐藏'设置'按钮")
    print("- 点击'设置'按钮应弹出提示信息")
    print("- 退出计算模式后应重新显示'设置'按钮")
    
    return True

if __name__ == "__main__":
    # 运行测试
    success = test_ui_layout()
    
    if success:
        print("\n测试指南显示成功")
    else:
        print("\n测试指南显示失败")
    
    # 播放提示音
    subprocess.run(["python", "tts_notification.py"])