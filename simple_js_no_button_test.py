#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简单验证计算模式不显示退出按钮功能
手动测试指导脚本
"""

import os
import subprocess
import time

def check_compilation():
    """检查编译是否成功"""
    print("1️⃣ 检查编译结果...")
    
    exe_path = "bin\\funny_quick.exe"
    if os.path.exists(exe_path):
        print(f"   ✓ 编译成功：{exe_path}")
        return True
    else:
        print(f"   ✗ 编译失败：找不到 {exe_path}")
        return False

def check_source_changes():
    """检查源代码修改"""
    print("2️⃣ 检查源代码修改...")
    
    source_file = "gui_main.cpp"
    if not os.path.exists(source_file):
        print(f"   ✗ 找不到源代码文件: {source_file}")
        return False
    
    # 检查代码中的注释是否正确
    with open(source_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # 检查是否注释了退出按钮显示
    if "// ShowWindow(g_hExitCalcButton, SW_SHOW);" in content:
        print("   ✓ 发现退出计算模式按钮已被注释（不会显示）")
        
        # 检查是否还有注释说明
        if "不显示退出计算模式按钮，通过输入\"q\"退出" in content:
            print("   ✓ 发现相应的注释说明")
            return True
        else:
            print("   ⚠️ 按钮注释存在但缺少说明")
            return False
    else:
        print("   ✗ 未找到按钮显示的注释")
        return False

def test_program_startup():
    """测试程序启动"""
    print("3️⃣ 验证程序启动...")
    
    try:
        print("   🚀 启动程序...")
        process = subprocess.Popen([".\\bin\\funny_quick.exe"], 
                                 cwd=os.getcwd(),
                                 stdout=subprocess.PIPE, 
                                 stderr=subprocess.PIPE)
        print(f"   ✓ 程序启动成功，PID: {process.pid}")
        
        # 等待程序完全启动
        time.sleep(3)
        
        # 检查进程是否还在运行
        try:
            process.wait(timeout=1)
            print("   ⚠️  程序已退出")
            return False
        except subprocess.TimeoutExpired:
            print("   ✓ 程序正在运行")
            
            # 终止进程
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
            return True
            
    except Exception as e:
        print(f"   ✗ 程序启动失败: {e}")
        return False

def show_manual_test_guide():
    """显示手动测试指导"""
    print("\n📋 手动测试指导")
    print("=" * 60)
    print("🎯 测试目标：验证计算模式下不显示退出按钮")
    print()
    print("📝 测试步骤：")
    print("1. 启动程序 funny_quick.exe")
    print("2. 在编辑框中输入 'js' 并按回车，进入计算模式")
    print("3. 验证：")
    print("   ✓ 不应看到'退出计算'按钮")
    print("   ✓ ListView第一行应显示：'计算模式 - 输入数学表达式按回车计算'")
    print("   ✓ ListView第二行应显示：'输入 q 退出计算模式'")
    print("4. 在编辑框中输入 '1+1' 并按回车，测试计算功能")
    print("5. 在编辑框中输入 'q' 并按回车，验证退出计算模式")
    print("6. 验证是否成功退出计算模式（输入'notepad'应能打开记事本）")
    print()
    print("🔍 关键检查点：")
    print("• 计算模式下没有退出按钮")
    print("• 用户通过提示信息知道输入'q'退出")
    print("• 'q'键能正确退出计算模式")
    print("• 退出后能正常使用其他功能")

def main():
    """主测试函数"""
    print("🧪 计算模式无按钮退出功能验证")
    print("=" * 50)
    
    # 检查编译
    if not check_compilation():
        print("\n❌ 编译失败，请先修复编译问题")
        return False
    
    # 检查源代码修改
    if not check_source_changes():
        print("\n❌ 源代码修改不正确")
        return False
    
    # 测试程序启动
    if not test_program_startup():
        print("\n❌ 程序启动失败")
        return False
    
    print("\n✅ 代码修改验证成功")
    show_manual_test_guide()
    
    return True

if __name__ == "__main__":
    try:
        success = main()
        if success:
            print("\n🎉 验证完成！")
            print("\n💡 现在可以手动测试验证功能是否正常工作")
        else:
            print("\n❌ 验证失败！")
    except Exception as e:
        print(f"\n❌ 验证过程出错: {e}")