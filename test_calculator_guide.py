# -*- coding: utf-8 -*-
"""
测试计算模式功能的简单脚本
"""

import subprocess
import time
import sys

def test_calculator_mode():
    print("开始测试计算模式功能...")
    print("请按照以下步骤手动测试:")
    print("1. 启动 quick_launcher.exe")
    print("2. 在输入框中输入 'js' 并按回车")
    print("3. 检查标签是否变为 '计算:'")
    print("4. 输入 '2+3' 并按回车，检查结果是否为 5")
    print("5. 输入 '10*5' 并按回车，检查结果是否为 50")
    print("6. 输入 '100/4' 并按回车，检查结果是否为 25")
    print("7. 输入 '50-20' 并按回车，检查结果是否为 30")
    print("8. 检查计算历史是否正确显示")
    print("9. 输入 'exit' 或 'quit' 退出计算模式")
    print("10. 检查标签是否恢复为 '搜索:'")
    
    return True

if __name__ == "__main__":
    # 运行测试
    success = test_calculator_mode()
    
    if success:
        print("测试指南显示成功")
    else:
        print("测试指南显示失败")
    
    # 播放提示音
    subprocess.run(["python", "speak.py", "测试运行完毕，过来看看！"])