#include "ui_manager.h"
#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

UIManager::UIManager() {
#ifdef _WIN32
    SetConsoleTitleA("Quick Launch Tool");
#endif
}

UIManager::~UIManager() {
}

void UIManager::clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void UIManager::setColor(int color) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
#endif
}

void UIManager::displayPrompt() {
    setColor(10);
    std::cout << "QuickLaunch > ";
    setColor(7);
}

std::string UIManager::getUserInput() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void UIManager::displayError(const std::string& message) {
    setColor(12);
    std::cout << "Error: " << message << std::endl;
    setColor(7);
}

void UIManager::displaySuccess(const std::string& message) {
    setColor(10);
    std::cout << "Success: " << message << std::endl;
    setColor(7);
}

void UIManager::displayHelp() {
    clearScreen();
    setColor(14);
    std::cout << "Quick Launch Tool - Usage Guide" << std::endl;
    std::cout << "==============================" << std::endl;
    setColor(7);
    std::cout << "Input directory path or keyword to open in Explorer" << std::endl;
    std::cout << "Input image file path to preview image" << std::endl;
    std::cout << "Input URL (http:// or www.) to open in Chrome" << std::endl;
    std::cout << "Input 'exit' or 'quit' to exit program" << std::endl;
    std::cout << "Input 'help' to view this help" << std::endl;
    std::cout << "==============================" << std::endl;
    setColor(10);
    std::cout << "Common directories examples:" << std::endl;
    setColor(7);
    std::cout << "- Users, Program Files, Windows, Downloads" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Press Enter to continue..." << std::endl;
    std::cin.ignore();
}