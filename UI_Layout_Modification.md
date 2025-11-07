# UI布局修改说明

## 修改内容

1. **添加了设置按钮**：
   - 在非计算模式下，右上角显示"设置"按钮
   - 点击设置按钮会弹出提示信息："设置功能正在开发中..."

2. **按钮显示逻辑**：
   - 非计算模式：显示"设置"按钮，隐藏"退出计算"按钮
   - 计算模式：显示"退出计算"按钮，隐藏"设置"按钮

3. **按钮位置**：
   - 设置按钮和退出计算按钮都位于窗口右上角
   - 按钮大小：80x25像素
   - 按钮位置：(300, 10)

## 修改的代码

1. **全局变量**：
   ```cpp
   HWND g_hSettingsButton = NULL;   // 设置按钮
   ```

2. **常量定义**：
   ```cpp
   #define IDC_SETTINGS_BUTTON 1004    // 设置按钮ID
   ```

3. **窗口创建代码**：
   ```cpp
   // Create settings button (initially visible in non-calculator mode)
   g_hSettingsButton = CreateWindowExW(
         0,
         L"BUTTON",
         L"设置",
         WS_CHILD | BS_PUSHBUTTON | WS_VISIBLE,
         300, 10, 80, 25,
         hwnd, (HMENU)IDC_SETTINGS_BUTTON,
         g_hInstance, NULL);
   ```

4. **按钮点击处理**：
   ```cpp
   // 处理设置按钮点击
   else if (LOWORD(wParam) == IDC_SETTINGS_BUTTON)
   {
       // 处理设置按钮点击
       LogToFile("WM_COMMAND: 用户点击设置按钮");
       // TODO: 实现设置功能
       MessageBoxW(hwnd, L"设置功能正在开发中...", L"提示", MB_OK | MB_ICONINFORMATION);
       return 0;
   }
   ```

5. **进入计算模式**：
   ```cpp
   // 隐藏设置按钮
   ShowWindow(g_hSettingsButton, SW_HIDE);
   ```

6. **退出计算模式**：
   ```cpp
   // 显示设置按钮
   ShowWindow(g_hSettingsButton, SW_SHOW);
   ```

## 测试方法

1. 启动程序
2. 检查右上角是否显示"设置"按钮
3. 点击"设置"按钮，检查是否弹出提示信息
4. 在输入框中输入"js"并按回车，进入计算模式
5. 检查右上角是否显示"退出计算"按钮，"设置"按钮是否隐藏
6. 点击"退出计算"按钮，检查是否退出计算模式
7. 检查右上角是否重新显示"设置"按钮

## 预期效果

- 非计算模式下不再显示空白区域，而是显示"设置"按钮
- 计算模式下显示"退出计算"按钮，隐藏"设置"按钮
- 按钮切换逻辑正确，界面更加美观