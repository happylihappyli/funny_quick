# -*- coding: utf-8 -*-
import os

print("测试文件读取...")

# 检查文件是否存在
if not os.path.exists("data\\calculation_history.txt"):
    print("文件不存在")
    exit(1)

# 获取文件大小
file_size = os.path.getsize("data\\calculation_history.txt")
print(f"文件大小为 {file_size} 字节")

# 读取文件内容
try:
    with open("data\\calculation_history.txt", "r", encoding="utf-8-sig") as file:
        content = file.read()
        print(f"文件内容: '{content}'")
        print(f"内容长度: {len(content)}")
        
        # 按行分割
        lines = content.splitlines()
        print(f"行数: {len(lines)}")
        
        for i, line in enumerate(lines, 1):
            print(f"第{i}行: '{line}'")
            
except Exception as e:
    print(f"读取文件时出错: {e}")

print("测试完成")