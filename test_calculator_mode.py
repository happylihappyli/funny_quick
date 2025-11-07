# -*- coding: utf-8 -*-
"""
测试计算模式功能的脚本
"""

import subprocess
import time
import sys
import os

def test_calculator_mode():
    print("开始测试计算模式功能...")
    print("=" * 50)
    
    # 清空日志文件
    log_file = "quick_launcher.log"
    if os.path.exists(log_file):
        os.remove(log_file)
        print("已清空日志文件")
    
    # 启动程序
    print("启动程序...")
    process = subprocess.Popen(["bin\\quick_launcher.exe"], 
                             cwd=".",
                             creationflags=subprocess.CREATE_NEW_CONSOLE)
    
    # 等待程序启动
    time.sleep(2)
    
    print("\n请按照以下步骤手动测试：")
    print("1. 在程序窗口中输入 'js' 并按回车键进入计算模式")
    print("2. 输入单个数字 (如 123) 并按回车键")
    print("3. 观察程序是否闪退")
    print("4. 输入简单表达式 (如 2+2) 并按回车键")
    print("5. 观察计算结果是否正确")
    print("6. 完成测试后按任意键继续...")
    
    # 等待用户输入
    input()
    
    # 检查程序是否仍在运行
    if process.poll() is None:
        print("✓ 程序仍在运行，测试通过")
        success = True
    else:
        print("✗ 程序已退出，测试失败")
        success = False
        
    # 关闭程序
    try:
        if process.poll() is None:
            process.terminate()
            print("已关闭程序")
    except:
        pass
        
    # 检查日志
    if os.path.exists(log_file):
        with open(log_file, "r", encoding="utf-8") as f:
            log_content = f.read()
            
        print("\n检查日志文件...")
        if "DisplayCalculationHistory: 显示计算历史" in log_content:
            print("✓ 找到显示计算历史的日志")
        if "EvaluateExpression: 成功解析为单个数字" in log_content:
            print("✓ 找到成功解析单个数字的日志")
        if "EvaluateExpression: 表达式计算异常" in log_content:
            print("✗ 找到表达式计算异常的日志")
            
        # 显示最后几行日志
        print("\n最后几行日志：")
        lines = log_content.strip().split('\n')
        for line in lines[-10:]:
            print(line)
            
    return success

if __name__ == "__main__":
    # 运行测试
    success = test_calculator_mode()
    
    if success:
        print("测试脚本执行成功")
    else:
        print("测试脚本执行失败")
    
    # 播放提示音
    subprocess.run(["python", "speak.py", "测试运行完毕，过来看看！"])