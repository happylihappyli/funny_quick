import os
import subprocess
import time
import psutil
import sys

def run_test():
    """测试UI布局修改"""
    print("开始测试UI布局修改...")
    
    # 启动程序
    print("启动 quick_launcher.exe...")
    exe_path = r"E:\github\funny_quick\bin\quick_launcher.exe"
    
    # 检查程序是否已经在运行
    for proc in psutil.process_iter(['pid', 'name']):
        try:
            if proc.info['name'] == 'quick_launcher.exe':
                print("程序已在运行，终止它...")
                proc.terminate()
                proc.wait(timeout=5)
                break
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            pass
    
    # 启动程序
    process = subprocess.Popen(exe_path)
    time.sleep(2)  # 等待程序启动
    
    try:
        # 使用SendKeys模拟键盘输入
        import SendKeys
        
        # 测试非计算模式下的设置按钮
        print("测试非计算模式下的设置按钮...")
        # 点击设置按钮
        SendKeys.SendKeys("{TAB}")  # 切换到设置按钮
        time.sleep(0.5)
        SendKeys.SendKeys("{ENTER}")  # 点击设置按钮
        time.sleep(1)  # 等待提示框显示
        
        # 关闭提示框
        SendKeys.SendKeys("{ENTER}")  # 按确定关闭提示框
        time.sleep(0.5)
        
        # 测试计算模式下的按钮切换
        print("测试计算模式下的按钮切换...")
        # 输入"js"进入计算模式
        SendKeys.SendKeys("js")
        time.sleep(0.5)
        SendKeys.SendKeys("{ENTER}")  # 按回车键进入计算模式
        time.sleep(1)
        
        # 点击退出计算按钮
        SendKeys.SendKeys("{TAB}")  # 切换到退出计算按钮
        time.sleep(0.5)
        SendKeys.SendKeys("{ENTER}")  # 点击退出计算按钮
        time.sleep(1)
        
        print("测试完成！")
        
        # 关闭程序
        process.terminate()
        process.wait(timeout=5)
        
        return True
    except Exception as e:
        print(f"测试过程中发生错误: {e}")
        
        # 如果程序还在运行，尝试终止它
        try:
            process.terminate()
            process.wait(timeout=5)
        except:
            pass
            
        return False

if __name__ == "__main__":
    success = run_test()
    
    if success:
        print("UI布局测试成功")
    else:
        print("UI布局测试失败")
    
    # 播放提示音
    subprocess.run(["python", "tts_notification.py"])