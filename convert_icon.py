#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将PNG图标转换为ICO格式
"""

from PIL import Image
import os

def png_to_ico(png_path, ico_path):
    """将PNG文件转换为ICO文件"""
    try:
        # 打开PNG图像
        img = Image.open(png_path)
        
        # 创建不同尺寸的图标
        sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
        
        # 保存为ICO文件
        img.save(ico_path, format='ICO', sizes=sizes)
        print(f"成功将 {png_path} 转换为 {ico_path}")
        return True
    except Exception as e:
        print(f"转换失败: {e}")
        return False

if __name__ == "__main__":
    png_file = "app_icon.png"
    ico_file = "app_icon.ico"
    
    if os.path.exists(png_file):
        png_to_ico(png_file, ico_file)
    else:
        print(f"找不到文件: {png_file}")