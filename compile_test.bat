
@echo off
chcp 65001
echo 开始编译测试程序...
cd /d E:\github\funny_quick

REM 使用VS2022的编译器
set PATH=D:\Code\VS2022\Community\VC\Tools\MSVC\14.38.33130\bin\Hostx64\x64;%PATH%
set INCLUDE=D:\Code\VS2022\Community\VC\Tools\MSVC\14.38.33130\include;%INCLUDE%
set LIB=D:\Code\VS2022\Community\VC\Tools\MSVC\14.38.33130\lib\x64;%LIB%

cl /EHsc /std:c++23 /Fe:test_bookmark_mode.exe test_bookmark_mode.cpp gui_main.cpp resource.res user32.lib gdi32.lib comdlg32.lib comctl32.lib

if %ERRORLEVEL% EQU 0 (
    echo 编译成功
    echo 运行测试程序...
    test_bookmark_mode.exe
) else (
    echo 编译失败
)
