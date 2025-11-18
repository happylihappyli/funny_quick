#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import sys

def main():
    print("=== 编译和测试主程序 ===")
    
    # 设置工作目录
    os.chdir(r"E:\GitHub3\funny_quick")
    
    # 编译器路径
    cl_path = r"D:\Code\VS2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
    
    # SFML路径
    sfml_include = r"D:\Code\SFML-2.6.1\include"
    sfml_lib = r"D:\Code\SFML-2.6.1\lib"
    
    # 编译命令
    compile_cmd = f'"{cl_path}" /Fe:funny_quick.exe sfml_main.cpp /I"{sfml_include}" /link /LIBPATH:"{sfml_lib}" sfml-graphics-d.lib sfml-window-d.lib sfml-system-d.lib /EHsc /utf-8 /std:c++17 /nologo'
    
    print(f"编译命令: {compile_cmd}")
    
    # 执行编译
    try:
        print("开始编译...")
        result = subprocess.run(compile_cmd, shell=True, capture_output=True, text=True, encoding='utf-8')
        
        print("编译输出:")
        print(result.stdout)
        if result.stderr:
            print("编译错误:")
            print(result.stderr)
        print(f"编译返回码: {result.returncode}")
        
        if result.returncode == 0:
            print("编译成功！")
            
            # 运行测试程序
            print("\n开始运行程序...")
            test_result = subprocess.run(["./funny_quick.exe"], capture_output=True, text=True)
            
            print("程序输出:")
            print(test_result.stdout)
            if test_result.stderr:
                print("程序错误:")
                print(test_result.stderr)
            print(f"程序返回码: {test_result.returncode}")
        else:
            print("编译失败！")
            
    except Exception as e:
        print(f"执行过程中出错: {e}")

if __name__ == "__main__":
    main()