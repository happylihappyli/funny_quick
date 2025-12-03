// 消息处理函数实现文件
// 用于重构WindowProc函数，将消息处理逻辑分离到独立的函数

#include "message_handlers.h"
#include "common.h"
#include "calculator.h"  // 计算器功能定义
#include "logger.h"
#include "ui_manager.h"
#include "resource.h"
#include "file_manager.h"  // 文件管理功能定义
#include <string>
#include <commctrl.h>  // 包含列表视图控件相关定义

// 处理退出计算模式按钮点击sc
void HandleExitCalculatorButton(HWND hwnd)
{
    if (g_calculatorMode)
    {
        ExitCalculatorMode();
        LogToFile("WM_COMMAND: 用户点击退出计算模式按钮");
    }
}



// 处理计算模式操作菜单按钮点击
void HandleCalculatorMenuButton(HWND hwnd)
{
    if (g_calculatorMode)
    {
        LogToFile("WM_COMMAND: 用户点击计算模式操作菜单按钮");
        
        // 获取按钮位置
        RECT buttonRect;
        GetWindowRect(g_hCalcMenuButton, &buttonRect);
        
        // 创建下拉菜单
        HMENU hMenu = CreatePopupMenu();
        
        // 添加菜单项
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_COPY, L"复制选中项");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_DELETE_ITEM, L"删除选中项");
        AppendMenuW(hMenu, MF_STRING, ID_CONTEXT_CLEAR_ALL, L"清空历史记录");
        
        // 显示菜单（在按钮下方）
        int command = TrackPopupMenu(hMenu, 
            TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY,
            buttonRect.left, buttonRect.bottom, 0, hwnd, NULL);
        
        // 销毁菜单
        DestroyMenu(hMenu);
        
        // 处理用户选择
        if (command == ID_CONTEXT_COPY)
        {
            CopySelectedListItem();
            LogToFile("操作菜单: 复制了选中的项目");
        }
        else if (command == ID_CONTEXT_DELETE_ITEM)
        {
            // 删除选中的计算结果
            if (g_calculationHistory.empty())
            {
                LogToFile("操作菜单: 历史记录为空，无法删除");
                MessageBoxW(hwnd, L"历史记录为空，没有可删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                return;
            }
            
            // 获取ListView中选中的项目
            INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
            if (selIndex == -1)
            {
                // 如果没有选中项，尝试获取焦点项
                selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
            }
            
            if (selIndex < 0 || selIndex >= (INT_PTR)g_calculationHistory.size())
            {
                MessageBoxW(hwnd, L"请先选择要删除的项目", L"提示", MB_OK | MB_ICONINFORMATION);
                return;
            }
            
            // 转换ListView索引到实际历史记录索引
            size_t actualIndex = g_calculationHistory.size() - 1 - selIndex;
            
            if (actualIndex >= g_calculationHistory.size())
            {
                MessageBoxW(hwnd, L"索引转换错误，无法删除", L"错误", MB_OK | MB_ICONERROR);
                return;
            }
            
            // 从历史记录中删除
            g_calculationHistory.erase(g_calculationHistory.begin() + actualIndex);
            
            // 保存到文件
            SaveCalculationHistory();
            
            // 重新显示历史记录
            DisplayCalculationHistory();
            
            // 更新WebView2显示
            UpdateCalculatorModeWebView();
            
            LogToFile("操作菜单: 删除了选中的计算结果");
        }
        else if (command == ID_CONTEXT_CLEAR_ALL)
        {
            // 清空所有历史记录
            if (MessageBoxW(hwnd, L"确定要清空所有计算历史吗？", 
                L"确认", MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                g_calculationHistory.clear();
                SaveCalculationHistory();
                DisplayCalculationHistory();
                
                // 更新WebView2显示
                UpdateCalculatorModeWebView();
                
                LogToFile("操作菜单: 清空了所有计算历史");
            }
        }
    }
}

// 处理设置菜单命令
void HandleSettingsMenuCommands(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case ID_SETTINGS_EXIT:
            // 退出程序
            LogToFile("WM_COMMAND: 用户选择设置菜单-退出程序");
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
    }
}

