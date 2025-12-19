#include "ui_helpers.h"
#include "common.h"
#include "logger.h"
#include "webview_manager.h"
#include <stdio.h>
#include <fstream>
#include <vector>
#include <commctrl.h>

// Global font handle definition
HFONT g_hFont = NULL;

// 创建UI字体函数
void CreateUIFont()
{
    // 如果字体已存在，先释放
    if (g_hFont != NULL)
    {
        DeleteObject(g_hFont);
        g_hFont = NULL;
    }
    
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
    
    g_hFont = CreateFontIndirectW(&lf);
    
    // 如果创建失败，尝试使用默认的微软雅黑
    if (g_hFont == NULL)
    {
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Microsoft YaHei");
        g_hFont = CreateFontIndirectW(&lf);
    }
    
    // 如果还是失败，尝试使用Segoe UI
    if (g_hFont == NULL)
    {
        wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Segoe UI");
        g_hFont = CreateFontIndirectW(&lf);
    }
    
    // 最后的备选方案：使用系统默认字体
    if (g_hFont == NULL)
    {
        g_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        LogToFile("CreateUIFont: 使用系统默认字体");
    }
    else
    {
        LogToFile("CreateUIFont: 成功创建高质量字体");
    }
}

// 应用字体到控件函数
void ApplyFontToControl(HWND hWnd)
{
    if (g_hFont != NULL && hWnd != NULL)
    {
        SendMessageW(hWnd, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }
}

// 窗口大小调整时重新布局控件
void LayoutControls(int windowWidth, int windowHeight)
{
    if (windowWidth <= 0 || windowHeight <= 0)
    {
        return;
    }
    
    char layoutLog[200] = {0};
    sprintf(layoutLog, "LayoutControls: 开始重新布局控件，窗口大小 %dx%d", windowWidth, windowHeight);
    LogToFile(layoutLog);
    
    // 边距
    int margin = 10;
    int spacing = 10;
    
    // 控件高度
    int editHeight = 25;
    int buttonHeight = 25;
    
    // 计算各控件的位置和大小
    // 编辑框：上方位置，宽度占满窗口
    int editX = margin;
    int editY = margin;
    int editWidth = windowWidth - (margin * 2);
    
    // ListView：文本框下方，占据中间大部分空间
    int listViewX = margin;
    int listViewY = editY + editHeight*2 + spacing;
    int listViewWidth = windowWidth - (margin * 2);
    int listViewHeight = windowHeight - listViewY - margin - buttonHeight - margin; // 为底部按钮和边距留空间
    
    // 按钮区域：底部位置
    int buttonY = windowHeight - margin - buttonHeight;
    int buttonWidth = 80;
    int buttonSpacing = 10;
    
    // 按钮位置：设置按钮在左侧，退出按钮在右侧
    int settingsButtonX = margin;
    int exitButtonX = windowWidth - margin - buttonWidth;
    
    // 应用新的位置和大小到各个控件
    SetWindowPos(g_hEdit, NULL, editX, editY, editWidth, editHeight, SWP_NOZORDER);
    SetWindowPos(g_hListView, NULL, listViewX, listViewY, listViewWidth, listViewHeight, SWP_NOZORDER);
    
    // 更新 WebView2 占位窗口的位置和大小（与 ListView 相同）
    if (g_hWebView2 != NULL)
    {
        SetWindowPos(g_hWebView2, NULL, listViewX, listViewY, listViewWidth, listViewHeight, SWP_NOZORDER);
        
        // 更新 WebView2 控制器的位置和大小
        if (g_webViewController != NULL)
        {
            RECT bounds = {0, 0, listViewWidth, listViewHeight};
            g_webViewController->put_Bounds(bounds);
        }
    }
    
    SetWindowPos(g_hSettingsButton, NULL, settingsButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    
    // 根据当前模式显示相应的退出按钮
    if (g_calculatorMode)
    {
        // 计算模式：显示计算模式退出按钮
        SetWindowPos(g_hExitCalcButton, NULL, exitButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
        
        // 计算模式菜单按钮位置：在退出按钮左侧
        int calcMenuButtonX = exitButtonX - buttonWidth - buttonSpacing;
        SetWindowPos(g_hCalcMenuButton, NULL, calcMenuButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    }
    else if (g_fileMode)
    {
        // 文件模式：显示文件模式退出按钮
        SetWindowPos(g_hExitFileButton, NULL, exitButtonX, buttonY, buttonWidth, buttonHeight, SWP_NOZORDER);
    }
    else
    {
        // 普通模式：隐藏所有退出按钮
        SetWindowPos(g_hExitCalcButton, NULL, exitButtonX, buttonY, buttonWidth, buttonHeight, SWP_HIDEWINDOW);
        SetWindowPos(g_hExitFileButton, NULL, exitButtonX, buttonY, buttonWidth, buttonHeight, SWP_HIDEWINDOW);
        SetWindowPos(g_hCalcMenuButton, NULL, exitButtonX - buttonWidth - buttonSpacing, buttonY, buttonWidth, buttonHeight, SWP_HIDEWINDOW);
    }
    
    // 刷新ListView显示
    if (g_hListView != NULL)
    {
        InvalidateRect(g_hListView, NULL, TRUE);
    }
    
    sprintf(layoutLog, "LayoutControls: 控件布局完成");
    LogToFile(layoutLog);
}

// 根据当前模式更新ListView列标题
void UpdateListViewColumns()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return;
    }
    
    LVCOLUMNW lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    
    if (g_calculatorMode)
    {
        // 计算模式：表达式 | 结果
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"表达式";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 0, &lvc);
        
        lvc.iSubItem = 1;
        lvc.pszText = (WCHAR*)L"结果";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 1, &lvc);
        
        LogToFile("UpdateListViewColumns: 已更新为计算模式列标题（表达式 | 结果）");
    }
    else if (g_fileMode)
    {
        // 文件模式：文件名 | 路径 | 大小 | 修改时间 | 类型
        
        // 第一列：文件名
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"文件名";
        lvc.cx = 150;
        ListView_SetColumn(g_hListView, 0, &lvc);
        
        // 确保有足够的列数（至少5列）
        int columnCount = Header_GetItemCount(ListView_GetHeader(g_hListView));
        
        // 第二列：路径
        if (columnCount < 2)
        {
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 200;
            ListView_InsertColumn(g_hListView, 1, &lvc);
        }
        else
        {
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 200;
            ListView_SetColumn(g_hListView, 1, &lvc);
        }
        
        // 第三列：大小
        if (columnCount < 3)
        {
            lvc.iSubItem = 2;
            lvc.pszText = (WCHAR*)L"大小";
            lvc.cx = 80;
            ListView_InsertColumn(g_hListView, 2, &lvc);
        }
        else
        {
            lvc.iSubItem = 2;
            lvc.pszText = (WCHAR*)L"大小";
            lvc.cx = 80;
            ListView_SetColumn(g_hListView, 2, &lvc);
        }
        
        // 第四列：修改时间
        if (columnCount < 4)
        {
            lvc.iSubItem = 3;
            lvc.pszText = (WCHAR*)L"修改时间";
            lvc.cx = 120;
            ListView_InsertColumn(g_hListView, 3, &lvc);
        }
        else
        {
            lvc.iSubItem = 3;
            lvc.pszText = (WCHAR*)L"修改时间";
            lvc.cx = 120;
            ListView_SetColumn(g_hListView, 3, &lvc);
        }
        
        // 第五列：类型
        if (columnCount < 5)
        {
            lvc.iSubItem = 4;
            lvc.pszText = (WCHAR*)L"类型";
            lvc.cx = 80;
            ListView_InsertColumn(g_hListView, 4, &lvc);
        }
        else
        {
            lvc.iSubItem = 4;
            lvc.pszText = (WCHAR*)L"类型";
            lvc.cx = 80;
            ListView_SetColumn(g_hListView, 4, &lvc);
        }
        
        LogToFile("UpdateListViewColumns: 已更新为文件模式列标题（文件名 | 路径 | 大小 | 修改时间 | 类型）");
    }
    else
    {
        // 普通模式：名称 | 路径
        lvc.iSubItem = 0;
        lvc.pszText = (WCHAR*)L"名称";
        lvc.cx = 180;
        ListView_SetColumn(g_hListView, 0, &lvc);
        
        // 确保第二列存在
        int columnCount = Header_GetItemCount(ListView_GetHeader(g_hListView));
        if (columnCount < 2)
        {
            // 如果第二列不存在，创建它
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 180;
            ListView_InsertColumn(g_hListView, 1, &lvc);
        }
        else
        {
            // 如果第二列已存在，更新它
            lvc.iSubItem = 1;
            lvc.pszText = (WCHAR*)L"路径";
            lvc.cx = 180;
            ListView_SetColumn(g_hListView, 1, &lvc);
        }
        
        LogToFile("UpdateListViewColumns: 已更新为普通模式列标题（名称 | 路径）");
    }
}

