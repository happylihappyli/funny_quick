# -*- coding: utf-8 -*-
"""
测试修复后的set和help命令功能
"""

import os
import subprocess
import time
import pywinauto
import signal
from pywinauto import findwindows

def kill_process_by_name(process_name):
    """根据进程名杀死进程"""
    try:
        os.system(f'taskkill /f /im "{process_name}" >nul 2>&1')
    except:
        pass

def test_commands():
    """测试set和help命令"""
    
    # 1. 清理可能存在的进程
    print("1. 清理可能存在的进程...")
    kill_process_by_name("funny_quick.exe")
    time.sleep(2)
    
    # 2. 启动程序
    print("2. 启动funny_quick程序...")
    exe_path = "bin\\funny_quick.exe"
    if not os.path.exists(exe_path):
        print(f"错误：找不到可执行文件 {exe_path}")
        return False
    
    try:
        # 启动程序
        process = subprocess.Popen(exe_path, cwd=".")
        time.sleep(3)  # 等待程序启动
        
        # 3. 查找窗口
        print("3. 查找程序窗口...")
        window = None
        for attempt in range(10):
            try:
                windows = findwindows.find_windows(title_re=".*BV.*快启工具箱.*")
                if windows:
                    window = pywinauto.Application().connect(handle=windows[0])
                    window = window.window(title_re=".*BV.*快启工具箱.*")
                    print("找到窗口!")
                    break
            except:
                pass
            time.sleep(1)
        
        if not window:
            print("未找到窗口，测试失败")
            return False
        
        # 4. 测试set命令
        print("4. 测试set命令...")
        window.set_focus()
        edit_ctrl = window["Edit"]
        
        # 清空输入框并输入set
        edit_ctrl.set_text("set")
        time.sleep(1)
        
        # 发送回车键
        edit_ctrl.type_keys("{ENTER}")
        time.sleep(2)
        
        # 5. 测试help命令
        print("5. 测试help命令...")
        edit_ctrl.set_text("help")
        time.sleep(1)
        edit_ctrl.type_keys("{ENTER}")
        time.sleep(2)
        
        # 6. 结束测试
        print("6. 关闭程序...")
        window.close()
        time.sleep(1)
        
        # 强制结束进程
        kill_process_by_name("funny_quick.exe")
        
        print("测试完成！")
        return True
        
    except Exception as e:
        print(f"测试过程中出错: {e}")
        kill_process_by_name("funny_quick.exe")
        return False

if __name__ == "__main__":
    success = test_commands()
    if success:
        print("测试成功完成！")
    else:
        print("测试失败！")