@echo off
chcp 65001 > nul
title Dual Language Build Script

echo ========================================
echo 双语言编译脚本 - Dual Language Build
echo ========================================
echo.

REM 检查编译参数
set BUILD_TYPE=%1
set PROJECT_PATH=%~dp0
set LOG_FILE=%PROJECT_PATH%build_log.txt

if "%BUILD_TYPE%"=="" (
    echo 使用方法 Usage: build_dual_language.bat [chinese^|english^|all]
    echo 示例 Examples:
    echo   build_dual_language.bat chinese   - 只编译中文版
    echo   build_dual_language.bat english   - 只编译英文版  
    echo   build_dual_language.bat all       - 同时编译中英文版
    echo.
    echo 默认编译中文版 Default: Chinese version
    set BUILD_TYPE=chinese

echo 测试编译修复后的表达式解析功能...
)

echo 开始编译 Starting build: %BUILD_TYPE%
echo 开始时间 Start time: %date% %time% >> %LOG_FILE%

cd /d "%PROJECT_PATH%"
chcp 65001

REM 清理之前的构建
echo 清理构建目录 Cleaning build directories...
if exist bin rmdir /s /q bin
if exist obj rmdir /s /q obj
mkdir bin
mkdir obj

echo 开始时间 Start time: %date% %time%

echo.
echo ========================================
echo 开始编译 %BUILD_TYPE% 版
echo ========================================

if "%BUILD_TYPE%"=="chinese" (
    echo 编译中文版 Building Chinese Version...
    echo 开始时间: %date% %time%
    chcp 65001
    scons LANGUAGE=chinese
    echo 编译完成时间: %date% %time%
    if errorlevel 1 (
        echo ❌ 中文版编译失败 Chinese build failed
        echo 编译错误信息已记录到 %LOG_FILE%
        goto :error
    ) else (
        echo ✅ 中文版编译成功 Chinese build completed
        if exist bin\quick_launcher.exe (
            rename bin\quick_launcher.exe quick_launcher_chinese.exe
            echo 已重命名为: bin\quick_launcher_chinese.exe
        )
    )
) else if "%BUILD_TYPE%"=="english" (
    echo 编译英文版 Building English Version...
    chcp 65001
    scons LANGUAGE=english
    if errorlevel 1 (
        echo ❌ 英文版编译失败 English build failed
        echo 编译错误信息已记录到 %LOG_FILE%
        goto :error
    ) else (
        echo ✅ 英文版编译成功 English build completed
        if exist bin\quick_launcher.exe (
            rename bin\quick_launcher.exe quick_launcher_english.exe
            echo 已重命名为: bin\quick_launcher_english.exe
        )
    )
) else if "%BUILD_TYPE%"=="all" (
    echo 编译中英文两个版本 Building both Chinese and English versions...
    
    REM 先编译中文版
    echo.
    echo 第一阶段：编译中文版 Phase 1: Building Chinese version...
    chcp 65001
    scons LANGUAGE=chinese
    if errorlevel 1 (
        echo ❌ 中文版编译失败 Chinese build failed
        goto :error
    ) else (
        echo ✅ 中文版编译成功 Chinese build completed
        if exist bin\quick_launcher.exe (
            rename bin\quick_launcher.exe quick_launcher_chinese.exe
            echo 已重命名为: bin\quick_launcher_chinese.exe
        )
    )
    
    REM 再编译英文版
    echo.
    echo 第二阶段：编译英文版 Phase 2: Building English version...
    chcp 65001
    scons LANGUAGE=english
    if errorlevel 1 (
        echo ❌ 英文版编译失败 English build failed
        goto :error
    ) else (
        echo ✅ 英文版编译成功 English build completed
        if exist bin\quick_launcher.exe (
            rename bin\quick_launcher.exe quick_launcher_english.exe
            echo 已重命名为: bin\quick_launcher_english.exe
        )
    )
) else (
    echo ❌ 无效的编译参数 Invalid build parameter: %BUILD_TYPE%
    echo 有效的参数: chinese, english, all
    goto :error
)

echo.
echo ========================================
echo 结束时间 Completion time: %date% %time%
echo.
echo 编译结果 Build Results
echo ========================================
if exist bin\quick_launcher_chinese.exe echo ✅ 中文版: bin\quick_launcher_chinese.exe
if exist bin\quick_launcher_english.exe echo ✅ 英文版: bin\quick_launcher_english.exe
echo.
echo 两个版本的快捷项比较:
echo Comparison of shortcut items between versions:

if exist bin\quick_launcher_chinese.exe (
    echo 英文版vs中文版差异检查 English vs Chinese version difference check:
    fc /b bin\quick_launcher_chinese.exe bin\quick_launcher_english.exe >nul 2>&1
    if errorlevel 1 (
        echo ✅ 两个版本大小不同，编译成功 (Different sizes - build successful)
        echo 中文版大小: 
        dir bin\quick_launcher_chinese.exe | findstr "quick_launcher_chinese"
        echo 英文版大小:
        dir bin\quick_launcher_english.exe | findstr "quick_launcher_english"
    ) else (
        echo ⚠️  警告: 两个版本完全相同，可能存在问题 (Warning: Versions identical - possible issue)
    )
)

echo.
echo 完成时间 Completion time: %date% %time% >> %LOG_FILE%
echo ✅ 双语言编译完成 Dual language build completed!

goto :end

:error
echo.
echo ❌ 编译失败 Build failed!
echo 错误时间 Error time: %date% %time% >> %LOG_FILE%
echo.
echo 请检查以下项目 Please check:
echo 1. Visual Studio 2022 是否已安装 Visual Studio 2022 installed
echo 2. scons 是否可用 scons is available  
echo 3. 源代码文件是否存在 Source files exist
echo 4. 国际化头文件是否创建 internationalization.h created
echo.

:end
pause