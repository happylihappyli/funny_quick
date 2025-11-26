#include <windows.h>
#include <stdio.h>

void LogToFile(const char* message) {
    FILE* f = fopen("font_test.log", "a");
    if (f) {
        fprintf(f, "[%s] %s\n", "测试", message);
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
