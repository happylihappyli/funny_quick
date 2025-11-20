# -*- coding: utf-8 -*-
"""
测试修复后的set和help命令功能 - 简化版
"""

import os
import subprocess
import time

def kill_process_by_name(process_name):
    """根据进程名杀死进程"""
    try:
        result = subprocess.run(f'taskkill /f /im "{process_name}"', shell=True, capture_output=True)
        return result.returncode == 0
    except:
        return False

def test_commands():
    """测试set和help命令"""
    
    print("开始测试修复后的set和help命令功能...")
    
    # 1. 清理可能存在的进程
    print("1. 清理可能存在的进程...")
    kill_process_by_name("funny_quick.exe")
    time.sleep(2)
    
    # 2. 删除旧的日志文件
    print("2. 清理旧的日志文件...")
    try:
        if os.path.exists("..\\bin\\log_error.txt"):
            os.remove("..\\bin\\log_error.txt")
    except:
        pass
    
    # 3. 启动程序（后台运行）
    print("3. 启动程序...")
    exe_path = "..\\bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print(f"错误：找不到可执行文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        process = subprocess.Popen(exe_path, cwd=".", stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(5)  # 等待程序启动并初始化
        
        # 4. 测试命令行输入（模拟用户操作）
        print("4. 模拟用户输入set和help命令...")
        
        # 发送字符串"set"和回车
        process.stdin.write(b"set\n")
        process.stdin.flush()
        time.sleep(2)
        
        # 发送字符串"help"和回车
        process.stdin.write(b"help\n")  
        process.stdin.flush()
        time.sleep(2)
        
        # 5. 等待处理完成
        print("5. 等待命令处理完成...")
        time.sleep(3)
        
        # 6. 关闭程序
        print("6. 关闭程序...")
        try:
            process.terminate()
            process.wait(timeout=5)
        except:
            process.kill()
        
        # 7. 检查日志文件
        print("7. 检查日志文件...")
        if os.path.exists("..\\bin\\log_error.txt"):
            with open("..\\bin\\log_error.txt", "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
                
            print("\n=== 日志文件内容 ===")
            print(content[-2000:])  # 显示最后2000个字符
            
            # 检查关键日志信息
            if "ProcessCommand: 处理特殊命令" in content and "set" in content:
                print("\n✅ set命令被正确处理！")
            else:
                print("\n❌ set命令处理可能有问题")
                
            if "ProcessCommand: 处理特殊命令" in content and "help" in content:
                print("✅ help命令被正确处理！") 
            else:
                print("❌ help命令处理可能有问题")
                
            if "ShowSettingsMenu" in content:
                print("✅ ShowSettingsMenu函数被调用！")
            else:
                print("❌ ShowSettingsMenu函数未被调用")
                
            if "ShowHelpInfo" in content:
                print("✅ ShowHelpInfo函数被调用！")
            else:
                print("❌ ShowHelpInfo函数未被调用")
                
            return True
        else:
            print("❌ 未找到日志文件")
            return False
            
    except Exception as e:
        print(f"测试过程中出错: {e}")
        kill_process_by_name("funny_quick.exe")
        return False

if __name__ == "__main__":
    success = test_commands()
    if success:
        print("\n🎉 测试完成！")
    else:
        print("\n💥 测试失败！")