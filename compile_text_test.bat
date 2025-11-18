@echo off
chcp 65001 >nul
echo 编译文本显示测试程序...
"D:\Code\VS2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" /Fe:text_display_test.exe text_display_test.cpp /EHsc /utf-8 /std:c++17 /nologo
if %errorlevel% equ 0 (
    echo 编译成功！
    echo 运行测试...
    text_display_test.exe
) else (
    echo 编译失败
)
pause