import os
import subprocess
import time
import psutil
import sys

def run_test():
    """测试计算模式下的表达式计算"""
    print("开始测试计算模式...")
    print("请按照以下步骤操作：")
    print("1. 程序启动后，在输入框中输入 'js' 并按回车键进入计算模式")
    print("2. 然后输入 '1+2' 并按回车键进行计算")
    print("3. 观察程序是否崩溃或正常显示计算结果")
    print("4. 关闭程序窗口")
    print("5. 在此窗口按任意键继续...")
    
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
    print(f"程序已启动，PID: {process.pid}")
    
    # 等待用户操作
    input("按回车键继续...")
    
    # 检查程序是否还在运行
    if process.poll() is None:
        print("程序仍在运行，尝试终止它...")
        process.terminate()
        process.wait(timeout=5)
    else:
        print(f"程序已退出，退出代码: {process.returncode}")
    
    # 查看最新的日志文件
    log_dir = r"E:\github\funny_quick\log"
    log_files = [f for f in os.listdir(log_dir) if f.startswith("quick_launcher_")]
    if log_files:
        log_files.sort()
        latest_log = os.path.join(log_dir, log_files[-1])
        print(f"最新日志文件: {latest_log}")
        
        # 读取最后几行日志
        with open(latest_log, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            print("最后10行日志:")
            for line in lines[-10:]:
                print(line.strip())
    
    return True

if __name__ == "__main__":
    success = run_test()
    if success:
        print("测试完成")
    else:
        print("测试失败")
    
    # 播放语音提示
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak("测试完成，请查看结果")
    except:
        print("无法播放语音提示")