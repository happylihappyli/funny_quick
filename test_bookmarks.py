#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
测试网址收藏功能的脚本
用于测试网址收藏中的中文显示是否正常
"""

import os
import sys

def create_test_bookmarks():
    """创建测试网址收藏文件"""
    
    # 确保data目录存在
    data_dir = "data"
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)
    
    # 创建测试网址收藏文件
    bookmarks_file = os.path.join(data_dir, "bookmarks.txt")
    
    # 测试数据，包含中文
    test_bookmarks = [
        "百度|https://www.baidu.com",
        "谷歌|https://www.google.com",
        "GitHub|https://github.com",
        "哔哩哔哩|https://www.bilibili.com",
        "知乎|https://www.zhihu.com",
        "CSDN|https://www.csdn.net",
        "掘金|https://juejin.cn",
        "Stack Overflow|https://stackoverflow.com"
    ]
    
    # 写入文件，使用UTF-8编码
    with open(bookmarks_file, 'w', encoding='utf-8') as f:
        for bookmark in test_bookmarks:
            f.write(bookmark + '\n')
    
    print(f"已创建测试网址收藏文件: {bookmarks_file}")
    print(f"包含 {len(test_bookmarks)} 个测试网址")
    
    return bookmarks_file

if __name__ == "__main__":
    create_test_bookmarks()
    print("测试网址收藏已创建，请重启程序查看效果")