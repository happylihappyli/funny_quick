import sys
import os
import time
import subprocess
from pywinauto import Application
from pywinauto.keyboard import send_keys

# 设置控制台输出编码为UTF-8
if sys.platform == 'win32':
    os.system('chcp 65001 >nul')

def test_multi_operator_expression():
    """测试多运算符表达式计算"""
    print("测试多运算符表达式计算")
    print("=" * 50)
    
    try:
        # 启动程序
        print("启动程序...")
        app_path = r"E:\github\funny_quick\bin\quick_launcher.exe"
        app = Application(backend="uia").start(app_path)
        time.sleep(1)  # 等待程序启动
        
        # 获取主窗口
        main_window = app.window(title_re=".*")
        main_window.wait('ready', timeout=10)
        
        # 获取编辑框
        edit_box = main_window.child_window(auto_id="1001", control_type="Edit")
        
        # 输入测试表达式
        test_expression = "2+3+4+5"
        print(f"输入测试表达式: {test_expression}")
        edit_box.set_text(test_expression)
        time.sleep(0.5)
        
        # 按回车键计算
        print("按回车键计算...")
        send_keys('{ENTER}')
        time.sleep(1)
        
        # 获取计算结果
        result_text = edit_box.get_value()
        print(f"计算结果: {result_text}")
        
        # 验证结果
        expected_result = "14"
        if result_text == expected_result:
            print(f"✓ 测试通过! {test_expression} = {expected_result}")
        else:
            print(f"✗ 测试失败! 期望: {expected_result}, 实际: {result_text}")
        
        # 测试带等号的表达式
        test_expression_with_equal = "2+3+4+5=7"
        print(f"\n输入带等号的测试表达式: {test_expression_with_equal}")
        edit_box.set_text(test_expression_with_equal)
        time.sleep(0.5)
        
        # 按回车键计算
        print("按回车键计算...")
        send_keys('{ENTER}')
        time.sleep(1)
        
        # 获取计算结果
        result_text = edit_box.get_value()
        print(f"计算结果: {result_text}")
        
        # 验证结果
        if result_text == expected_result:
            print(f"✓ 测试通过! {test_expression_with_equal} = {expected_result} (忽略等号后内容)")
        else:
            print(f"✗ 测试失败! 期望: {expected_result}, 实际: {result_text}")
        
        # 关闭程序
        print("\n关闭程序...")
        app.kill()
        
        return True
        
    except Exception as e:
        print(f"测试过程中发生错误: {str(e)}")
        try:
            app.kill()
        except:
            pass
        return False

if __name__ == "__main__":
    success = test_multi_operator_expression()
    if success:
        print("\n测试完成!")
    else:
        print("\n测试失败!")
    
    # 播放语音提示
    try:
        import win32com.client
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak("测试完成，请查看结果")
    except:
        pass