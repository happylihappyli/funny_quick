# -*- coding: utf-8 -*-
"""
最小化的字体测试程序
"""
import subprocess
import time

def test_font_creation():
    """测试字体创建功能"""
    print("=== 创建最小化测试程序 ===")
    
    # 创建最简单的测试代码
    test_code = '''#include <windows.h>
#include <stdio.h>

void LogToFile(const char* message) {
    FILE* f = fopen("font_test.log", "a");
    if (f) {
        fprintf(f, "[%s] %s\\n", "测试", message);
        fclose(f);
    }
}

HFONT CreateUIFont() {
    LogToFile("CreateUIFont: 开始创建字体");
    
    LOGFONTW lf = {0};
    lf.lfHeight = -16;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;
    
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei UI");
    
    HFONT hFont = CreateFontIndirectW(&lf);
    
    if (hFont == NULL) {
        LogToFile("CreateUIFont: 尝试备选字体 Microsoft YaHei");
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei");
        hFont = CreateFontIndirectW(&lf);
    }
    
    if (hFont == NULL) {
        LogToFile("CreateUIFont: 尝试备选字体 Segoe UI");
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Segoe UI");
        hFont = CreateFontIndirectW(&lf);
    }
    
    if (hFont == NULL) {
        LogToFile("CreateUIFont: 使用系统默认字体");
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    } else {
        LogToFile("CreateUIFont: 成功创建高质量字体");
    }
    
    return hFont;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    LogToFile("测试程序启动");
    
    HFONT hFont = CreateUIFont();
    LogToFile("字体创建完成");
    
    if (hFont != NULL) {
        LogToFile("字体句柄有效");
        DeleteObject(hFont);
        LogToFile("字体资源已释放");
    } else {
        LogToFile("字体句柄无效");
    }
    
    LogToFile("测试程序结束");
    return 0;
}
'''
    
    # 写入测试文件
    with open('minimal_font_test.cpp', 'w', encoding='utf-8') as f:
        f.write(test_code)
    
    print("测试代码已写入 minimal_font_test.cpp")
    
    # 编译测试程序
    print("编译测试程序...")
    try:
        # 使用VS编译器的cl命令
        result = subprocess.run([
            'cl', 
            '/Fe:minimal_font_test.exe',
            'minimal_font_test.cpp'
        ], 
        capture_output=True, 
        text=True, 
        cwd='.',
        shell=True)
        
        print(f"编译结果: 退出代码 {result.returncode}")
        if result.stdout:
            print("标准输出:")
            print(result.stdout)
        if result.stderr:
            print("错误输出:")
            print(result.stderr)
        
        if result.returncode == 0:
            print("编译成功!")
            
            # 删除旧日志文件
            try:
                import os
                os.remove('font_test.log')
            except:
                pass
            
            # 运行测试程序
            print("运行测试程序...")
            test_result = subprocess.run(['minimal_font_test.exe'], capture_output=True, text=True, cwd='.')
            
            if test_result.returncode == 0:
                print("测试程序运行成功!")
            else:
                print(f"测试程序运行失败: {test_result.returncode}")
                if test_result.stderr:
                    print("错误输出:", test_result.stderr)
            
            # 检查日志文件
            try:
                with open('font_test.log', 'r', encoding='utf-8') as f:
                    log_content = f.read()
                print("\n=== 测试日志内容 ===")
                print(log_content)
            except Exception as e:
                print(f"无法读取日志文件: {e}")
        else:
            print("编译失败!")
            
    except Exception as e:
        print(f"编译过程出错: {e}")

if __name__ == "__main__":
    test_font_creation()