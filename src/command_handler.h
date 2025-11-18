#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <string>
#include <vector>

class CommandHandler {
private:
    std::vector<std::string> commonDirectories;
    
    bool isUrl(const std::string& input);
    bool isImageFile(const std::string& input);
    bool isDirectory(const std::string& input);
    bool openDirectory(const std::string& path);
    bool previewImage(const std::string& path);
    bool openUrl(const std::string& url);
    bool findCommonDirectory(const std::string& keyword);

public:
    CommandHandler();
    ~CommandHandler();
    
    void setCommonDirectories(const std::vector<std::string>& dirs);
    const std::vector<std::string>& getCommonDirectories() const;
    bool handleCommand(const std::string& command);
};

#endif