#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import os
import subprocess

def test_listview_chinese():
    """测试ListView中文显示修复"""
    print("开始测试ListView中文显示修复...")
    
    # 设置编码为UTF-8
    os.environ['PYTHONIOENCODING'] = 'utf-8'
    
    try:
        # 启动程序
        print("启动funny_quick程序...")
        
        # 启动程序进程
        process = subprocess.Popen(
            [os.path.join("bin", "funny_quick.exe")],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding='utf-8'
        )
        
        print("程序已启动，进程ID:", process.pid)
        
        # 等待程序启动
        time.sleep(2)
        
        # 检查进程是否仍在运行
        if process.poll() is None:
            print("✓ 程序运行正常")
            print("请检查ListView控件中的列标题是否显示为中文：")
            print("  - 第一列：表达式")
            print("  - 第二列：注释")
            print("\n如果列标题显示正常，中文乱码问题已修复！")
            print("\n您可以手动测试以下功能：")
            print("1. 搜索中文关键词，观察结果是否正常显示")
            print("2. 添加中文注释，查看ListView中是否正常显示")
            print("3. 切换到计算器模式，检查计算结果的中文显示")
            
            # 保持程序运行一段时间让用户检查
            print("\n程序将运行10秒钟...")
            time.sleep(10)
            
            # 终止程序
            print("正在关闭程序...")
            process.terminate()
            process.wait(timeout=5)
            print("✓ 程序已正常关闭")
            
        else:
            stdout, stderr = process.communicate()
            print("✗ 程序启动失败")
            print("STDOUT:", stdout)
            print("STDERR:", stderr)
            
    except Exception as e:
        print("✗ 测试过程中发生错误:", str(e))

if __name__ == "__main__":
    test_listview_chinese()