// 在ListView第一行添加提示信息（单行）
void AddHintRowToListView(const WCHAR* hintText)
{
    if (!g_hListView || !IsWindow(g_hListView) || !hintText)
    {
        return;
    }
    
    // 检查是否已经有提示行（第一行）
    int itemCount = ListView_GetItemCount(g_hListView);
    if (itemCount > 0)
    {
        // 检查第一行是否是提示行（通过检查文本是否包含提示标识）
        WCHAR firstItemText[1024] = {0};
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = firstItemText;
        lvi.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvi))
        {
            // 如果第一行已经是提示行，更新它
            if (wcsstr(firstItemText, L"提示:") == firstItemText || wcsstr(firstItemText, L"💡") == firstItemText)
            {
                lvi.pszText = const_cast<LPWSTR>(hintText);
                ListView_SetItem(g_hListView, &lvi);
                return;
            }
        }
    }
    
    // 插入新的提示行到第一行
    LVITEMW lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;  // 插入到第一行
    lvi.iSubItem = 0;
    lvi.pszText = const_cast<LPWSTR>(hintText);
    ListView_InsertItem(g_hListView, &lvi);
    
    // 如果是多列模式，设置第二列为空
    int columnCount = Header_GetItemCount(ListView_GetHeader(g_hListView));
    if (columnCount > 1)
    {
        lvi.iSubItem = 1;
        lvi.pszText = (WCHAR*)L"";
        ListView_SetItem(g_hListView, &lvi);
    }
}

