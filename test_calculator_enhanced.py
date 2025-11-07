# -*- coding: utf-8 -*-
import os
import subprocess
import time
import sys

# 测试计算模式功能
def test_calculator_mode():
    print("=" * 60)
    print("测试计算模式功能 - 增强日志版本")
    print("=" * 60)
    
    # 启动程序
    print("1. 启动quick_launcher程序...")
    process = subprocess.Popen(
        ["bin\\quick_launcher.exe"],
        cwd="E:\\github\\funny_quick",
        creationflags=subprocess.CREATE_NEW_CONSOLE
    )
    
    # 等待程序启动
    time.sleep(2)
    
    print("2. 程序已启动，现在请按照以下步骤测试：")
    print("   a) 在输入框中输入 'js' 并按回车键进入计算模式")
    print("   b) 在输入框中输入 '1' 并按回车键")
    print("   c) 观察程序是否崩溃或正常显示计算结果")
    print("   d) 如果程序崩溃，请查看quick_launcher.log文件中的详细日志")
    
    # 等待用户测试
    input("   按回车键继续...")
    
    # 检查日志文件
    print("\n3. 检查日志文件...")
    log_file = "E:\\github\\funny_quick\\quick_launcher.log"
    
    if os.path.exists(log_file):
        print("   日志文件存在，读取最后50行日志：")
        print("   " + "-" * 50)
        
        # 读取日志文件的最后50行
        with open(log_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            for line in lines[-50:]:
                print(f"   {line.strip()}")
        
        print("   " + "-" * 50)
        print("   日志文件位置：", log_file)
    else:
        print("   警告：日志文件不存在！")
    
    # 询问测试结果
    print("\n4. 请回答以下问题：")
    result = input("   程序是否正常显示计算结果？(y/n): ")
    
    if result.lower() == 'y':
        print("\n✅ 测试通过！计算模式功能正常工作。")
    else:
        print("\n❌ 测试失败！计算模式仍然存在问题。")
        print("   请检查日志文件中的错误信息，特别是以'EvaluateExpression:'开头的日志。")
    
    print("\n5. 结束测试...")
    try:
        process.terminate()
        print("   程序已终止。")
    except:
        print("   无法终止程序，可能已经关闭。")
    
    print("\n测试完成！")

if __name__ == "__main__":
    test_calculator_mode()