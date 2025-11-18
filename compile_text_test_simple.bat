@echo off
cd /d "E:\GitHub3\funny_quick"
chcp 65001
"D:\Code\VS2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" /Fe:text_display_test.exe text_display_test.cpp /EHsc /utf-8 /std:c++17 /nologo
echo 编译完成，运行测试...
text_display_test.exe