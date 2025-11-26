#include "command_handler.h"
#include <iostream>
#include <fstream>
#include <cctype>
#include <windows.h>
#include <shellapi.h>

CommandHandler::CommandHandler() {
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::setCommonDirectories(const std::vector<std::string>& dirs) {
    commonDirectories = dirs;
}

const std::vector<std::string>& CommandHandler::getCommonDirectories() const {
    return commonDirectories;
}

bool CommandHandler::isUrl(const std::string& input) {
    if (input.find("http://") == 0 || input.find("https://") == 0 || input.find("www.") == 0) {
        return true;
    }
    return false;
}

bool CommandHandler::isImageFile(const std::string& input) {
    size_t dotPos = input.find_last_of(".");
    if (dotPos == std::string::npos) return false;
    
    std::string ext = input.substr(dotPos + 1);
    for (size_t i = 0; i < ext.size(); i++) {
        ext[i] = tolower(ext[i]);
    }
    
    return ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "tiff" || ext == "webp";
}

bool CommandHandler::isDirectory(const std::string& input) {
    DWORD fileAttributes = GetFileAttributesA(input.c_str());
    return (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool CommandHandler::openDirectory(const std::string& path) {
    std::string cmd = "explorer.exe \"" + path + "\"";
    return system(cmd.c_str()) == 0;
}

bool CommandHandler::previewImage(const std::string& path) {
    std::ifstream file(path);
    if (!file.good()) return false;
    file.close();
    
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    std::cout << "Image opened, press Enter to continue" << std::endl;
    std::cin.ignore();
    return true;
}

bool CommandHandler::openUrl(const std::string& url) {
    std::string fullUrl = url;
    
    if (fullUrl.find("http://") != 0 && fullUrl.find("https://") != 0 && fullUrl.find("www.") == 0) {
        fullUrl = "http://" + fullUrl;
    }
    
    ShellExecuteA(NULL, "open", fullUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return true;
}

bool CommandHandler::findCommonDirectory(const std::string& keyword) {
    std::vector<std::string> matches;
    
    for (size_t i = 0; i < commonDirectories.size(); i++) {
        const std::string& dir = commonDirectories[i];
        std::string lowerDir = dir;
        std::string lowerKey = keyword;
        
        for (size_t j = 0; j < lowerDir.size(); j++) {
            lowerDir[j] = tolower(lowerDir[j]);
        }
        for (size_t j = 0; j < lowerKey.size(); j++) {
            lowerKey[j] = tolower(lowerKey[j]);
        }
        
        if (lowerDir.find(lowerKey) != std::string::npos) {
            matches.push_back(dir);
        }
    }
    
    if (matches.empty()) return false;
    
    if (matches.size() == 1) {
        return openDirectory(matches[0]);
    }
    
    std::cout << "Found directories:" << std::endl;
    for (size_t i = 0; i < matches.size(); i++) {
        std::cout << "[" << (i+1) << "] " << matches[i] << std::endl;
    }
    
    std::cout << "Select directory (0 to cancel): ";
    int choice;
    std::cin >> choice;
    
    if (choice > 0 && choice <= (int)matches.size()) {
        return openDirectory(matches[choice-1]);
    }
    
    return false;
}

bool CommandHandler::handleCommand(const std::string& command) {
    if (command.empty()) return false;
    
    if (isUrl(command)) {
        return openUrl(command);
    }
    
    if (isImageFile(command)) {
        std::ifstream file(command);
        if (file.good()) {
            file.close();
            return previewImage(command);
        }
    }
    
    if (isDirectory(command)) {
        return openDirectory(command);
    }
    
    if (findCommonDirectory(command)) {
        return true;
    }
    
    std::ifstream file(command);
    if (file.good()) {
        file.close();
        ShellExecuteA(NULL, "open", command.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }
    
    return false;
}