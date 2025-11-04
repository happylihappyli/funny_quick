#include <iostream>
#include <string>
#include <vector>
#include "command_handler.h"
#include "ui_manager.h"

int main() {
    CommandHandler commandHandler;
    UIManager uiManager;
    
    std::vector<std::string> commonDirs;
    commonDirs.push_back("C:\\Users");
    commonDirs.push_back("C:\\Program Files");
    commonDirs.push_back("C:\\Windows");
    commonDirs.push_back("C:\\Downloads");
    commandHandler.setCommonDirectories(commonDirs);
    
    uiManager.displayHelp();
    
    while (true) {
        uiManager.displayPrompt();
        
        std::string input = uiManager.getUserInput();
        
        if (input == "exit" || input == "quit") {
            break;
        } else if (input == "help") {
            uiManager.displayHelp();
            continue;
        }
        
        if (!commandHandler.handleCommand(input)) {
            uiManager.displayError("Cannot recognize command or path");
        }
    }
    
    return 0;
}