// 处理托盘菜单命令
void HandleTrayMenuCommands(HWND hwnd, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case ID_TRAY_SHOW:
            ShowLauncherWindow();
            LogToFile("WM_COMMAND: 用户选择显示窗口");
            break;
            
        case ID_TRAY_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            LogToFile("WM_COMMAND: 用户选择退出");
            break;
    }
}



// 处理编辑控件EN_CHANGE通知
void HandleEditControlChange(HWND hwnd)
{
    // Log edit control EN_CHANGE notification
    LogToFile("  Edit control EN_CHANGE notification - processing search");
    
    // Real-time search
    WCHAR searchText[1024] = {0};
    GetWindowTextW(g_hEdit, searchText, sizeof(searchText)/sizeof(WCHAR));
    
    // 记录输入的字符到日志
    {
        char inputCharLog[1024] = {0};
        WideCharToMultiByte(CP_UTF8, 0, searchText, -1, inputCharLog, sizeof(inputCharLog), NULL, NULL);
        char changeLog[1100] = {0};
        sprintf(changeLog, "  EN_CHANGE: 输入字符: '%s'", inputCharLog);
        LogToFile(changeLog);
    }
    
    wcscpy(g_currentSearch, searchText);
    
    // 检查是否在计算模式
    if (g_calculatorMode)
    {
        // 在计算模式下，不进行搜索，也不实时计算，只记录输入变化
        LogToFile("  EN_CHANGE: 计算模式下，输入内容已变化，但不计算");
    }
    else if (g_fileMode)
    {
        // 文件模式下，设置500ms延迟搜索
        char logMsg[512] = {0};
        char searchTextLog[256] = {0};
        WideCharToMultiByte(CP_UTF8, 0, searchText, -1, searchTextLog, sizeof(searchTextLog), NULL, NULL);
        sprintf(logMsg, "  EN_CHANGE: 文件模式下，输入内容: '%s'，设置500ms延迟搜索", searchTextLog);
        LogToFile(logMsg);
        
        // 检查输入内容是否有效
        if (searchText == NULL) {
            LogToFile("  EN_CHANGE: 错误：searchText 为 NULL");
            return;
        }
        
        // 检查字符串长度
        size_t textLen = wcslen(searchText);
        sprintf(logMsg, "  EN_CHANGE: 输入字符串长度: %zu", textLen);
        LogToFile(logMsg);
        
        // 如果已有定时器在运行，先取消
        if (g_fileSearchTimerId != 0)
        {
            KillTimer(g_hMainWindow, g_fileSearchTimerId);
            g_fileSearchTimerId = 0;
            LogToFile("  EN_CHANGE: 取消之前的文件搜索定时器");
        }
        
        // 保存待处理的搜索查询
        wcscpy_s(g_pendingFileSearchQuery, sizeof(g_pendingFileSearchQuery)/sizeof(WCHAR), searchText);
        
        // 设置500ms定时器
        g_fileSearchTimerId = SetTimer(g_hMainWindow, 2, 500, NULL);  // 定时器ID设为2，避免与现有定时器冲突
        
        if (g_fileSearchTimerId == 0)
        {
            LogToFile("  EN_CHANGE: 错误：无法设置文件搜索定时器");
            // 定时器设置失败，立即搜索
            SearchFiles(searchText);
        }
        else
        {
            sprintf(logMsg, "  EN_CHANGE: 文件搜索定时器已设置，ID: %Id", g_fileSearchTimerId);
            LogToFile(logMsg);
        }
    }
    else
    {
        // 在搜索模式下，但需要检查是否是完整的特殊命令
        // 只有当输入不是特殊命令时才进行搜索，避免误解发
        if (wcscmp(searchText, L"js") != 0 && 
            wcscmp(searchText, L"help") != 0 && 
            wcscmp(searchText, L"set") != 0)
        {
            // 在搜索模式下，进行正常搜索
            SearchAndDisplayResults(searchText);
        }
        else
        {
            // 输入为特殊命令，但不进行搜索，只清空搜索结果
            LogToFile("  EN_CHANGE: 检测到特殊命令输入，但不执行搜索，等待回车键");
            SearchAndDisplayResults(L""); // 清空搜索结果
        }
    }
}

