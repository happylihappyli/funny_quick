#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复C++文件编码问题的脚本
将文件重新保存为UTF-8 with BOM编码
"""

import os
import sys

def fix_file_encoding(file_path):
    """修复单个文件的编码问题"""
    try:
        # 读取原始文件内容
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # 重新保存为UTF-8 with BOM
        with open(file_path, 'w', encoding='utf-8-sig') as f:
            f.write(content)
        
        print(f"✓ 已修复文件编码: {file_path}")
        return True
    except Exception as e:
        print(f"✗ 修复文件 {file_path} 失败: {e}")
        return False

def main():
    """主函数"""
    # 需要修复的文件列表
    files_to_fix = [
        "src/webview_manager.h",
        "src/webview_manager.cpp",
        "src/common.h",
        "src/gui_main.cpp"
    ]
    
    print("开始修复C++文件编码问题...")
    
    success_count = 0
    for file_path in files_to_fix:
        if os.path.exists(file_path):
            if fix_file_encoding(file_path):
                success_count += 1
        else:
            print(f"✗ 文件不存在: {file_path}")
    
    print(f"\n修复完成！成功修复 {success_count}/{len(files_to_fix)} 个文件")
    
    if success_count == len(files_to_fix):
        print("✓ 所有文件编码修复成功")
        return 0
    else:
        print("⚠ 部分文件修复失败，请检查")
        return 1

if __name__ == "__main__":
    sys.exit(main())