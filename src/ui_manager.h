#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <string>

class UIManager {
private:
    void clearScreen();
    void setColor(int color);

public:
    UIManager();
    ~UIManager();
    
    void displayPrompt();
    std::string getUserInput();
    void displayError(const std::string& message);
    void displaySuccess(const std::string& message);
    void displayHelp();
};

// UI管理函数声明
void UpdateListViewColumns();  // 根据当前模式更新ListView列标题
void AddMultiLineHintsToListView(const WCHAR* hints[], int hintCount);  // 在ListView前面添加多行提示信息

#endif