#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简单测试q键退出功能
"""

import sys
import time
import os
import subprocess

def test_compilation():
    """验证编译是否成功"""
    print("🔍 验证编译结果...")
    
    if os.path.exists("bin\\funny_quick.exe"):
        print("✅ 编译成功，funny_quick.exe已生成")
        file_size = os.path.getsize("bin\\funny_quick.exe")
        print(f"   文件大小: {file_size:,} 字节")
        return True
    else:
        print("❌ 编译失败，找不到funny_quick.exe")
        return False

def test_process():
    """验证程序是否在运行"""
    print("🔍 检查程序运行状态...")
    
    try:
        result = subprocess.run(['Get-Process', 'funny_quick', '-ErrorAction', 'SilentlyContinue'], 
                              capture_output=True, text=True, shell=True)
        
        if result.returncode == 0 and result.stdout.strip():
            print("✅ 程序正在运行")
            print(f"   {result.stdout.strip()}")
            return True
        else:
            print("⚠️  程序未运行")
            return False
    except Exception as e:
        print(f"❌ 检查程序状态时出错: {e}")
        return False

def test_source_changes():
    """验证源代码修改"""
    print("🔍 验证源代码修改...")
    
    try:
        with open("gui_main.cpp", "r", encoding="utf-8") as f:
            content = f.read()
        
        checks = [
            ("g_calculatorMode && wcscmp(command, L\"q\")", "检查q键退出计算模式逻辑"),
            ("g_bookmarkMode && wcscmp(command, L\"q\")", "检查q键退出网址收藏模式逻辑"),
            ("输入 q 退出计算模式", "检查计算模式提示信息"),
            ("输入 q 退出网址收藏模式", "检查网址收藏模式提示信息"),
            ("计算模式 - 输入数学表达式按回车计算", "检查计算模式标题提示"),
            ("网址收藏模式 - 搜索或浏览收藏的网址", "检查网址收藏模式标题提示")
        ]
        
        all_found = True
        for check_text, description in checks:
            if check_text in content:
                print(f"✅ {description}")
            else:
                print(f"❌ 未找到: {description}")
                all_found = False
        
        return all_found
    except Exception as e:
        print(f"❌ 读取源代码时出错: {e}")
        return False

def show_test_instructions():
    """显示手动测试指导"""
    print("\n📋 手动测试指导:")
    print("1. 按 Ctrl+F2 显示程序窗口")
    print("2. 在搜索框输入 'js' 并按回车，检查是否:")
    print("   - 进入计算模式")
    print("   - ListView显示提示信息：'计算模式 - 输入数学表达式按回车计算'")
    print("   - ListView显示提示：'输入 q 退出计算模式'")
    print("3. 在搜索框输入任意数学表达式并按回车，测试计算功能")
    print("4. 在搜索框输入 'q' 并按回车，检查是否退出计算模式")
    print("5. 在搜索框输入 'wz' 并按回车，检查是否:")
    print("   - 进入网址收藏模式")
    print("   - ListView显示提示信息：'网址收藏模式 - 搜索或浏览收藏的网址'")
    print("   - ListView显示提示：'输入 q 退出网址收藏模式'")
    print("6. 在搜索框输入 'q' 并按回车，检查是否退出网址收藏模式")

def main():
    """主测试函数"""
    print("=" * 60)
    print("🧪 q键退出功能和模式提示信息测试")
    print("=" * 60)
    
    # 检查编译
    compilation_ok = test_compilation()
    
    # 检查程序运行
    process_running = test_process()
    
    # 检查源代码修改
    source_ok = test_source_changes()
    
    # 显示测试结果
    print("\n" + "=" * 60)
    print("📊 测试结果总结:")
    print("=" * 60)
    
    print(f"✅ 编译状态: {'通过' if compilation_ok else '失败'}")
    print(f"{'✅' if process_running else '⚠️'} 程序运行: {'正常' if process_running else '未运行'}")
    print(f"✅ 源码修改: {'完成' if source_ok else '不完整'}")
    
    if compilation_ok and source_ok:
        print("\n🎉 所有自动化检查通过！")
        print("\n✨ 新功能特性:")
        print("• 计算模式输入'q'可快速退出")
        print("• 网址收藏模式输入'q'可快速退出") 
        print("• 进入模式时显示清晰的提示信息")
        print("• 用户无需使用按钮即可退出模式")
        
        show_test_instructions()
        
        print("\n💡 使用提示:")
        print("• 输入'js'进入计算模式，输入'q'退出")
        print("• 输入'wz'进入网址收藏模式，输入'q'退出")
        print("• 所有操作都有明确的文字提示")
        
        return True
    else:
        print("\n❌ 存在未通过的检查项目")
        return False

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"\n💥 测试过程中发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)