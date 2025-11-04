#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试日志文件的中文显示
"""

import os
import sys

def test_log_file_encoding():
    """测试日志文件的编码"""
    log_dir = "log"
    if not os.path.exists(log_dir):
        print("日志目录不存在")
        return
    
    # 获取最新的日志文件
    log_files = [f for f in os.listdir(log_dir) if f.endswith('.log')]
    if not log_files:
        print("没有找到日志文件")
        return
    
    # 按文件名排序（文件名包含时间戳）
    log_files.sort()
    latest_log = os.path.join(log_dir, log_files[-1])
    
    print(f"测试日志文件: {latest_log}")
    print("=" * 50)
    
    # 尝试用UTF-8编码读取
    try:
        with open(latest_log, 'r', encoding='utf-8') as f:
            content = f.read()
            print("使用UTF-8编码读取成功:")
            print(content)
    except UnicodeDecodeError as e:
        print(f"UTF-8编码读取失败: {e}")
        
        # 尝试用GBK编码读取
        try:
            with open(latest_log, 'r', encoding='gbk') as f:
                content = f.read()
                print("使用GBK编码读取成功:")
                print(content)
        except UnicodeDecodeError as e2:
            print(f"GBK编码读取也失败: {e2}")
    
    # 检查文件开头是否有BOM
    try:
        with open(latest_log, 'rb') as f:
            first_bytes = f.read(10)
            print(f"文件开头字节: {first_bytes}")
            if first_bytes.startswith(b'\xef\xbb\xbf'):
                print("文件包含UTF-8 BOM")
            else:
                print("文件不包含UTF-8 BOM")
    except Exception as e:
        print(f"读取文件字节失败: {e}")

if __name__ == "__main__":
    test_log_file_encoding()