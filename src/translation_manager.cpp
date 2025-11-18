#include "translation_manager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <codecvt>

// 初始化单例实例
TranslationManager* TranslationManager::instance = nullptr;

// 私有构造函数
TranslationManager::TranslationManager() : currentLanguage(L"zh_CN") {
}

// 获取单例实例
TranslationManager* TranslationManager::getInstance() {
    if (!instance) {
        instance = new TranslationManager();
    }
    return instance;
}

// 初始化翻译管理器
bool TranslationManager::initialize(const std::wstring& languageCode) {
    return switchLanguage(languageCode);
}

// 切换语言
bool TranslationManager::switchLanguage(const std::wstring& languageCode) {
    // 清空现有翻译
    translations.clear();
    currentLanguage = languageCode;
    
    // 构建PO文件路径
    std::wstring poFilePath = L"data/translations/" + languageCode + L".po";
    
    // 尝试加载PO文件
    if (loadPOFile(poFilePath)) {
        return true;
    }
    
    // 如果找不到指定语言的PO文件，尝试加载默认语言
    if (languageCode != L"zh_CN") {
        return switchLanguage(L"zh_CN");
    }
    
    return false;
}

// 从PO文件加载翻译
bool TranslationManager::loadPOFile(const std::wstring& filePath) {
    // 检查文件是否存在
    std::filesystem::path path(filePath);
    if (!std::filesystem::exists(path)) {
        return false;
    }
    
    // 读取文件内容
    std::wifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // 设置UTF-8编码
    // file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
        // 使用更现代的方法处理UTF-8编码
        file.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::consume_header>));
    
    // 读取整个文件
    std::wstringstream contentStream;
    contentStream << file.rdbuf();
    file.close();
    
    // 解析PO文件
    return parsePOFile(contentStream.str());
}

// 解析PO文件中的msgid和msgstr
bool TranslationManager::parsePOFile(const std::wstring& content) {
    std::wstring currentMsgid;
    std::wstring currentMsgstr;
    bool inMsgid = false;
    bool inMsgstr = false;
    
    std::wstringstream ss(content);
    std::wstring line;
    
    while (std::getline(ss, line)) {
        // 跳过注释行
        if (line.empty() || line[0] == L'#') {
            continue;
        }
        
        // 处理msgid行
        if (line.find(L"msgid ") == 0) {
            inMsgid = true;
            inMsgstr = false;
            
            // 提取msgid内容，去掉引号
            size_t start = line.find(L'"') + 1;
            size_t end = line.rfind(L'"');
            if (start < end) {
                currentMsgid = line.substr(start, end - start);
            } else {
                currentMsgid = L"";
            }
        }
        // 处理msgstr行
        else if (line.find(L"msgstr ") == 0) {
            inMsgid = false;
            inMsgstr = true;
            
            // 提取msgstr内容，去掉引号
            size_t start = line.find(L'"') + 1;
            size_t end = line.rfind(L'"');
            if (start < end) {
                currentMsgstr = line.substr(start, end - start);
            } else {
                currentMsgstr = L"";
            }
            
            // 将翻译对添加到映射表
            if (!currentMsgid.empty()) {
                translations[currentMsgid] = currentMsgstr;
            }
        }
        // 处理多行字符串（续行）
        else if (line[0] == L'"' && line.back() == L'"') {
            size_t start = 1;
            size_t end = line.size() - 1;
            std::wstring content = line.substr(start, end - start);
            
            if (inMsgid) {
                currentMsgid += content;
            } else if (inMsgstr) {
                currentMsgstr += content;
                // 更新翻译对
                if (!currentMsgid.empty()) {
                    translations[currentMsgid] = currentMsgstr;
                }
            }
        }
    }
    
    return !translations.empty();
}

// 翻译文本（类似Godot的TTR功能）
std::wstring TranslationManager::translate(const std::wstring& sourceText) {
    // 查找翻译
    auto it = translations.find(sourceText);
    if (it != translations.end()) {
        // 如果找到翻译且翻译不为空，则返回翻译
        if (!it->second.empty()) {
            return it->second;
        }
    }
    
    // 如果没有找到翻译或翻译为空，返回原始文本
    return sourceText;
}

// 获取当前语言
std::wstring TranslationManager::getCurrentLanguage() const {
    return currentLanguage;
}

// 获取支持的语言列表
std::vector<std::wstring> TranslationManager::getSupportedLanguages() {
    std::vector<std::wstring> languages;
    
    try {
        // 检查翻译目录是否存在
        std::filesystem::path translationsDir(L"data/translations");
        if (!std::filesystem::exists(translationsDir) || !std::filesystem::is_directory(translationsDir)) {
            // 添加默认语言
            languages.push_back(L"zh_CN");
            languages.push_back(L"en_US");
            return languages;
        }
        
        // 遍历翻译目录中的PO文件
        for (const auto& entry : std::filesystem::directory_iterator(translationsDir)) {
            if (entry.is_regular_file() && entry.path().extension() == L".po") {
                std::wstring languageCode = entry.path().stem().wstring();
                languages.push_back(languageCode);
            }
        }
        
        // 如果没有找到PO文件，添加默认语言
        if (languages.empty()) {
            languages.push_back(L"zh_CN");
            languages.push_back(L"en_US");
        }
    } catch (...) {
        // 发生异常时，返回默认语言
        languages.push_back(L"zh_CN");
        languages.push_back(L"en_US");
    }
    
    return languages;
}

// 释放资源
void TranslationManager::release() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}