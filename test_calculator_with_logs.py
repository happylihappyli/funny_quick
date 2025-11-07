#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试计算模式功能
"""

import subprocess
import time

def main():
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
    print("9. 点击'退出计算'按钮退出计算模式")
    print()
    print("测试完成后，请查看日志文件内容：E:\\github\\funny_quick\\quick_launcher.log")
    print()
    
    # 等待用户测试
    input("按回车键结束测试...")

if __name__ == "__main__":
    main()