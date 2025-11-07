# -*- coding: utf-8 -*-
"""
测试计算模式下的复杂表达式计算
"""

import subprocess
import time
import sys

def test_complex_expression():
    print("=" * 60)
    print("测试计算模式下的复杂表达式计算")
    print("=" * 60)
    print("\n测试内容:")
    print("输入 '2+3+4+5=7' 并检查计算结果")
    print("\n预期结果:")
    print("- 2+3+4+5 = 14")
    print("- 如果输入包含等号，应该只计算等号前的部分")
    print("\n测试步骤:")
    print("1. 启动 quick_launcher.exe")
    print("2. 在输入框中输入 'js' 并按回车，进入计算模式")
    print("3. 输入 '2+3+4+5=7' 并按回车")
    print("4. 检查计算结果是否正确")
    print("5. 如果有问题，请查看日志文件")
    
    return True

if __name__ == "__main__":
    # 运行测试
    success = test_complex_expression()
    
    if success:
        print("\n测试指南显示成功")
    else:
        print("\n测试指南显示失败")
    
    # 播放提示音
    subprocess.run(["python", "tts_notification.py"])