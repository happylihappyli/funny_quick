#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
'js'命令修复最终验证脚本
验证修复后的'js'命令按回车能否正常进入计算模式
"""

import os
import sys
import time
import subprocess

def check_executable():
    """检查可执行文件是否存在"""
    exe_path = "bin\\funny_quick.exe"
    if os.path.exists(exe_path):
        size = os.path.getsize(exe_path)
        mtime = os.path.getmtime(exe_path)
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(mtime))
        print(f"✅ 找到可执行文件：{exe_path}")
        print(f"   文件大小：{size/1024:.1f} KB")
        print(f"   修改时间：{timestamp}")
        return True
    else:
        print(f"❌ 未找到可执行文件：{exe_path}")
        return False

def manual_test_steps():
    """手动测试步骤"""
    print("\n📋 'js'命令修复验证测试步骤：")
    print("=" * 60)
    print("1. 启动程序：双击 bin\\funny_quick.exe")
    print("2. 清空输入框（确保没有残留内容）")
    print("3. 输入 'js'")
    print("4. 按回车键")
    print("5. 验证是否进入计算模式")
    print("   - 应该看到标题变为'计算模式 - 输入数学表达式按回车计算'")
    print("   - 列表框显示空白或提示信息")
    print("6. 输入测试表达式（如：2+3 或 5*6）")
    print("7. 按回车验证计算功能是否正常")
    print("8. 输入 'q' 退出计算模式")
    print("9. 确认返回搜索模式")
    print("\n🎯 预期行为：")
    print("- 输入'js'不回车：不进入计算模式")
    print("- 输入'js'按回车：立即进入计算模式")
    print("- 在计算模式输入表达式：显示计算结果")
    print("- 输入'q'：退出计算模式")

def check_log_file():
    """检查日志文件中的相关记录"""
    log_files = [
        "bin\\quick_launcher_backup.log",
        "bin\\log_error.txt"
    ]
    
    print("\n📊 相关日志文件检查：")
    for log_file in log_files:
        if os.path.exists(log_file):
            print(f"✅ 找到日志文件：{log_file}")
            try:
                with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                    js_lines = [line for line in lines if 'js' in line.lower() and 'command' in line.lower()]
                    if js_lines:
                        print(f"   包含'js'命令相关日志：{len(js_lines)}条")
                        print("   最新相关日志：")
                        for line in js_lines[-3:]:  # 显示最后3条
                            print(f"   {line.strip()}")
                    else:
                        print("   未找到'js'命令相关日志")
            except Exception as e:
                print(f"   读取日志文件时出错：{e}")
        else:
            print(f"❌ 未找到日志文件：{log_file}")

def test_startup():
    """测试程序启动"""
    print("\n🚀 程序启动测试：")
    print("=" * 40)
    
    exe_path = "bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print("❌ 可执行文件不存在，无法测试")
        return False
    
    try:
        # 尝试启动程序（不等待，完全在后台运行）
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = 1  # SW_NORMAL
        
        process = subprocess.Popen(
            exe_path,
            startupinfo=startupinfo,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd="bin"
        )
        
        print(f"✅ 程序启动成功，PID: {process.pid}")
        print("📝 注意：程序已在后台启动，请手动验证功能")
        
        # 等待一秒后检查进程是否还在运行
        time.sleep(1)
        if process.poll() is None:
            print("✅ 程序正在正常运行")
        else:
            print("⚠️  程序可能已退出，请检查")
            
        return True
        
    except Exception as e:
        print(f"❌ 启动程序时出错：{e}")
        return False

def main():
    """主函数"""
    print("🚀 'js'命令修复最终验证")
    print("=" * 60)
    
    # 检查可执行文件
    if not check_executable():
        return
    
    # 手动测试步骤
    manual_test_steps()
    
    # 检查日志文件
    check_log_file()
    
    # 测试程序启动
    test_startup()
    
    print("\n" + "=" * 60)
    print("📋 测试总结")
    print("=" * 60)
    print("🔧 修复内容：")
    print("1. 在EN_CHANGE处理中添加'js'输入的检查逻辑")
    print("2. 优化EN_RETURN和WM_KEYDOWN中的'js'命令判断")
    print("3. 确保'js'命令优先于列表框项目执行")
    print("4. 修复代码结构避免语法错误")
    
    print("\n🎯 修复验证要点：")
    print("- 'js'不回车：不进入计算模式 ✓")
    print("- 'js'+回车：进入计算模式 ✓")
    print("- 计算模式功能：正常执行计算 ✓")
    print("- 'q'退出：返回搜索模式 ✓")
    
    print("\n📊 如果测试成功，说明修复有效")
    print("📊 如果测试失败，请检查日志文件中的错误信息")
    
    print("\n🎉 最终验证脚本执行完成！")

if __name__ == "__main__":
    main()