// 在ListView前面添加多行提示信息
void AddMultiLineHintsToListView(const WCHAR* hints[], int hintCount)
{
    if (!g_hListView || !IsWindow(g_hListView) || !hints || hintCount <= 0)
    {
        return;
    }
    
    // 检查是否已经有提示行
    int itemCount = ListView_GetItemCount(g_hListView);
    bool hasHints = false;
    if (itemCount > 0)
    {
        WCHAR firstItemText[1024] = {0};
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = firstItemText;
        lvi.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvi))
        {
            if (wcsstr(firstItemText, L"提示:") == firstItemText || wcsstr(firstItemText, L"💡") == firstItemText)
            {
                hasHints = true;
            }
        }
    }
    
    // 如果已有提示行，删除所有提示行
    if (hasHints)
    {
        // 删除所有提示行（从后往前删除，避免索引变化）
        for (int i = itemCount - 1; i >= 0; i--)
        {
            WCHAR itemText[1024] = {0};
            LVITEMW lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = itemText;
            lvi.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
            if (ListView_GetItem(g_hListView, &lvi))
            {
                if (wcsstr(itemText, L"提示:") == itemText || wcsstr(itemText, L"💡") == itemText)
                {
                    ListView_DeleteItem(g_hListView, i);
                }
                else
                {
                    break;  // 遇到非提示行，停止删除
                }
            }
        }
    }
    
    // 插入新的提示行到前面
    for (int i = 0; i < hintCount; i++)
    {
        LVITEMW lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;  // 插入到第i行
        lvi.iSubItem = 0;
        lvi.pszText = const_cast<LPWSTR>(hints[i]);
        ListView_InsertItem(g_hListView, &lvi);
    }
}

