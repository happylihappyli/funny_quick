#include <windows.h>
#include <iostream>
#include <fstream>

// 简单测试字体创建功能
HFONT CreateUIFont()
{
    // 如果字体已存在，先释放
    HFONT hFont = NULL;
    
    // 创建更光滑的字体 - 使用微软雅黑，启用抗锯齿
    LOGFONTW lf = {0};
    lf.lfHeight = -16;  // 字体大小，负值表示字符高度
    lf.lfWeight = FW_NORMAL;  // 正常字重
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;  // 使用TrueType字体
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;  // 启用ClearType抗锯齿
    lf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH;  // 无衬线字体
    
    // 尝试使用微软雅黑字体，这是Windows系统中显示效果最好的字体之一
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei UI");
    
    hFont = CreateFontIndirectW(&lf);
    
    // 如果创建失败，尝试使用默认的微软雅黑
    if (hFont == NULL)
    {
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei");
        hFont = CreateFontIndirectW(&lf);
    }
    
    // 如果还是失败，尝试使用Segoe UI
    if (hFont == NULL)
    {
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Segoe UI");
        hFont = CreateFontIndirectW(&lf);
    }
    
    // 最后的备选方案：使用系统默认字体
    if (hFont == NULL)
    {
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
    
    return hFont;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    std::ofstream logFile("font_test.log", std::ios::app);
    logFile << "=== 字体测试开始 ===" << std::endl;
    
    // 测试字体创建
    HFONT hFont = CreateUIFont();
    if (hFont != NULL)
    {
        logFile << "字体创建成功" << std::endl;
        logFile << "字体句柄: " << (void*)hFont << std::endl;
        
        // 测试字体信息
        LOGFONTW lf = {0};
        if (GetObjectW(hFont, sizeof(LOGFONTW), &lf))
        {
            logFile << "字体名称: " << lf.lfFaceName << std::endl;
            logFile << "字体高度: " << lf.lfHeight << std::endl;
            logFile << "字体质量: " << lf.lfQuality << std::endl;
        }
        
        // 清理字体
        DeleteObject(hFont);
        logFile << "字体已清理" << std::endl;
    }
    else
    {
        logFile << "字体创建失败" << std::endl;
    }
    
    logFile << "=== 字体测试结束 ===" << std::endl;
    logFile.close();
    
    // 显示消息框确认测试完成
    MessageBoxW(NULL, L"字体测试完成，请查看 font_test.log 文件了解详细信息。", L"字体测试", MB_OK);
    
    return 0;
}