// 处理编辑控件EN_RETURN通知
void HandleEditControlReturn(HWND hwnd)
{
    // Log edit control EN_RETURN notification
    LogToFile("  Edit control EN_RETURN notification - processing");
    
    // 打印ListView所有内容用于调试
    LogToFile("  EN_RETURN: 回车键按下，打印ListView内容:");
    LogListViewContents();
    
    // Handle Enter key press in edit control
    // Check if this EN_RETURN is caused by focus change and should be ignored
    if (g_ignoreNextReturn)
    {
        LogToFile("  EN_RETURN: 回车键被忽略 (焦点变化导致)");
        g_ignoreNextReturn = false; // Reset the flag
        return;
    }
    
    LogToFile("  EN_RETURN: 处理用户按下的回车键");
    
    // 获取当前输入框的内容
    WCHAR currentText[1024] = {0};
    GetWindowTextW(g_hEdit, currentText, sizeof(currentText)/sizeof(WCHAR));
    char currentTextLog[1024] = {0};
    WideCharToMultiByte(CP_UTF8, 0, currentText, -1, currentTextLog, sizeof(currentTextLog), NULL, NULL);
    char enterLog[1100] = {0};
    sprintf(enterLog, "  EN_RETURN: 当前输入框内容: '%s'", currentTextLog);
    LogToFile(enterLog);
    
    // 检查是否在计算模式
    if (g_calculatorMode)
    {
        // 在计算模式下，计算表达式
        LogToFile("  EN_RETURN: 计算模式下，计算表达式");
        EvaluateExpression(currentText);
    }
    else
    {
        // 在搜索模式下，执行正常的搜索结果
        // Handle return key - Ensure it executes the first item
        {
            int itemCount = ListView_GetItemCount(g_hListView);
            char logMsg[200] = {0};
            sprintf(logMsg, "  EN_RETURN: 列表框项目数量: %d", itemCount);
            LogToFile(logMsg);
        
            // 检查输入内容是否是特殊命令 - 优先处理命令
            if (wcscmp(currentText, L"js") == 0)
            {
                LogToFile("  EN_RETURN: 识别为'js'命令，调用ProcessCommand");
                ProcessCommand(currentText);
                return;
            }
            else if (wcscmp(currentText, L"help") == 0)
            {
                LogToFile("  EN_RETURN: 识别为'help'命令，显示使用帮助");
                ShowHelpInfo();
                return;
            }
            else if (wcscmp(currentText, L"set") == 0)
            {
                LogToFile("  EN_RETURN: 识别为'set'命令，显示设置菜单");
                ShowSettingsMenu();
                return;
            }
            else if (itemCount > 0)
            {
                // 获取第一个实际项目（跳过提示行）
                INT_PTR firstSelIndex = GetFirstActualItemIndex();
                if (firstSelIndex == -1)
                {
                    LogToFile("  EN_RETURN: 只有提示行，没有实际项目");
                    return;
                }
                
                // Force select the first actual item to ensure it's highlighted
                ListView_SetItemState(g_hListView, firstSelIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                LogToFile("  EN_RETURN: 强制选择第一个实际项目（跳过提示行）");
                
                // 获取第一个实际项目的文本
                WCHAR firstItemText[1024] = {0};
                LVITEMW lvItem = {0};
                lvItem.iItem = (int)firstSelIndex;
                lvItem.iSubItem = 0;
                lvItem.pszText = firstItemText;
                lvItem.cchTextMax = sizeof(firstItemText) / sizeof(WCHAR);
                ListView_GetItem(g_hListView, &lvItem);
                char firstItemLog[1024] = {0};
                WideCharToMultiByte(CP_UTF8, 0, firstItemText, -1, firstItemLog, sizeof(firstItemLog), NULL, NULL);
                sprintf(logMsg, "  EN_RETURN: 第一个实际项目文本: '%s'", firstItemLog);
                LogToFile(logMsg);
                
                // 检查是否是收藏的网址
                bool isBookmark = (wcsstr(firstItemText, L"收藏:") == firstItemText);
                if (isBookmark)
                {
                    LogToFile("  EN_RETURN: 识别为收藏的网址，直接打开");
                    // 获取收藏的网址名称
                    std::wstring bookmarkName = firstItemText + 4; // 跳过"收藏:"前缀
                    
                    // 在收藏中查找对应的网址
                    for (size_t i = 0; i < g_bookmarks.size(); i++)
                    {
                        if (g_bookmarks[i].first == bookmarkName)
                        {
                            // 直接打开收藏的网址
                            ShellExecuteW(NULL, L"open", g_bookmarks[i].second.c_str(), NULL, NULL, SW_SHOWNORMAL);
                            LogToFile("  EN_RETURN: 成功打开收藏的网址");
                            break;
                        }
                    }
                }
                else
                {
                    // Verify g_searchResults has items before executing
                    // Also check if the first item is not the "No matching items found" message
                    if (!g_searchResults.empty() && g_searchResults.size() > 0)
                    {
                        LogToFile("  EN_RETURN: 搜索结果不为空，执行第一个项目");
                        ExecuteSelectedItem(firstSelIndex);
                    }
                    else
                    {
                        // Check if the first item is "No matching items found"
                        if (wcscmp(firstItemText, L"No matching items found") == 0)
                        {
                            LogToFile("  EN_RETURN: 第一个项目是'未找到匹配项'消息，不执行");
                        }
                        else
                        {
                            LogToFile("  EN_RETURN: 错误：搜索结果为空但列表框有实际项目");
                        }
                    }
                }
            }
            else
            {
                // If no items, process as command
                if (wcslen(currentText) > 0)
                {
                    sprintf(logMsg, "  EN_RETURN: 列表为空，将输入内容作为命令处理: '%s'", currentTextLog);
                    LogToFile(logMsg);
                    ProcessCommand(currentText);
                }
                else
                {
                    LogToFile("  EN_RETURN: 列表为空且输入内容为空，不执行任何操作");
                }
            }
        }
    }
}

// 处理列表视图双击事件
void HandleListViewDoubleClick(HWND hwnd)
{
    // Only handle double click - explicitly ignore all other listbox notifications
    // This prevents auto-opening on selection change or focus change
    INT_PTR selIndex = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
    if (selIndex != -1)
    {
        if (g_settingsMenuMode)
        {
            if (selIndex == 0)
            {
                LogToFile("WM_COMMAND: 设置菜单模式下双击提示行，忽略");
                return;
            }
            
            LogToFile("WM_COMMAND: 检测到设置菜单模式，调用菜单项处理函数");
            HandleSettingsMenuItemClick(selIndex);
        }
        else
        {
            // 正常模式，执行选中的项目
            LogToFile("WM_COMMAND: 正常模式，执行选中的项目");
            ExecuteSelectedItem(selIndex);
        }
    }
}

// WM_COMMAND消息处理函数
LRESULT HandleWMCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // Log WM_COMMAND message details for debugging
    char logMsg[200] = {0};
    sprintf(logMsg, "WM_COMMAND received: Control ID=%d, Notification=%d", LOWORD(wParam), HIWORD(wParam));
    LogToFile(logMsg);
    
    // 处理退出计算模式按钮点击
    if (LOWORD(wParam) == IDC_EXIT_CALC_BUTTON)
    {
        HandleExitCalculatorButton(hwnd);
        return 0;
    }
    // 处理退出文件模式按钮点击
    else if (LOWORD(wParam) == IDC_EXIT_FILE_BUTTON)
    {
        HandleExitFileButton(hwnd);
        return 0;
    }
    // 设置按钮已移除，不再处理设置按钮点击事件
    // 处理退出网址收藏模式按钮点击（已禁用，只允许"q"退出）
    else if (LOWORD(wParam) == IDC_EXIT_BOOKMARK_BUTTON)
    {
        HandleExitBookmarkButton(hwnd);
        return 0;
    }
    // 处理计算模式操作菜单按钮点击
    else if (LOWORD(wParam) == IDC_CALC_MENU_BUTTON)
    {
        HandleCalculatorMenuButton(hwnd);
        return 0;
    }
    // 处理设置菜单命令
    else if (LOWORD(wParam) == ID_SETTINGS_BOOKMARK || 
             LOWORD(wParam) == ID_SETTINGS_EXIT)
    {
        HandleSettingsMenuCommands(hwnd, wParam);
        return 0;
    }
    // 处理托盘菜单命令
    else if (LOWORD(wParam) == ID_TRAY_SHOW || 
             LOWORD(wParam) == ID_TRAY_EXIT)
    {
        HandleTrayMenuCommands(hwnd, wParam);
        return 0;
    }
    // 处理网址收藏模式右键菜单命令
    else if (LOWORD(wParam) == ID_CONTEXT_DELETE_BOOKMARK || 
             LOWORD(wParam) == ID_CONTEXT_SYNC_CHROME)
    {
        HandleBookmarkContextMenuCommands(hwnd, wParam);
        return 0;
    }
    
    if (LOWORD(wParam) == IDC_EDIT)
    {
        switch (HIWORD(wParam))
        {
            case EN_CHANGE:
                HandleEditControlChange(hwnd);
                break;
                
            case EN_RETURN:
                HandleEditControlReturn(hwnd);
                break;
                
            default:
                // Log other edit control notifications
                {
                    char logMsg[200] = {0};
                    sprintf(logMsg, "  Edit control unknown notification: %d", HIWORD(wParam));
                    LogToFile(logMsg);
                }
                break;
        }
    }
    else if (LOWORD(wParam) == IDC_LISTVIEW)
    {
        // Only handle double click - explicitly ignore all other listbox notifications
        // This prevents auto-opening on selection change or focus change
        if (HIWORD(wParam) == LBN_DBLCLK)
        {
            HandleListViewDoubleClick(hwnd);
        }
        // Explicitly ignore LBN_SELCHANGE to prevent auto-open on selection
        // Also ignore LBN_SETFOCUS and other notifications
    }
    
    return 0;
}