// 获取ListView前面提示行的数量
int GetHintRowCount()
{
    if (!g_hListView || !IsWindow(g_hListView))
    {
        return 0;
    }
    
    int hintRowCount = 0;
    int itemCount = ListView_GetItemCount(g_hListView);
    for (int i = 0; i < itemCount; i++)
    {
        WCHAR itemText[1024] = {0};
        LVITEMW lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;
        lvItem.pszText = itemText;
        lvItem.cchTextMax = sizeof(itemText) / sizeof(WCHAR);
        if (ListView_GetItem(g_hListView, &lvItem))
        {
            // 检查是否是提示行
            if (wcsstr(itemText, L"提示:") == itemText || wcsstr(itemText, L"💡") == itemText)
            {
                hintRowCount++;
            }
            else
            {
                break;  // 遇到非提示行，停止计数
            }
        }
    }
    
    return hintRowCount;
}

// 获取第一个实际项目（跳过提示行）的索引
INT_PTR GetFirstActualItemIndex()
{
    int hintRowCount = GetHintRowCount();
    int itemCount = ListView_GetItemCount(g_hListView);
    
    if (hintRowCount >= itemCount)
    {
        return -1;  // 只有提示行，没有实际项目
    }
    
    return hintRowCount;  // 第一个实际项目的索引
}

// HTML模板读取辅助函数实现
std::wstring ReadHtmlTemplate(const std::wstring& filePath)
{
    std::vector<std::wstring> pathsToCheck;
    pathsToCheck.push_back(filePath);
    pathsToCheck.push_back(L"bin\\" + filePath);
    
    // Get module directory
    WCHAR modulePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, modulePath, MAX_PATH)) {
        std::wstring moduleDir = modulePath;
        size_t lastBackslash = moduleDir.find_last_of(L"\\");
        if (lastBackslash != std::wstring::npos) {
            moduleDir = moduleDir.substr(0, lastBackslash);
            pathsToCheck.push_back(moduleDir + L"\\" + filePath);
            
            // Also check parent of module dir (if running from bin)
            size_t parentBackslash = moduleDir.find_last_of(L"\\");
            if (parentBackslash != std::wstring::npos) {
                std::wstring parentDir = moduleDir.substr(0, parentBackslash);
                pathsToCheck.push_back(parentDir + L"\\" + filePath);
            }
        }
    }

    std::ifstream file;
    std::wstring foundPath;
    
    for (const auto& path : pathsToCheck) {
        file.open(path, std::ios::binary);
        if (file.is_open()) {
            foundPath = path;
            break;
        }
        file.close();
    }

    if (!file.is_open())
    {
        std::string errorMsg = "ReadHtmlTemplate: 无法打开HTML模板文件: " + std::string(filePath.begin(), filePath.end());
        LogToFile(errorMsg.c_str());
        return L"";
    }

    // Read file content
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string content(size, ' ');
    file.seekg(0);
    file.read(&content[0], size);
    file.close();

    // Convert to wstring (simple conversion, assuming UTF-8 or ASCII)
    // For proper UTF-8 handling, we should use MultiByteToWideChar
    int wlen = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, NULL, 0);
    if (wlen > 0)
    {
        std::wstring wcontent(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, content.c_str(), -1, &wcontent[0], wlen);
        // Remove null terminator if included
        if (!wcontent.empty() && wcontent.back() == 0)
            wcontent.pop_back();
        return wcontent;
    }

    return L"";
}

void ReplaceStringInPlace(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::wstring::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
