# -*- coding: utf-8 -*-
"""
测试表达式中的等号处理
"""

import subprocess
import time
import sys

def test_equal_sign_handling():
    print("=" * 60)
    print("测试表达式中的等号处理")
    print("=" * 60)
    print("\n测试内容:")
    print("输入 '2+3+4+5=7' 并检查计算结果")
    print("\n预期结果:")
    print("- 2+3+4+5 = 14")
    print("- 程序应该只计算等号前的部分")
    print("- 历史记录应该显示完整的输入 '2+3+4+5=7 = 14'")
    print("\n测试步骤:")
    print("1. 启动 quick_launcher.exe")
    print("2. 在输入框中输入 'js' 并按回车，进入计算模式")
    print("3. 输入 '2+3+4+5=7' 并按回车")
    print("4. 检查计算结果是否为 14")
    print("5. 检查历史记录是否显示完整输入")
    print("6. 如果有问题，请查看日志文件")
    
    # 启动程序
    print("\n正在启动程序...")
    try:
        process = subprocess.Popen(["bin\\quick_launcher.exe"], cwd=".")
        time.sleep(2)  # 等待程序启动
        
        # 模拟键盘输入进入计算模式
        print("输入 'js' 进入计算模式...")
        import win32com.client
        shell = win32com.client.Dispatch("WScript.Shell")
        shell.AppActivate("Quick Launcher")
        time.sleep(1)
        shell.SendKeys("js{ENTER}")
        time.sleep(1)
        
        # 输入测试表达式
        print("输入 '2+3+4+5=7'...")
        shell.SendKeys("2+3+4+5=7{ENTER}")
        time.sleep(2)
        
        print("\n测试完成！请检查程序中的计算结果和历史记录。")
        print("预期结果应该是 14，历史记录应该显示 '2+3+4+5=7 = 14'")
        
        # 等待用户查看结果
        input("\n按回车键继续...")
        
        # 关闭程序
        process.terminate()
        print("程序已关闭。")
        
        return True
        
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        try:
            if 'process' in locals():
                process.terminate()
        except:
            pass
        return False

if __name__ == "__main__":
    success = test_equal_sign_handling()
    
    if success:
        print("\n测试指南显示成功")
    else:
        print("\n测试指南显示失败")
    
    # 播放提示音
    try:
        subprocess.run(["python", "tts_notification.py"])
    except:
        print("无法播放语音提示")