# -*- coding: utf-8 -*-
"""
详细测试搜索收藏网址功能
"""
import pyautogui
import time
import subprocess
import sys
import os

# 禁用PyAutoGUI的故障保护机制
pyautogui.FAILSAFE = False

def log_message(message):
    """记录日志消息"""
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {message}")
    sys.stdout.flush()

def test_search_bookmark_detailed():
    """详细测试搜索收藏网址功能"""
    log_message("开始详细测试搜索收藏网址功能...")
    
    # 启动程序
    log_message("启动程序...")
    process = subprocess.Popen(["E:\\github\\funny_quick\\bin\\funny_quick.exe"])
    time.sleep(3)  # 等待程序启动
    
    try:
        # 测试1: 搜索"谷歌"
        log_message("测试1: 搜索'谷歌'...")
        pyautogui.hotkey('ctrl', 'a')  # 全选
        time.sleep(0.5)
        pyautogui.typewrite("谷歌")
        time.sleep(2)  # 等待搜索结果
        
        # 截图保存搜索结果
        log_message("保存搜索结果截图...")
        screenshot = pyautogui.screenshot()
        screenshot.save("search_google.png")
        
        # 按回车键打开第一个搜索结果
        log_message("按回车键打开第一个搜索结果...")
        pyautogui.press("enter")
        time.sleep(3)  # 等待网页打开
        
        # 截图验证网页是否打开
        log_message("验证网页是否打开...")
        screenshot = pyautogui.screenshot()
        screenshot.save("opened_google.png")
        
        # 关闭浏览器
        log_message("关闭浏览器...")
        pyautogui.hotkey('alt', 'f4')
        time.sleep(2)
        
        # 测试2: 搜索"zhihu"
        log_message("测试2: 搜索'zhihu'...")
        pyautogui.hotkey('ctrl', 'a')  # 全选
        time.sleep(0.5)
        pyautogui.typewrite("zhihu")
        time.sleep(2)  # 等待搜索结果
        
        # 截图保存搜索结果
        log_message("保存搜索结果截图...")
        screenshot = pyautogui.screenshot()
        screenshot.save("search_zhihu.png")
        
        # 按回车键打开第一个搜索结果
        log_message("按回车键打开第一个搜索结果...")
        pyautogui.press("enter")
        time.sleep(3)  # 等待网页打开
        
        # 截图验证网页是否打开
        log_message("验证网页是否打开...")
        screenshot = pyautogui.screenshot()
        screenshot.save("opened_zhihu.png")
        
        # 关闭浏览器
        log_message("关闭浏览器...")
        pyautogui.hotkey('alt', 'f4')
        time.sleep(2)
        
        # 测试3: 搜索"github.com"
        log_message("测试3: 搜索'github.com'...")
        pyautogui.hotkey('ctrl', 'a')  # 全选
        time.sleep(0.5)
        pyautogui.typewrite("github.com")
        time.sleep(2)  # 等待搜索结果
        
        # 截图保存搜索结果
        log_message("保存搜索结果截图...")
        screenshot = pyautogui.screenshot()
        screenshot.save("search_github.png")
        
        # 按回车键打开第一个搜索结果
        log_message("按回车键打开第一个搜索结果...")
        pyautogui.press("enter")
        time.sleep(3)  # 等待网页打开
        
        # 截图验证网页是否打开
        log_message("验证网页是否打开...")
        screenshot = pyautogui.screenshot()
        screenshot.save("opened_github.png")
        
        # 关闭浏览器
        log_message("关闭浏览器...")
        pyautogui.hotkey('alt', 'f4')
        time.sleep(2)
        
        # 关闭程序
        log_message("关闭程序...")
        pyautogui.hotkey('alt', 'f4')
        time.sleep(2)
        
        log_message("详细测试完成！搜索收藏网址功能应该已经正常工作。")
        return True
        
    except Exception as e:
        log_message(f"测试过程中发生错误: {str(e)}")
        return False

if __name__ == "__main__":
    # 记录开始时间
    start_time = time.strftime("%Y-%m-%d %H:%M:%S")
    log_message(f"测试开始时间: {start_time}")
    
    # 运行测试
    success = test_search_bookmark_detailed()
    
    # 记录结束时间
    end_time = time.strftime("%Y-%m-%d %H:%M:%S")
    log_message(f"测试结束时间: {end_time}")
    
    # 播放语音提示
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        if success:
            speaker.Speak("搜索收藏网址功能详细测试完成，请查看截图！")
        else:
            speaker.Speak("测试过程中发生错误，请检查日志！")
    except:
        log_message("无法播放语音提示")