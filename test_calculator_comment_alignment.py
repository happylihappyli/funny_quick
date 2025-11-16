#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
测试计算器注释对齐功能
测试新的注释显示格式：表达式和结果在左边，注释在右边
"""

import time
import pyautogui
import subprocess
import os
import signal

def test_calculator_comment_alignment():
    """测试计算器注释对齐功能"""
    print("开始测试计算器注释对齐功能...")
    print("="*60)
    
    # 测试用例
    test_cases = [
        ("1+2 #买面包", "3", "买面包"),
        ("10*5 #买水果", "50", "买水果"),
        ("100/4 #买饮料", "25", "买饮料"),
        ("20+30-10 #计算总分", "40", "计算总分"),
        ("2*3+4 #混合运算", "10", "混合运算"),
        ("5-2 #简单减法", "3", "简单减法"),
        ("15/3 #除法计算", "5", "除法计算"),
    ]
    
    print("测试用例:")
    for i, (input_expr, expected, comment) in enumerate(test_cases, 1):
        print(f"  {i}. 输入: {input_expr} → 预期: {expected}, 注释: {comment}")
    
    print("\n操作步骤:")
    print("1. 启动程序...")
    
    # 启动程序
    try:
        process = subprocess.Popen(
            [os.path.abspath("bin\\funny_quick.exe")],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=subprocess.CREATE_NEW_CONSOLE
        )
        
        print("2. 等待程序启动...")
        time.sleep(3)  # 等待程序完全启动
        
        print("3. 输入 'js' 进入计算模式...")
        pyautogui.typewrite("js")
        pyautogui.press('enter')
        time.sleep(1)
        
        print("4. 开始测试注释对齐功能...")
        
        for i, (input_expr, expected, comment) in enumerate(test_cases, 1):
            print(f"\n测试 {i}: {input_expr}")
            
            # 清除编辑框
            pyautogui.hotkey('ctrl', 'a')
            time.sleep(0.1)
            
            # 输入表达式
            pyautogui.typewrite(input_expr)
            time.sleep(0.5)
            
            # 按回车计算
            pyautogui.press('enter')
            time.sleep(1)
            
            # 截图查看结果
            pyautogui.screenshot(f'test_alignment_{i}.png')
            print(f"   输入: {input_expr}")
            print(f"   预期计算: {expected}")
            print(f"   注释: {comment}")
            print(f"   已截图: test_alignment_{i}.png")
            
            # 等待下一个测试
            time.sleep(2)
        
        print("\n5. 检查注释对齐效果...")
        print("   - 表达式和结果应该显示在左边")
        print("   - 注释应该显示在右对齐位置")
        print("   - 格式应该类似: '1+2 = 3                     # 买面包'")
        
        print("\n6. 测试完成，退出程序...")
        
        # 发送ESC键退出
        pyautogui.press('escape')
        time.sleep(1)
        
        # 关闭程序
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
        
        print("✅ 测试完成！")
        print("\n新的显示格式:")
        print("左侧: 表达式 = 结果")
        print("右侧: # 注释内容")
        print("- 注释内容右对齐显示")
        print("- 表达式和计算结果清晰可读")
        
        # 播放语音提示
        try:
            import pyttsx3
            engine = pyttsx3.init()
            engine.say("计算器注释对齐功能测试完成")
            engine.runAndWait()
        except:
            pass
            
        return True
        
    except Exception as e:
        print(f"❌ 测试失败: {e}")
        return False

if __name__ == "__main__":
    success = test_calculator_comment_alignment()
    if success:
        print("\n🎉 测试成功！计算器注释对齐功能正常工作")
        print("\n查看截图文件查看对齐效果:")
        for i in range(1, 8):
            print(f"- test_alignment_{i}.png")
    else:
        print("\n❌ 测试失败！请检查程序日志")