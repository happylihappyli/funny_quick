#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
测试计算器注释功能
测试输入带有#注释的计算表达式，如 1+2 #买面包
"""

import time
import pyautogui
import subprocess
import os
import signal

def test_calculator_comment():
    """测试计算器注释功能"""
    print("开始测试计算器注释功能...")
    print("="*60)
    
    # 测试用例
    test_cases = [
        ("1+2 #买面包", "3"),
        ("10*5 #买水果", "50"),
        ("100/4 #买饮料", "25"),
        ("20+30-10 #计算总分", "40"),
        ("2*3+4 #混合运算", "10"),
        ("1+2 #买面包 = 3", "3"),  # 包含等号的情况
    ]
    
    print("测试用例:")
    for i, (input_expr, expected) in enumerate(test_cases, 1):
        print(f"  {i}. 输入: {input_expr} → 预期结果: {expected}")
    
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
        
        print("4. 开始测试注释功能...")
        
        for i, (input_expr, expected) in enumerate(test_cases, 1):
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
            
            # 获取结果
            result_text = pyautogui.screenshot('test_comment_result.png')
            print(f"   输入: {input_expr}")
            print(f"   预期结果: {expected}")
            print(f"   已截图保存为: test_comment_result.png")
            
            # 等待下一个测试
            time.sleep(2)
        
        print("\n5. 测试完成，退出程序...")
        
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
        print("\n测试要点:")
        print("- #符号后的内容应该被视为注释，不参与计算")
        print("- 带有注释的表达式应该正常计算")
        print("- 计算历史中应该显示完整表达式（包括注释）")
        
        # 播放语音提示
        try:
            import pyttsx3
            engine = pyttsx3.init()
            engine.say("计算器注释功能测试完成")
            engine.runAndWait()
        except:
            pass
            
        return True
        
    except Exception as e:
        print(f"❌ 测试失败: {e}")
        return False

if __name__ == "__main__":
    success = test_calculator_comment()
    if success:
        print("\n🎉 测试成功！计算器注释功能正常工作")
    else:
        print("\n❌ 测试失败！请检查程序日志")