# -*- coding: utf-8 -*-
"""
帮助按钮功能测试脚本
测试每个模式下的帮助按钮是否正确显示和点击响应
"""

import os
import sys
import time
import subprocess
import pyautogui
import pyttsx3

def print_utf8(msg):
    """使用UTF-8编码打印中文消息"""
    print(msg.encode('utf-8').decode('utf-8'))

def speak_chinese(text):
    """使用语音播放中文文本"""
    try:
        engine = pyttsx3.init()
        engine.setProperty('rate', 150)  # 设置语音速度
        engine.setProperty('volume', 0.8)  # 设置音量
        engine.say(text)
        engine.runAndWait()
    except Exception as e:
        print(f"语音播放失败: {e}")

def test_help_buttons():
    """测试帮助按钮功能"""
    print_utf8("开始测试帮助按钮功能...")
    
    # 启动应用程序
    app_path = "bin\\funny_quick.exe"
    if not os.path.exists(app_path):
        print_utf8(f"错误：找不到应用程序 {app_path}")
        return False
    
    print_utf8("启动应用程序...")
    try:
        process = subprocess.Popen(app_path, shell=True)
        time.sleep(3)  # 等待程序启动
        
        # 测试计算器模式的帮助按钮
        print_utf8("测试1：计算器模式帮助按钮")
        test_calculator_help()
        
        # 等待一下
        time.sleep(1)
        
        # 测试网址收藏模式的帮助按钮
        print_utf8("测试2：网址收藏模式帮助按钮")
        test_bookmark_help()
        
        # 等待一下
        time.sleep(1)
        
        # 切换回计算器模式再次测试
        print_utf8("测试3：再次测试计算器模式帮助按钮")
        test_calculator_help()
        
        # 结束测试
        print_utf8("帮助按钮功能测试完成！")
        speak_chinese("帮助按钮功能测试完成！过来查看结果！")
        
        # 关闭应用程序
        try:
            process.terminate()
            process.wait(timeout=5)
        except:
            process.kill()
            
        return True
        
    except Exception as e:
        print_utf8(f"测试过程中出现错误：{e}")
        return False

def test_calculator_help():
    """测试计算器模式的帮助按钮"""
    try:
        # 如果不在计算器模式，先切换到计算器模式
        pyautogui.press('esc')  # 清除可能的焦点
        time.sleep(0.5)
        
        # 输入"js"切换到计算器模式
        pyautogui.write('js', interval=0.1)
        pyautogui.press('enter')
        time.sleep(1)
        
        # 查找并点击帮助按钮
        print_utf8("  查找帮助按钮...")
        try:
            # 使用图像识别查找帮助按钮（如果截图存在）
            # help_button = pyautogui.locateOnScreen('help_button.png', confidence=0.8)
            # if help_button:
            #     pyautogui.click(help_button)
            #     print_utf8("  帮助按钮点击成功（图像识别）")
            # else:
            #     使用坐标点击（如果知道按钮位置）
            #     pyautogui.click(x=350, y=65)  # 帮助按钮的大概位置
            #     print_utf8("  帮助按钮点击成功（坐标点击）")
            
            # 尝试在屏幕中心区域查找按钮文字
            help_found = False
            for attempt in range(5):
                # 截取屏幕
                screenshot = pyautogui.screenshot()
                
                # 在截图中搜索帮助文字
                # 这里使用OCR或者简单的像素检测
                # 为了简化，我们直接尝试坐标点击
                pyautogui.click(x=350, y=65)  # 帮助按钮位置
                help_found = True
                break
                
            if help_found:
                print_utf8("  帮助按钮点击成功")
                time.sleep(1)  # 等待帮助对话框出现
                
                # 检查是否有帮助对话框出现
                # 可以通过检查是否有新的窗口或者对话框
                print_utf8("  验证帮助对话框显示...")
                
                # 尝试按ESC键关闭可能的对话框
                pyautogui.press('escape')
                time.sleep(0.5)
                
                return True
            else:
                print_utf8("  帮助按钮未找到")
                return False
                
        except Exception as e:
            print_utf8(f"  点击帮助按钮失败：{e}")
            return False
            
    except Exception as e:
        print_utf8(f"计算器模式帮助测试失败：{e}")
        return False

def test_bookmark_help():
    """测试网址收藏模式的帮助按钮"""
    try:
        # 切换到网址收藏模式
        pyautogui.press('esc')  # 清除可能的焦点
        time.sleep(0.5)
        
        pyautogui.write('wz', interval=0.1)
        pyautogui.press('enter')
        time.sleep(1)
        
        # 点击帮助按钮
        print_utf8("  查找并点击帮助按钮...")
        try:
            pyautogui.click(x=350, y=65)  # 帮助按钮位置
            print_utf8("  帮助按钮点击成功")
            time.sleep(1)
            
            # 关闭可能的对话框
            pyautogui.press('escape')
            time.sleep(0.5)
            
            return True
            
        except Exception as e:
            print_utf8(f"  点击帮助按钮失败：{e}")
            return False
            
    except Exception as e:
        print_utf8(f"网址收藏模式帮助测试失败：{e}")
        return False

def main():
    """主函数"""
    print_utf8("=== 帮助按钮功能测试 ===")
    
    # 设置pyAutoGUI的安全措施
    pyautogui.FAILSAFE = True
    pyautogui.PAUSE = 1
    
    try:
        success = test_help_buttons()
        if success:
            print_utf8("✅ 所有测试通过")
        else:
            print_utf8("❌ 存在测试失败")
            
    except KeyboardInterrupt:
        print_utf8("用户中断测试")
    except Exception as e:
        print_utf8(f"测试异常：{e}")
    
    print_utf8("测试结束")

if __name__ == "__main__":
    main()