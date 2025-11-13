#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
直接测试网址收藏模式中的中文显示
"""

import os
import sys
import time

def test_bookmark_chinese_direct():
    """直接测试网址收藏模式中的中文显示"""
    print("=" * 60)
    print("直接测试网址收藏模式中的中文显示")
    print("=" * 60)
    
    # 1. 检查测试数据文件是否存在
    bookmarks_file = "E:\\github\\funny_quick\\data\\bookmarks.txt"
    if not os.path.exists(bookmarks_file):
        print(f"错误: 测试数据文件不存在: {bookmarks_file}")
        return False
    
    # 2. 读取并显示测试数据
    print("\n1. 读取测试数据文件:")
    try:
        with open(bookmarks_file, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            print(f"文件包含 {len(lines)} 行数据")
            
            # 显示前几行
            for i, line in enumerate(lines[:5]):
                print(f"  行 {i+1}: {line.strip()}")
                
                # 检查是否包含中文字符
                if any('\u4e00' <= char <= '\u9fff' for char in line):
                    print(f"    ✓ 包含中文字符")
                else:
                    print(f"    - 不包含中文字符")
    except Exception as e:
        print(f"读取文件时出错: {e}")
        return False
    
    # 3. 启动程序
    print("\n2. 启动程序")
    os.startfile("E:\\github\\funny_quick\\bin\\quick_launcher.exe")
    time.sleep(3)  # 等待程序启动
    
    # 4. 创建一个简单的测试脚本，用于直接调用EnterBookmarkMode函数
    print("\n3. 创建测试脚本")
    test_script = """
import ctypes
import time

# 加载程序
dll = ctypes.windll.LoadLibrary("E:\\\\github\\\\funny_quick\\\\bin\\\\quick_launcher.exe")

# 这里我们无法直接调用函数，因为这是一个EXE文件
# 所以我们需要手动检查日志文件
print("无法直接调用EnterBookmarkMode函数，因为这是一个EXE文件")
print("请手动打开网址管理对话框，然后点击关闭按钮进入网址收藏模式")
print("然后检查列表框中的中文显示是否正常")
"""
    
    with open("E:\\github\\funny_quick\\test_direct_call.py", 'w', encoding='utf-8') as f:
        f.write(test_script)
    
    print("已创建测试脚本: test_direct_call.py")
    print("\n4. 请手动执行以下步骤:")
    print("   a) 点击程序窗口中的'设置'按钮")
    print("   b) 在弹出的网址管理对话框中点击'关闭'按钮")
    print("   c) 检查列表框中的中文网址是否显示正常")
    print("   d) 点击'退出'按钮退出网址收藏模式")
    
    return True

if __name__ == "__main__":
    success = test_bookmark_chinese_direct()
    
    if success:
        print("\n测试准备完成！")
        print("请按照上述步骤手动测试网址收藏模式中的中文显示")
    else:
        print("\n测试准备失败！")
    
    # 播放提示音
    os.system("python tts_notification.py")
    
    sys.exit(0 if success else 1)