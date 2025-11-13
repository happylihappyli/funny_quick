# -*- coding: utf-8 -*-
"""
修复gui_main.cpp文件的编码问题
"""
import os

def fix_file_encoding(file_path):
    """修复文件编码为UTF-8+BOM"""
    try:
        # 读取原文件内容
        with open(file_path, 'r', encoding='utf-8-sig') as f:
            content = f.read()
        
        # 重新写入，使用UTF-8+BOM编码
        with open(file_path, 'w', encoding='utf-8-sig') as f:
            f.write(content)
        
        print(f"文件 {file_path} 编码已修复为UTF-8+BOM")
        return True
        
    except Exception as e:
        print(f"修复文件编码失败: {e}")
        return False

if __name__ == "__main__":
    file_path = "e:\\github\\funny_quick\\gui_main.cpp"
    fix_file_encoding(file_path)