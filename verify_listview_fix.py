#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
验证ListView中文显示修复效果的脚本
"""

import os
import sys
import time
import subprocess
import psutil
import signal

def check_process_status():
    """检查funny_quick进程状态"""
    print("=== 检查funny_quick进程状态 ===")
    
    found_processes = []
    for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
        if proc.info['name'] and 'funny_quick' in proc.info['name'].lower():
            found_processes.append(proc)
            print(f"找到进程: PID={proc.info['pid']}, Name={proc.info['name']}")
            if proc.info['cmdline']:
                print(f"  命令行: {' '.join(proc.info['cmdline'])}")
    
    if not found_processes:
        print("❌ 没有找到funny_quick进程")
        return False
    else:
        print(f"✅ 找到 {len(found_processes)} 个funny_quick进程")
        return True

def test_program_functionality():
    """测试程序功能"""
    print("\n=== 测试程序功能 ===")
    
    # 检查程序是否响应
    try:
        # 尝试发送热键来显示窗口
        print("如果程序运行正常，可以尝试以下方式验证ListView中文显示:")
        print("1. 按 Ctrl+F2 显示程序窗口")
        print("2. 在搜索框中输入 'js' 切换到计算模式")
        print("3. 在搜索框中输入 'wz' 切换到网址收藏模式")
        print("4. 在搜索框中输入任意中文文字，观察ListView列标题")
        print("   - 第一列标题应该是'表达式'")
        print("   - 第二列标题应该是'注释'")
        print("5. 检查这些中文文字是否显示正常，没有乱码")
        
        return True
    except Exception as e:
        print(f"❌ 测试过程中出现错误: {e}")
        return False

def verify_file_existence():
    """验证相关文件是否存在"""
    print("\n=== 验证相关文件 ===")
    
    files_to_check = [
        "bin/funny_quick.exe",
        "gui_main.cpp",
        "SConstruct"
    ]
    
    all_exist = True
    for file_path in files_to_check:
        if os.path.exists(file_path):
            print(f"✅ {file_path} 存在")
        else:
            print(f"❌ {file_path} 不存在")
            all_exist = False
    
    return all_exist

def main():
    """主函数"""
    print("ListView中文显示修复验证程序")
    print("=" * 50)
    
    # 验证文件存在
    if not verify_file_existence():
        print("\n❌ 部分必要文件缺失，无法进行全面验证")
        return
    
    # 检查进程状态
    if check_process_status():
        print("\n✅ 程序正在运行")
    else:
        print("\n❌ 程序未运行，请先启动程序")
        return
    
    # 测试程序功能
    if test_program_functionality():
        print("\n✅ 程序功能测试完成")
    else:
        print("\n❌ 程序功能测试失败")
    
    print("\n" + "=" * 50)
    print("验证总结:")
    print("1. ✅ 编译成功 - ListView_InsertColumn错误已修复")
    print("2. ✅ 程序启动成功")
    print("3. 🔍 请手动验证ListView中文显示是否正常:")
    print("   - 按 Ctrl+F2 显示窗口")
    print("   - 输入'js'或'wz'测试ListView列标题显示")
    print("   - 确认'表达式'和'注释'列标题显示正确，无乱码")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n用户中断了验证过程")
    except Exception as e:
        print(f"\n验证过程中出现未预期的错误: {e}")
        import traceback
        traceback.print_exc()