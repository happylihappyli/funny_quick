#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import re
import codecs

# 确保翻译目录存在
def ensure_dir_exists(path):
    if not os.path.exists(path):
        os.makedirs(path)

# 从internationalization.h提取翻译字符串
def extract_translations(header_file):
    with codecs.open(header_file, 'r', 'utf-8') as f:
        content = f.read()
    
    # 分离中文和英文部分
    chinese_part = re.search(r'#ifdef LANGUAGE_CHINESE(.*?)(?=#else)', content, re.DOTALL)
    english_part = re.search(r'#else(.*?)(?=#endif)', content, re.DOTALL)
    
    if not chinese_part or not english_part:
        print("无法提取中文字符串或英文字符串部分")
        return None, None
    
    # 提取中文翻译
    chinese_translations = {}
    pattern = r'#define\s+(\w+)\s+L"([^"]+)"'
    for match in re.finditer(pattern, chinese_part.group(1)):
        key = match.group(1)
        value = match.group(2)
        chinese_translations[key] = value
    
    # 提取英文翻译
    english_translations = {}
    for match in re.finditer(pattern, english_part.group(1)):
        key = match.group(1)
        value = match.group(2)
        english_translations[key] = value
    
    return chinese_translations, english_translations

# 生成PO文件
def generate_po_file(translations, po_file, is_source=True):
    with codecs.open(po_file, 'w', 'utf-8') as f:
        # 写入PO文件头部
        f.write("# Translation file\n")
        f.write("msgid \"\"\n")
        f.write("msgstr \"\"\n")
        f.write("Content-Type: text/plain; charset=UTF-8\\n\n")
        
        # 写入翻译对
        for key, value in translations.items():
            # 转义特殊字符
            escaped_value = value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
            
            if is_source:
                # 源文件使用英文作为msgid，中文作为msgstr
                f.write(f"msgid \"{escaped_value}\"\n")
                f.write(f"msgstr \"\"\n\n")
            else:
                # 翻译文件使用英文作为msgid，对应语言作为msgstr
                f.write(f"msgid \"{escaped_value}\"\n")
                f.write(f"msgstr \"{escaped_value}\"\n\n")

# 生成映射文件，用于将宏名映射到原始字符串
def generate_mapping_file(chinese_translations, english_translations, mapping_file):
    with codecs.open(mapping_file, 'w', 'utf-8') as f:
        for key in english_translations:
            if key in chinese_translations:
                en_value = english_translations[key]
                # 转义特殊字符
                escaped_en = en_value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                f.write(f"{key}\t{escaped_en}\n")

# 主函数
def main():
    header_file = 'internationalization.h'
    translations_dir = 'data/translations'
    
    # 确保翻译目录存在
    ensure_dir_exists(translations_dir)
    
    # 提取翻译字符串
    print("正在提取翻译字符串...")
    chinese_translations, english_translations = extract_translations(header_file)
    
    if not chinese_translations or not english_translations:
        print("提取翻译失败")
        return
    
    # 生成英文PO文件（源语言）
    en_po_file = os.path.join(translations_dir, 'en_US.po')
    print(f"正在生成英文PO文件: {en_po_file}")
    generate_po_file(english_translations, en_po_file, is_source=True)
    
    # 生成中文PO文件
    zh_po_file = os.path.join(translations_dir, 'zh_CN.po')
    print(f"正在生成中文PO文件: {zh_po_file}")
    
    # 对于中文PO文件，我们需要将英文作为msgid，中文作为msgstr
    with codecs.open(zh_po_file, 'w', 'utf-8') as f:
        f.write("# Chinese translation file\n")
        f.write("msgid \"\"\n")
        f.write("msgstr \"\"\n")
        f.write("Content-Type: text/plain; charset=UTF-8\\n\n")
        
        for key in english_translations:
            if key in chinese_translations:
                en_value = english_translations[key]
                zh_value = chinese_translations[key]
                # 转义特殊字符
                escaped_en = en_value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                escaped_zh = zh_value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
                f.write(f"msgid \"{escaped_en}\"\n")
                f.write(f"msgstr \"{escaped_zh}\"\n\n")
    
    # 生成映射文件
    mapping_file = os.path.join(translations_dir, 'macro_mapping.txt')
    print(f"正在生成宏映射文件: {mapping_file}")
    generate_mapping_file(chinese_translations, english_translations, mapping_file)
    
    print("转换完成！")
    print(f"生成的文件:")
    print(f"- {en_po_file}")
    print(f"- {zh_po_file}")
    print(f"- {mapping_file}")

if __name__ == '__main__':
    main()