// WM_KEYDOWN消息处理函数
LRESULT HandleWMKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // Log WM_KEYDOWN message details for debugging
    char logMsg[200] = {0};
    sprintf(logMsg, "WM_KEYDOWN received: Virtual key code=%d, Repeat count=%d, Scan code=%d, Extended key=%d, Context code=%d, Previous state=%d, Transition state=%d", 
            (int)wParam, 
            (int)(lParam & 0xFFFF), 
            (int)((lParam >> 16) & 0xFF), 
            (int)((lParam >> 24) & 0x1), 
            (int)((lParam >> 29) & 0x1), 
            (int)((lParam >> 30) & 0x1), 
            (int)((lParam >> 31) & 0x1));
    LogToFile(logMsg);
    
    // 处理特殊按键
    switch (wParam)
    {
        case VK_ESCAPE:
            // ESC键：隐藏窗口
            LogToFile("WM_KEYDOWN: ESC键按下，隐藏窗口");
            HideLauncherWindow();
            return 0;
            
        case 'Q':
        case 'q':
            // Q键：退出特殊模式
            if (g_calculatorMode)
            {
                LogToFile("WM_KEYDOWN: Q键按下，退出计算模式");
                ExitCalculatorMode();
                return 0;
            }
            else if (g_bookmarkMode)
            {
                LogToFile("WM_KEYDOWN: Q键按下，退出网址收藏模式");
                ExitBookmarkMode();
                return 0;
            }
            else if (g_fileMode)
            {
                LogToFile("WM_KEYDOWN: Q键按下，退出文件模式");
                ExitFileMode();
                return 0;
            }
            break;
            
        case VK_UP:
        {
            // 上箭头键：在列表框中向上移动选择
            if (g_hListView && IsWindow(g_hListView))
            {
                int itemCount = ListView_GetItemCount(g_hListView);
                if (itemCount > 0)
                {
                    // 获取当前选中的项目
                    INT_PTR currentSel = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                    if (currentSel == -1)
                    {
                        // 如果没有选中项，选择第一个实际项目
                        INT_PTR firstActual = GetFirstActualItemIndex();
                        if (firstActual != -1)
                        {
                            ListView_SetItemState(g_hListView, firstActual, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            LogToFile("WM_KEYDOWN: 上箭头键，选择第一个实际项目");
                        }
                    }
                    else
                    {
                        // 向上移动选择
                        INT_PTR newSel = currentSel - 1;
                        if (newSel >= 0)
                        {
                            // 跳过提示行
                            INT_PTR hintCount = GetHintRowCount();
                            if (newSel < hintCount)
                            {
                                newSel = hintCount;
                            }
                            
                            ListView_SetItemState(g_hListView, newSel, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            LogToFile("WM_KEYDOWN: 上箭头键，向上移动选择");
                        }
                    }
                }
            }
            return 0;
        }
        
        case VK_DOWN:
        {
            // 下箭头键：在列表框中向下移动选择
            if (g_hListView && IsWindow(g_hListView))
            {
                int itemCount = ListView_GetItemCount(g_hListView);
                if (itemCount > 0)
                {
                    // 获取当前选中的项目
                    INT_PTR currentSel = ListView_GetNextItem(g_hListView, -1, LVNI_FOCUSED);
                    if (currentSel == -1)
                    {
                        // 如果没有选中项，选择第一个实际项目
                        INT_PTR firstActual = GetFirstActualItemIndex();
                        if (firstActual != -1)
                        {
                            ListView_SetItemState(g_hListView, firstActual, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            LogToFile("WM_KEYDOWN: 下箭头键，选择第一个实际项目");
                        }
                    }
                    else
                    {
                        // 向下移动选择
                        INT_PTR newSel = currentSel + 1;
                        if (newSel < itemCount)
                        {
                            ListView_SetItemState(g_hListView, newSel, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            LogToFile("WM_KEYDOWN: 下箭头键，向下移动选择");
                        }
                    }
                }
            }
            return 0;
        }
        
        case VK_TAB:
            // Tab键：在编辑框和列表框之间切换焦点
            if (GetFocus() == g_hEdit)
            {
                // 当前焦点在编辑框，切换到列表框
                if (g_hListView && IsWindow(g_hListView))
                {
                    SetFocus(g_hListView);
                    LogToFile("WM_KEYDOWN: Tab键，焦点从编辑框切换到列表框");
                }
            }
            else
            {
                // 当前焦点在列表框或其他控件，切换到编辑框
                if (g_hEdit && IsWindow(g_hEdit))
                {
                    SetFocus(g_hEdit);
                    LogToFile("WM_KEYDOWN: Tab键，焦点切换到编辑框");
                }
            }
            return 0;
    }
    
    // 对于其他按键，返回默认处理
    return DefWindowProcW(hwnd, WM_KEYDOWN, wParam, lParam);
}

// 处理退出文件模式按钮点击
void HandleExitFileButton(HWND hwnd)
{
    if (g_fileMode)
    {
        ExitFileMode();
        LogToFile("HandleExitFileButton: 用户点击退出文件模式按钮");
    }
}

// 处理退出网址收藏模式按钮点击
void HandleExitBookmarkButton(HWND hwnd)
{
    LogToFile("HandleExitBookmarkButton: 退出网址收藏模式按钮被点击");
    
    // 退出网址收藏模式
    ExitBookmarkMode();
}

// 处理网址收藏模式右键菜单命令
void HandleBookmarkContextMenuCommands(HWND hwnd, WPARAM wParam)
{
    char logMsg[200] = {0};
    sprintf(logMsg, "HandleBookmarkContextMenuCommands: 菜单命令ID=%d", LOWORD(wParam));
    LogToFile(logMsg);
    
    switch (LOWORD(wParam))
    {
        case ID_CONTEXT_DELETE_BOOKMARK:
            // 删除选中的书签
            LogToFile("HandleBookmarkContextMenuCommands: 删除书签命令");
            // TODO: 实现删除书签功能
            break;
            
        case ID_CONTEXT_SYNC_CHROME:
            // 同步Chrome书签
            LogToFile("HandleBookmarkContextMenuCommands: 同步Chrome书签命令");
            // TODO: 实现同步Chrome书签功能
            break;
            
        default:
            LogToFile("HandleBookmarkContextMenuCommands: 未知的菜单命令");
            break;
    }
}