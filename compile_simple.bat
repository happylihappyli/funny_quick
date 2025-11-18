@echo off
chcp 65001 >nul
echo 编译简单测试程序...
"D:\Code\VS2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" /Fe:simple_test.exe simple_test.cpp /EHsc /utf-8 /std:c++17 /nologo
if %errorlevel% equ 0 (
    echo 编译成功！
    echo 运行测试...
    simple_test.exe
) else (
    echo 编译失败
)
pause