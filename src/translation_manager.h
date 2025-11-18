#ifndef TRANSLATION_MANAGER_H
#define TRANSLATION_MANAGER_H

#include <windows.h>
#include <string>
#include <map>
#include <vector>

// 翻译管理器类，负责加载和管理PO文件翻译
class TranslationManager {
private:
    // 单例实例
    static TranslationManager* instance;
    
    // 当前语言代码
    std::wstring currentLanguage;
    
    // 翻译映射表：原始文本 -> 翻译文本
    std::map<std::wstring, std::wstring> translations;
    
    // 私有的构造函数，确保单例模式
    TranslationManager();
    
    // 从PO文件加载翻译
    bool loadPOFile(const std::wstring& filePath);
    
    // 解析PO文件中的msgid和msgstr
    bool parsePOFile(const std::wstring& content);

public:
    // 获取单例实例
    static TranslationManager* getInstance();
    
    // 初始化翻译管理器
    bool initialize(const std::wstring& languageCode = L"zh_CN");
    
    // 切换语言
    bool switchLanguage(const std::wstring& languageCode);
    
    // 翻译文本（类似Godot的TTR功能）
    std::wstring translate(const std::wstring& sourceText);
    
    // 获取当前语言
    std::wstring getCurrentLanguage() const;
    
    // 获取支持的语言列表
    std::vector<std::wstring> getSupportedLanguages();
    
    // 释放资源
    void release();
};

// 翻译宏，简化使用
#define TTR(text) TranslationManager::getInstance()->translate(text)

#endif // TRANSLATION_MANAGER_H