#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试计算模式功能并添加详细日志
"""

import subprocess
import time
import os

def create_test_log():
    """创建测试日志文件"""
    log_file = "E:\\github\\funny_quick\\quick_launcher.log"
    if os.path.exists(log_file):
        os.remove(log_file)
    return log_file

def run_test():
    """运行测试"""
    log_file = create_test_log()
    
    print("测试计算模式功能...")
    print("请按照以下步骤操作：")
    print("1. 程序已经启动")
    print("2. 在输入框中输入 'js' 并按回车，进入计算模式")
    print("3. 输入 '1+1' 并按回车")
    print("4. 查看结果是否为 '2'")
    print("5. 输入 '2+3' 并按回车")
    print("6. 查看结果是否为 '5'")
    print("7. 输入 '5*6' 并按回车")
    print("8. 查看结果是否为 '30'")
    print()
    print("测试完成后，请查看日志文件内容：", log_file)
    print()
    
    # 启动程序
    process = subprocess.Popen(["E:\\github\\funny_quick\\bin\\quick_launcher.exe"])
    
    # 等待用户测试
    input("按回车键结束测试...")
    
    # 关闭程序
    try:
        process.terminate()
        process.wait(timeout=5)
    except:
        try:
            process.kill()
        except:
            pass
    
    # 显示日志文件内容
    if os.path.exists(log_file):
        print("\n日志文件内容：")
        with open(log_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            # 只显示最后50行
            for line in lines[-50:]:
                print(line.strip())
    else:
        print("未找到日志文件")

if __name__ == "__main__":
    run_test()