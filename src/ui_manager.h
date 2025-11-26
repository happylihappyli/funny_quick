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

#endif